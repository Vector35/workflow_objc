/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "TypeParser.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ObjectiveNinja {

/**
 * A description of a CFString instance.
 */
struct CFStringInfo {
    uint64_t address {};
    uint64_t dataAddress {};
    size_t size {};
};

/**
 * A description of a selector reference.
 */
struct SelectorRefInfo {
    uint64_t address {};

    std::string name {};

    uint64_t rawSelector {};
    uint64_t nameAddress {};
};

using SharedSelectorRefInfo = std::shared_ptr<SelectorRefInfo>;

/**
 * A description of an Objective-C method.
 */
struct MethodInfo {
    uint64_t address {};

    std::string selector;
    std::string type;

    uint64_t nameAddress {};
    uint64_t typeAddress {};
    uint64_t implAddress {};

    /**
     * Get the selector as a series of tokens, split at ':' characters.
     */
    std::vector<std::string> selectorTokens() const;

    /**
     * Get the method's type as series of C-style tokens.
     */
    std::vector<QualifiedNameOrType> decodedTypeTokens() const;
};

/**
 * A description of an Objective-C method list.
 */
struct MethodListInfo {
    uint64_t address {};
    uint32_t flags {};
    std::vector<MethodInfo> methods {};

    /**
     * Tells whether the method list uses relative offsets or not.
     */
    bool hasRelativeOffsets() const;

    /**
     * Tells whether the method list uses direct selectors or not.
     */
    bool hasDirectSelectors() const;
};

struct MetaClassInfo;

/**
 * A description of an Objective-C instance variable (ivar).
 */
struct IvarInfo {
    uint64_t address = {};

    uint32_t offset;
    std::string name;
    std::string type;

    uint64_t offsetAddress {};
    uint64_t nameAddress {};
    uint64_t typeAddress {};
    uint32_t size {};

    /**
     * Get the instance variable's type as a C-style token.
     */
    QualifiedNameOrType decodedTypeToken() const;
};

/**
 * A description of an Objective-C instance variable list.
 */
struct IvarListInfo {
    uint64_t address {};

    uint32_t count {};
    std::vector<IvarInfo> ivars {};
};

/**
 * A description of an Objective-C class.
 */
struct ClassInfo {
    uint64_t address {};

    bool isMetaClass;
    MetaClassInfo* metaClassInfo;

    std::string name {};
    MethodListInfo methodList {};
    IvarListInfo ivarList {};

    uint64_t listPointer {};
    uint64_t dataAddress {};
    uint64_t nameAddress {};
    uint64_t methodListAddress {};
    uint64_t ivarListAddress {};
};
/**
 * A description of an Objective-C category.
 */
struct CategoryInfo {
    uint64_t address {};

    std::string name {};
    std::string className {};
    MethodListInfo methodList {};
    MethodListInfo classMethodList {};

    uint64_t listPointer {};
    uint64_t classPointer {};
    /// Address of the pointer to the class (which may be an ImportedDataSymbol)
    uint64_t classPointerAddress {};
    uint64_t nameAddress {};
    uint64_t methodListAddress {};
    uint64_t classMethodListAddress {};
};

struct MetaClassInfo {
    std::string name {};
    bool imported;
    ClassInfo info {};
};

struct ClassRefInfo {
    uint64_t address;
    uint64_t referencedAddress;
};

/**
 * Analysis info storage.
 *
 * AnalysisInfo is intended to be a common structure for persisting information
 * during and after analysis. All significant info obtained or produced through
 * analysis should be stored here, ideally in the form of other *Info structs.
 */
struct AnalysisInfo {
    std::vector<CFStringInfo> cfStrings {};

    std::vector<ClassRefInfo> classRefs {};
    std::vector<ClassRefInfo> superRefs {};
    std::vector<SharedSelectorRefInfo> selectorRefs {};
    std::unordered_map<uint64_t, SharedSelectorRefInfo> selectorRefsByKey {};

    std::vector<ClassInfo> classes {};
    std::vector<CategoryInfo> categories {};
    std::unordered_map<uint64_t, uint64_t> methodImpls;

    std::string dump() const;
};

}
