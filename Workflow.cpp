/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "Workflow.h"

#include "Constants.h"
#include "GlobalState.h"
#include "Performance.h"
#include "ArchitectureHooks.h"

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

    // k: also check direct selector value (x64 does this)
    const auto info = GlobalState::analysisInfo(bv);
    if (!info)
        return;
    std::vector<uint64_t> imps;
    if (const auto& it = info->selRefToImp.find(rawSelector); it != info->selRefToImp.end())
        imps = it->second;
    else if (const auto& iter = info->selToImp.find(rawSelector); iter != info->selToImp.end())
        imps = iter->second;

    if (imps.empty())
        return;

    // Attempt to look up the implementation for the given selector, first by
    // using the raw selector, then by the address of the selector reference. If
    // the lookup fails in both cases, abort.

    // k: This is the same behavior as before, however it is more apparent now by implementation
    //      that we are effectively just guessing which method this hits. This has _obvious_ drawbacks,
    //      but until we have more robust typing and objective-c type libraries, fixing this would
    //      make the objective-c workflow do effectively nothing.
    uint64_t implAddress = imps[0];
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
    auto cfstrCall = llil->Intrinsic({ BinaryNinja::RegisterOrFlag(0, destRegister) }, CFSTRIntrinsicIndex, {targetPointer}, 0, llilInsn);

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

    const auto rewriteIfEligible = [bv, messageHandler, ssa](size_t insnIndex) {
        auto insn = ssa->GetInstruction(insnIndex);

        if (insn.operation == LLIL_CALL_SSA)
        {
            // Filter out calls that aren't to `objc_msgSend`.
            auto callExpr = insn.GetDestExpr<LLIL_CALL_SSA>();
            if (!messageHandler->isMessageSend(callExpr.GetValue().value))
                return;

            // By convention, the selector is the second argument to `objc_msgSend`,
            // therefore two parameters are required for a proper rewrite; abort if
            // this condition is not met.
            auto params = insn.GetParameterExprs<LLIL_CALL_SSA>();
            if (params.size() < 2
                || params[0].operation != LLIL_REG_SSA
                || params[1].operation != LLIL_REG_SSA)
                return;

            rewriteMethodCall(ssa, insnIndex);

        }
        else if (insn.operation == LLIL_SET_REG_SSA)
        {
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
