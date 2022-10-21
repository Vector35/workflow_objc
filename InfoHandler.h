/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "Core/AnalysisInfo.h"

#include "BinaryNinja.h"

#include <cstddef>
#include <memory>
#include <string>

using SharedAnalysisInfo = std::shared_ptr<ObjectiveNinja::AnalysisInfo>;

/**
 * Utility class for applying collected AnalysisInfo to a database.
 *
 * InfoHandler is meant to be used after all analyzers intended to run on a
 * database have finished. The resulting AnalysisInfo will then be used to
 * create data variables, symbols, etc. in the database.
 */
class InfoHandler {
    /**
     * Sanitize a string by searching for series of alphanumeric characters and
     * concatenating the matches. The input string will first be truncated.
     */
    static std::string sanitizeText(const std::string&);

    /**
     * Sanitize a selector so that it round-trips the type parser. Colon
     * characters will be replaced underscores.
     */
    static std::string sanitizeSelector(const std::string&);

    /**
     * Get the type with the given name defined inside the BinaryView.
     */
    static inline TypeRef namedType(BinaryViewRef, const std::string&);

    /**
     * Create a type for a string (character array) of the given size.
     */
    static inline TypeRef stringType(std::size_t);

    /**
     * Shorthand function for defining a user data variable.
     */
    static inline void defineVariable(BinaryViewRef, ObjectiveNinja::Address, TypeRef);

    /**
     * Shorthand function for defining a user symbol, with an optional prefix.
     */
    static inline void defineSymbol(BinaryViewRef, ObjectiveNinja::Address,
        const std::string& name, const std::string& prefix = "",
        BNSymbolType type = DataSymbol);

    /**
     * Shorthand function for defining a user data reference.
     */
    static inline void defineReference(BinaryViewRef bv, ObjectiveNinja::Address from, ObjectiveNinja::Address to);

    /**
     * Create a symbol and apply return/argument types for a method.
     */
    static void applyMethodType(BinaryViewRef, const std::string& base_name,
        const ObjectiveNinja::MethodInfo&);

    /**
     * Create variables & symbols and apply return/argument types for each method in a method list.
     */
    static void applyMethodListType(SharedAnalysisInfo, BinaryViewRef,
        const TypeRef&, const TypeRef&,
        std::size_t&, const std::string& base_name,
        const ObjectiveNinja::MethodListInfo&, const std::string&);

public:
    /**
     * Apply AnalysisInfo to a BinaryView.
     */
    static void applyInfoToView(SharedAnalysisInfo, BinaryViewRef);
};
