/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "DataRenderers.h"

#include <cinttypes>
#include <cstdio>

using namespace BinaryNinja;

/**
 * Get the appropriate token type for a pointer to a given symbol.
 */
BNInstructionTextTokenType tokenTypeForSymbol(Ref<Symbol> symbol)
{
    switch (symbol->GetType()) {
    case DataSymbol:
        return DataSymbolToken;
    case FunctionSymbol:
        return CodeSymbolToken;
    default:
        return CodeRelativeAddressToken;
    }
}

/**
 * Get a line for a given pointer.
 */
DisassemblyTextLine lineForPointer(BinaryView* bv, uint64_t pointer,
    uint64_t address, const std::vector<InstructionTextToken>& prefix)
{
    std::string tokenText = "???";
    auto tokenType = CodeRelativeAddressToken;

    Ref<Symbol> symbol = bv->GetSymbolByAddress(pointer);
    if (pointer == 0 || pointer == bv->GetStart()) {
        tokenText = "NULL";
        tokenType = KeywordToken;
    } else if (symbol) {
        tokenText = symbol->GetFullName();
        tokenType = tokenTypeForSymbol(symbol);
    } else {
        char addressBuffer[32];
        snprintf(addressBuffer, sizeof(addressBuffer), "0x%" PRIx64, pointer);

        tokenText = std::string(addressBuffer);
        tokenType = CodeRelativeAddressToken;
    }

    DisassemblyTextLine line;
    line.addr = address;
    line.tokens = prefix;
    line.tokens.emplace_back(tokenType, tokenText, pointer);

    return { line };
}

/**
 * Checks if the deepest type in the data renderer context is a named type with
 * the given name.
 */
bool isType(const DataRendererContext& context, const std::string& name)
{
    if (context.empty())
        return false;

    auto [deepestType, size] = context.back();
    if (!deepestType->IsNamedTypeRefer())
        return false;

    return deepestType->GetTypeName().GetString() == name;
}

/* ---- Relative Pointer ---------------------------------------------------- */

bool RelativePointerDataRenderer::IsValidForData(BinaryView* bv, uint64_t address,
    Type* type, DataRendererContext& context)
{
    return isType(context, "rptr_t");
}

std::vector<DisassemblyTextLine> RelativePointerDataRenderer::GetLinesForData(
    BinaryView* bv, uint64_t address, Type*,
    const std::vector<InstructionTextToken>& prefix, size_t,
    DataRendererContext&)
{
    BinaryReader reader(bv);
    reader.Seek(address);

    auto pointer = (int32_t)reader.Read32() + address;

    return { lineForPointer(bv, pointer, address, prefix) };
}

void RelativePointerDataRenderer::Register()
{
    DataRendererContainer::RegisterTypeSpecificDataRenderer(new RelativePointerDataRenderer());
}
