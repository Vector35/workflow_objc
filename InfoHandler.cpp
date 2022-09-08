/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "InfoHandler.h"

#include "Constants.h"
#include "CustomTypes.h"
#include "Performance.h"

#include <algorithm>
#include <cinttypes>
#include <cstddef>
#include <regex>

using namespace BinaryNinja;

std::string InfoHandler::sanitizeText(const std::string& text)
{
    std::string result;
    std::string input = text.substr(0, 24);

    std::regex re("[a-zA-Z0-9]+");
    std::smatch sm;
    while (std::regex_search(input, sm, re)) {
        std::string part = sm[0];
        part[0] = static_cast<char>(std::toupper(part[0]));

        result += part;
        input = sm.suffix();
    }

    return result;
}

std::string InfoHandler::sanitizeSelector(const std::string& text)
{
    auto result = text;
    std::replace(result.begin(), result.end(), ':', '_');

    return result;
}

TypeRef InfoHandler::namedType(BinaryViewRef bv, const std::string& name)
{
    return Type::NamedType(bv, name);
}

TypeRef InfoHandler::stringType(std::size_t size)
{
    return Type::ArrayType(Type::IntegerType(1, true), size + 1);
}

void InfoHandler::defineVariable(BinaryViewRef bv, ObjectiveNinja::Address address, TypeRef type)
{
    bv->DefineUserDataVariable(address, type);
}

void InfoHandler::defineSymbol(BinaryViewRef bv, ObjectiveNinja::Address address, const std::string& name,
    const std::string& prefix, BNSymbolType symbolType)
{
    bv->DefineUserSymbol(new Symbol(symbolType, prefix + name, address));
}

void InfoHandler::defineReference(BinaryViewRef bv, ObjectiveNinja::Address from, ObjectiveNinja::Address to)
{
    bv->AddUserDataReference(from, to);
}

void InfoHandler::applyMethodType(BinaryViewRef bv, const std::string& base_name,
    const ObjectiveNinja::MethodInfo& mi)
{
    auto selectorTokens = mi.selectorTokens();
    auto typeTokens = mi.decodedTypeTokens();

    // For safety, ensure out-of-bounds indexing is not about to occur. This has
    // never happened and likely won't ever happen, but crashing the product is
    // generally undesirable, so it's better to be safe than sorry.
    if (selectorTokens.size() > typeTokens.size()) {
        LogWarn("Cannot apply method type to %" PRIx64 " due to selector/type token size mismatch.", mi.impl.address);
        return;
    }

    // Shorthand for formatting an individual "part" of the type signature.
    auto partForIndex = [selectorTokens, typeTokens](std::size_t i) {
        std::string argName;

        // Indices 0, 1, and 2 are the function return type, self parameter, and
        // selector parameter, respectively. Indices 3+ are the actual
        // arguments to the function.
        if (i == 0)
            argName = "";
        else if (i == 1)
            argName = "self";
        else if (i == 2)
            argName = "sel";
        else if (i - 3 < selectorTokens.size())
            argName = selectorTokens[i - 3];

        return typeTokens[i] + " " + argName;
    };

    // Build the type string for the method.
    std::string typeString;
    for (std::size_t i = 0; i < typeTokens.size(); ++i) {
        std::string suffix;
        auto part = partForIndex(i);

        // The underscore being used as the function name here is critically
        // important as Clang will not parse the type string correctly---unlike
        // the old type parser---if there is no function name. The underscore
        // itself isn't special, and will not end up being used as the function
        // name in either case.
        if (i == 0)
            suffix = " _(";
        else if (i == typeTokens.size() - 1)
            suffix = ")";
        else
            suffix = ", ";

        typeString += part + suffix;
    }
    typeString += ";";

    std::string errors;
    TypeParserResult tpResult;
    auto ok = bv->ParseTypesFromSource(typeString, {}, {}, tpResult, errors);
    if (ok && !tpResult.functions.empty()) {
        auto functionType = tpResult.functions[0].type;

        // Search for the method's implementation function; apply the type if found.
        if (auto f = bv->GetAnalysisFunction(bv->GetDefaultPlatform(), mi.impl.address))
            f->SetUserType(functionType);
    }

    // TODO: Use '+' or '-' conditionally once class methods are supported. For
    // right now, only instance methods are analyzed and we can just use '-'.
    auto name = "-[" + base_name + " " + mi.selectorName.referenced + "]";
    defineSymbol(bv, mi.impl.address, name, "", FunctionSymbol);
}

void InfoHandler::applyMethodListType(
    SharedAnalysisInfo info, BinaryViewRef bv,
    const TypeRef &taggedPointerType, const TypeRef &methodListType,
    std::size_t &totalMethods, const std::string& base_name,
    const ObjectiveNinja::MethodListInfo& mli, const std::string& prefix)
{
    if (mli.address && !mli.methods.empty()) {
        auto methodType = mli.hasRelativeOffsets()
            ? bv->GetTypeByName(CustomTypes::MethodListEntry)
            : bv->GetTypeByName(CustomTypes::Method);

        // Create a data variable and symbol for the method list header.
        defineVariable(bv, mli.address, methodListType);
        defineSymbol(bv, mli.address, base_name, prefix);

        // Create data variables for each method in the method list.
        for (const auto& mi : mli.methods) {
            ++totalMethods;
            defineReference(bv, mli.address, mi.address);

            defineVariable(bv, mi.address, methodType);
            defineSymbol(bv, mi.address, sanitizeSelector(mi.selectorName.referenced), "mt_");

            if (!info->selectorRefsByKey.count(mi.selectorName.address)) {
                defineVariable(bv, mi.selectorName.address, stringType(mi.selectorName.referenced.size()));
                defineSymbol(bv, mi.selectorName.address, sanitizeSelector(mi.selectorName.referenced), "sn_");
            }
            defineReference(bv, mi.address, mi.selectorName.address);

            defineVariable(bv, mi.type.address, stringType(mi.type.referenced.size()));
            defineSymbol(bv, mi.type.address, sanitizeSelector(mi.selectorName.referenced), "mu_");
            defineReference(bv, mi.address, mi.type.address);

            defineReference(bv, mi.address, mi.impl.address);

            if (mi.extendedType.list.address) {
                defineVariable(bv, mi.extendedType.list.address, taggedPointerType);
                defineSymbol(bv, mi.extendedType.list.address, sanitizeSelector(mi.selectorName.referenced), "emup_");
                if (mi.extendedType.entry.address) {
                    defineReference(bv, mi.extendedType.list.address, mi.extendedType.entry.address);
                    if (mi.extendedType.entry.address != mi.type.address) {
                        defineVariable(bv, mi.extendedType.entry.address, stringType(mi.extendedType.entry.referenced.size()));
                        defineSymbol(bv, mi.extendedType.entry.address, sanitizeSelector(mi.selectorName.referenced), "emu_");
                        defineReference(bv, mi.address, mi.extendedType.entry.address);
                    }
                }
            }

            applyMethodType(bv, base_name, mi);
        }
    }
}

void InfoHandler::applyInfoToView(SharedAnalysisInfo info, BinaryViewRef bv)
{
    auto start = Performance::now();

    bv->BeginUndoActions();

    BinaryReader reader(bv);

    auto taggedPointerType = namedType(bv, CustomTypes::TaggedPointer);
    auto cfStringType = namedType(bv, CustomTypes::CFString);
    auto classType = namedType(bv, CustomTypes::Class);
    auto classDataType = namedType(bv, CustomTypes::ClassRO);
    auto methodListType = namedType(bv, CustomTypes::MethodList);

    // Create data variables and symbols for all CFString instances.
    for (const auto& csi : info->cfStrings) {
        reader.Seek(csi.data.address);
        auto text = reader.ReadString(csi.size + 1);
        auto sanitizedText = sanitizeText(text);

        defineVariable(bv, csi.address, cfStringType);
        defineVariable(bv, csi.data.address, stringType(csi.size));
        defineSymbol(bv, csi.address, sanitizedText, "cf_");
        defineSymbol(bv, csi.data.address, sanitizedText, "as_");

        defineReference(bv, csi.address, csi.data.address);
    }

    // Create data variables and symbols for selectors and selector references.
    for (const auto& sr : info->selectorRefs) {
        auto sanitizedSelector = sanitizeSelector(sr->referenced.resolved.referenced);

        defineVariable(bv, sr->address, taggedPointerType);
        defineVariable(bv, sr->referenced.resolved.address, stringType(sr->referenced.resolved.referenced.size()));
        defineSymbol(bv, sr->address, sanitizedSelector, "stp_");
        defineSymbol(bv, sr->referenced.resolved.address, sanitizedSelector, "sn_");

        defineReference(bv, sr->address, sr->referenced.resolved.address);
    }

    auto totalMethods = std::size_t {0};

    std::map<ObjectiveNinja::Address, std::string> addressToClassMap;

    // Create data variables and symbols for the analyzed classes.
    for (const auto& cir : info->classes) {
        const auto& ci = cir.referenced;
        if (cir.address) {
            defineVariable(bv, cir.address, taggedPointerType);
            defineSymbol(bv, cir.address, ci.name.referenced, "ctp_");
            defineReference(bv, cir.address, ci.address);
        }
        defineVariable(bv, ci.address, classType);
        defineSymbol(bv, ci.address, ci.name.referenced, "ct_");
        defineVariable(bv, ci.data.address, classDataType);
        defineVariable(bv, ci.name.address, stringType(ci.name.referenced.size()));
        defineSymbol(bv, cir.address, ci.name.referenced, "cp_");
        defineSymbol(bv, ci.address, ci.name.referenced, "cl_");
        addressToClassMap[ci.address] = ci.name.referenced;
        defineSymbol(bv, ci.data.address, ci.name.referenced, "cd_");
        defineSymbol(bv, ci.name.address, ci.name.referenced, "nm_");

        defineReference(bv, cir.address, ci.address);
        defineReference(bv, ci.address, ci.data.address);
        if (ci.name.address) {
            defineVariable(bv, ci.name.address, stringType(ci.name.referenced.size()));
            defineSymbol(bv, ci.name.address, ci.name.referenced, "cn_");
            defineReference(bv, ci.data.address, ci.name.address);
        }
        if (ci.methodList.address) {
            defineReference(bv, ci.data.address, ci.methodList.address);
        }

        applyMethodListType(info, bv, taggedPointerType, methodListType,
            totalMethods, ci.name.referenced, ci.methodList.referenced, "ml_");
    }

    for (const auto classRef : info->classRefs) {
        bv->DefineDataVariable(classRef.address, taggedPointerType);

        if (classRef.referenced.address != 0) {
            auto localClass = addressToClassMap.find(classRef.referenced.address);
            if (localClass != addressToClassMap.end())
                defineSymbol(bv, classRef.address, localClass->second, "cr_");
        }
    }

    for (const auto superClassRef : info->superClassRefs) {
        bv->DefineDataVariable(superClassRef.address, taggedPointerType);

        if (superClassRef.referenced.address != 0) {
            auto localClass = addressToClassMap.find(superClassRef.referenced.address);
            if (localClass != addressToClassMap.end())
                defineSymbol(bv, superClassRef.address, localClass->second, "scr_");
        }
    }

    bv->CommitUndoActions();
    bv->UpdateAnalysis();

    auto elapsed = Performance::elapsed<std::chrono::milliseconds>(start);

    const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);
    log->LogInfo("Analysis results applied in %lu ms", elapsed.count());
    log->LogInfo("Found %d classes, %d methods, %d selector references",
        info->classes.size(), totalMethods, info->selectorRefs.size());
    log->LogInfo("Found %d CFString instances", info->cfStrings.size());
    log->LogInfo("Found %d class references", info->classRefs.size());
    log->LogInfo("Found %d super-class references", info->superClassRefs.size());
}
