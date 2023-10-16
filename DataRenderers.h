/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "BinaryNinja.h"

using DataRendererContext = std::vector<std::pair<TypePtr, size_t>>;

/**
 * Data renderer for relative offset pointers.
 */
class RelativePointerDataRenderer : public BinaryNinja::DataRenderer {
    RelativePointerDataRenderer() = default;

public:
    bool IsValidForData(BinaryViewPtr, uint64_t address, TypePtr,
        DataRendererContext&) override;

    std::vector<BinaryNinja::DisassemblyTextLine> GetLinesForData(
        BinaryViewPtr, uint64_t address, TypePtr,
        const std::vector<BinaryNinja::InstructionTextToken>& prefix,
        size_t width, DataRendererContext&) override;

    static void Register();
};
