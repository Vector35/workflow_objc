/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "Workflow.h"

#include "ArchitectureHooks.h"
#include "Constants.h"
#include "CustomTypes.h"
#include "GlobalState.h"
#include "InfoHandler.h"
#include "Performance.h"

#include "Core/AnalysisProvider.h"
#include "Core/BinaryViewFile.h"

#include <lowlevelilinstruction.h>

#include <queue>

static std::mutex g_initialAnalysisMutex;

using SectionRef = BinaryNinja::Ref<BinaryNinja::Section>;
using SymbolRef = BinaryNinja::Ref<BinaryNinja::Symbol>;

void Workflow::rewriteMethodCall(LLILFunctionRef ssa, size_t insnIndex)
{
    const auto bv = ssa->GetFunction()->GetView();
    const auto llil = ssa->GetNonSSAForm();
    const auto insn = ssa->GetInstruction(insnIndex);
    const auto params = insn.GetParameterExprs<LLIL_CALL_SSA>();

    // The second parameter passed to the objc_msgSend call is the address of
    // either the selector reference or the method's name, which in both cases
    // is dereferenced to retrieve a selector.
    const auto selectorRegister = params[1].GetSourceSSARegister<LLIL_REG_SSA>();
    uint64_t rawSelector = ssa->GetSSARegisterValue(selectorRegister).value;

    // Check the analysis info for a selector reference corresponding to the
    // current selector. It is possible no such selector reference exists, for
    // example, if the selector is for a method defined outside the current
    // binary. If this is the case, there are no meaningful changes that can be
    // made to the IL, and the operation should be aborted.
    const auto info = GlobalState::analysisInfo(bv);
    if (!info || !info->selectorRefsByKey.count(rawSelector))
        return;
    const auto selectorRef = info->selectorRefsByKey[rawSelector];

    // Attempt to look up the implementation for the given selector, first by
    // using the raw selector, then by the address of the selector reference. If
    // the lookup fails in both cases, abort.
    uint64_t implAddress = info->methodImpls[selectorRef->rawSelector];
    if (!implAddress)
        implAddress = info->methodImpls[selectorRef->address];
    if (!implAddress)
        return;

    const auto llilIndex = ssa->GetNonSSAInstructionIndex(insnIndex);
    auto llilInsn = llil->GetInstruction(llilIndex);

    // Change the destination expression of the LLIL_CALL operation to point to
    // the method implementation. This turns the "indirect call" piped through
    // `objc_msgSend` and makes it a normal C-style function call.
    auto callDestExpr = llilInsn.GetDestExpr<LLIL_CALL>();
    callDestExpr.Replace(llil->ConstPointer(callDestExpr.size, implAddress, callDestExpr));
    llilInsn.Replace(llil->Call(callDestExpr.exprIndex, llilInsn));

    llil->GenerateSSAForm();
}

void Workflow::rewriteCFString(LLILFunctionRef ssa, size_t insnIndex)
{
    const auto bv = ssa->GetFunction()->GetView();
    const auto llil = ssa->GetNonSSAForm();
    const auto insn = ssa->GetInstruction(insnIndex);
    const auto llilIndex = ssa->GetNonSSAInstructionIndex(insnIndex);
    auto llilInsn = llil->GetInstruction(llilIndex);

    auto sourceExpr = insn.GetSourceExpr<LLIL_SET_REG_SSA>();
    auto destRegister = llilInsn.GetDestRegister();

    auto addr = sourceExpr.GetValue().value;
    auto stringPointer = addr + 0x10;
    uint64_t dest;
    bv->Read(&dest, stringPointer, bv->GetDefaultArchitecture()->GetAddressSize());

    auto targetPointer = llil->ConstPointer(bv->GetAddressSize(), dest, llilInsn);
    auto cfstrCall = llil->Intrinsic({ BinaryNinja::RegisterOrFlag(0, destRegister) }, CFSTRIntrinsicIndex, { targetPointer }, 0, llilInsn);

    llilInsn.Replace(cfstrCall);

    llil->GenerateSSAForm();
    llil->Finalize();
}

void Workflow::inlineMethodCalls(AnalysisContextRef ac)
{
    const auto func = ac->GetFunction();
    const auto arch = func->GetArchitecture();
    const auto bv = func->GetView();

    if (GlobalState::viewIsIgnored(bv))
        return;

    const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);

    // Ignore the view if it has an unsupported architecture.
    //
    // The reasoning for querying the default architecture here rather than the
    // architecture of the function being analyzed is that the view needs to
    // have a default architecture for the Objective-C runtime types to be
    // defined successfully.
    auto defaultArch = bv->GetDefaultArchitecture();
    auto defaultArchName = defaultArch ? defaultArch->GetName() : "";
    if (defaultArchName != "aarch64" && defaultArchName != "x86_64") {
        if (!defaultArch)
            log->LogError("View must have a default architecture.");
        else
            log->LogError("Architecture '%s' is not supported", defaultArchName.c_str());

        GlobalState::addIgnoredView(bv);
        return;
    }

    // The workflow relies on some data acquired through analysis of Objective-C
    // structures present in the binary. The structure analysis must run
    // exactly once per binary. Until the Workflows API supports a "run once"
    // idiom, this is accomplished through a mutex and a check for present
    // analysis information.
    {
        std::scoped_lock<std::mutex> lock(g_initialAnalysisMutex);

        if (!GlobalState::hasAnalysisInfo(bv)) {
            SharedAnalysisInfo info;
            CustomTypes::defineAll(bv);
            auto messageHandler = GlobalState::messageHandler(bv);

            try {
                auto file = std::make_shared<ObjectiveNinja::BinaryViewFile>(bv);

                auto start = Performance::now();
                info = ObjectiveNinja::AnalysisProvider::infoForFile(file);
                auto elapsed = Performance::elapsed<std::chrono::milliseconds>(start);

                const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);
                log->LogInfo("Structures analyzed in %lu ms", elapsed.count());

                InfoHandler::applyInfoToView(info, bv);

                const auto msgSendFunctions = messageHandler->getMessageSendFunctions();
                for (auto addr : msgSendFunctions) {
                    BinaryNinja::QualifiedNameAndType nameAndType;
                    std::string errors;
                    std::set<BinaryNinja::QualifiedName> typesAllowRedefinition;

                    // void *
                    auto retType = BinaryNinja::Confidence<BinaryNinja::Ref<BinaryNinja::Type>>(
                        BinaryNinja::Type::PointerType(bv->GetAddressSize(), BinaryNinja::Type::VoidType(),
                            0));

                    std::vector<BinaryNinja::FunctionParameter> params;
                    auto cc = bv->GetDefaultPlatform()->GetDefaultCallingConvention();

                    params.push_back({ "self",
                        BinaryNinja::Type::NamedType(bv, { "id" }),
                        true,
                        BinaryNinja::Variable() });
                    params.push_back({ "sel",
                        BinaryNinja::Type::PointerType(bv->GetAddressSize(), BinaryNinja::Type::IntegerType(1, false)),
                        true,
                        BinaryNinja::Variable() });

                    auto funcType = BinaryNinja::Type::FunctionType(retType, cc, params, true);
                    bv->DefineDataVariable(addr, BinaryNinja::Type::PointerType(bv->GetDefaultArchitecture(), funcType));
                }
            } catch (...) {
                log->LogError("Structure analysis failed; binary may be malformed.");
                log->LogError("Objective-C analysis will not be applied due to previous errors.");
            }

            GlobalState::setFlag(bv, Flag::DidRunStructureAnalysis);
            GlobalState::storeAnalysisInfo(bv, info);
        }
    }
    auto messageHandler = GlobalState::messageHandler(bv);

    if (!messageHandler->hasMessageSendFunctions()) {
        log->LogError("Cannot perform Objective-C IL cleanup; no objc_msgSend candidates found");
        GlobalState::addIgnoredView(bv);
        return;
    }

    const auto llil = ac->GetLowLevelILFunction();
    if (!llil) {
        log->LogError("(Workflow) Failed to get LLIL for 0x%llx", func->GetStart());
        return;
    }
    const auto ssa = llil->GetSSAForm();
    if (!ssa) {
        log->LogError("(Workflow) Failed to get LLIL SSA form for 0x%llx", func->GetStart());
        return;
    }

    const auto rewriteIfEligible = [bv, llil, messageHandler, ssa](size_t insnIndex) {
        auto insn = ssa->GetInstruction(insnIndex);

        if (insn.operation == LLIL_CALL_SSA
            || (insn.operation == LLIL_JUMP && insnIndex == ssa->GetInstructionCount() - 1)
            || insn.operation == LLIL_TAILCALL_SSA) {
            // Filter out calls that aren't to `objc_msgSend`.
            auto callExpr = insn.GetDestExpr();
            if (insn.operation == LLIL_CALL_SSA
                && messageHandler->isMessageSend(callExpr.GetValue().value)) {
                auto params = insn.GetParameterExprs();
                if (params.size() >= 2
                    && params[0].operation == LLIL_REG_SSA
                    && params[1].operation == LLIL_REG_SSA)
                    rewriteMethodCall(ssa, insnIndex);
            } else if (messageHandler->isARCFunction(callExpr.GetValue().value)) {
                auto nonSSAIdx = ssa->GetNonSSAInstructionIndex(insnIndex);
                auto targetInsn = llil->GetInstruction(nonSSAIdx);
                if (insn.operation == LLIL_CALL_SSA)
                    targetInsn.Replace(llil->Nop(targetInsn));
                else // Other two categories. TailCall or Jump that is last instruction (so, tailcall...)
                {
                    auto lr = llil->GetArchitecture()->GetLinkRegister();
                    auto lrInfo = llil->GetArchitecture()->GetRegisterInfo(lr);

                    targetInsn.Replace(llil->Return(llil->Register(lrInfo.size, lr, targetInsn), targetInsn));
                }
                llil->GenerateSSAForm();
                llil->Finalize();
                return;
            }
            if (messageHandler->isFunctionLocatedInStubSection(callExpr.GetValue().value))
                messageHandler->functionWasAnalyzed(llil->GetFunction()->GetStart());
        } else if (insn.operation == LLIL_SET_REG_SSA) {
            auto sourceExpr = insn.GetSourceExpr<LLIL_SET_REG_SSA>();
            auto addr = sourceExpr.GetValue().value;
            BinaryNinja::DataVariable var;
            if (!bv->GetDataVariableAtAddress(addr, var) || var.type->GetString() != "struct CFString")
                return;

            rewriteCFString(ssa, insnIndex);
        }
    };

    for (const auto& block : ssa->GetBasicBlocks())
        for (size_t i = block->GetStart(), end = block->GetEnd(); i < end; ++i)
            rewriteIfEligible(i);
}

static constexpr auto WorkflowInfo = R"({
  "title": "Objective-C",
  "description": "Enhanced analysis for Objective-C code.",
  "capabilities": []
})";

void Workflow::registerActivities()
{
    const auto wf = BinaryNinja::Workflow::Instance()->Clone("core.function.objectiveC");
    wf->RegisterActivity(new BinaryNinja::Activity(
        ActivityID::ResolveMethodCalls, &Workflow::inlineMethodCalls));
    wf->Insert("core.function.translateTailCalls", ActivityID::ResolveMethodCalls);

    BinaryNinja::Workflow::RegisterWorkflow(wf, WorkflowInfo);
}
