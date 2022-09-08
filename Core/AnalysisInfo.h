/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace ObjectiveNinja {

using Address = uint64_t;

/**
 * A description of an address.
 */
struct AddressInfo {
    Address address {};

    AddressInfo() noexcept = default;
    AddressInfo(Address address) noexcept : address(address) {}
};

/**
 * A description of a list entry, including its containing list.
 */
template<typename T>
struct ListEntryInfo {
    AddressInfo list {};
    T entry {};

    ListEntryInfo() noexcept = default;
    template<typename... Args>
    ListEntryInfo(Address listAddress, Args&&... entryArgs) : list(listAddress), entry(std::forward<Args>(entryArgs)...) {}
};

/**
 * A description of a reference.
 */
template<typename T>
struct RefInfo : AddressInfo {
    T referenced {};

    RefInfo() noexcept = default;
    template<typename... Args>
    RefInfo(Address address, Args&&... referencedArgs) : AddressInfo(address), referenced(std::forward<Args>(referencedArgs)...) {}
};

/**
 * A description of an address reference.
 */
using AddressRefInfo = RefInfo<AddressInfo>;

/**
 * A description of an unresolved address.
 */
struct UnresolvedAddressInfo {
    Address unresolvedAddress {};

    UnresolvedAddressInfo() noexcept = default;
    UnresolvedAddressInfo(Address unresolvedAddress) : unresolvedAddress(unresolvedAddress) {}
};

/**
 * A description of a resolved value, including its unresolved address.
 */
template<typename T>
struct UnresolvedInfo : UnresolvedAddressInfo {
    T resolved {};

    UnresolvedInfo() noexcept = default;
    template<typename... Args>
    UnresolvedInfo(Address unresolvedAddress, Args&&... resolvedArgs) : UnresolvedAddressInfo(unresolvedAddress), resolved(std::forward<Args>(resolvedArgs)...) {}
};

/**
 * A description of a CFString instance.
 */
struct CFStringInfo : AddressInfo {
    AddressInfo data {};
    size_t size {};
};

/**
 * A description of a selector name.
 */
using SelectorNameInfo = RefInfo<std::string>;

/**
 * A description of a selector reference.
 */
using SelectorRefInfo = RefInfo<UnresolvedInfo<SelectorNameInfo>>;

using SharedSelectorRefInfo = std::shared_ptr<SelectorRefInfo>;

/**
 * A description of an Objective-C method.
 */
struct MethodInfo : AddressInfo {
    RefInfo<std::string> selectorName {};
    RefInfo<std::string> type {};
    AddressInfo impl {};
    ListEntryInfo<RefInfo<std::string>> extendedType;

    /**
     * Get the selector as a series of tokens, split at ':' characters.
     */
    std::vector<std::string> selectorTokens() const;

    /**
     * Get the method's type as series of C-style tokens.
     */
    std::vector<std::string> decodedTypeTokens() const;
};

/**
 * A description of an Objective-C method list.
 */
struct MethodListInfo : AddressInfo {
    uint16_t entsize {};
    uint16_t flags {};
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

/**
 * A description of an Objective-C property.
 */
struct PropertyInfo : AddressInfo {
    RefInfo<std::string> name;
    RefInfo<std::string> attributes;
};

/**
 * A description of an Objective-C property list.
 */
struct PropertyListInfo : AddressInfo {
    uint16_t entsize {};
    uint16_t flags {};
    std::vector<PropertyInfo> properties {};

    /**
     * Tells whether the method list uses relative offsets or not.
     */
    bool hasRelativeOffsets() const;
};

/**
 * A description of an Objective-C class.
 */
struct ClassInfo : AddressInfo {
    RefInfo<std::string> name {};
    AddressInfo data {};
    RefInfo<MethodListInfo> methodList {};
    RefInfo<PropertyListInfo> propertyList {};
};

/**
 * A description of an Objective-C class reference.
 */
using ClassRefInfo = RefInfo<ClassInfo>;

/**
 * Analysis info storage.
 *
 * AnalysisInfo is intended to be a common structure for persisting information
 * during and after analysis. All significant info obtained or produced through
 * analysis should be stored here, ideally in the form of other *Info structs.
 */
struct AnalysisInfo {
    std::vector<CFStringInfo> cfStrings {};

    std::vector<AddressRefInfo> classRefs {};
    std::vector<AddressRefInfo> superClassRefs {};

    std::vector<ClassRefInfo> classes {};
    std::unordered_map<Address, AddressInfo> methodImpls;

    std::unordered_map<Address, AddressInfo> propertiesByKey {};

    std::vector<SharedSelectorRefInfo> selectorRefs {};
    std::unordered_map<Address, SharedSelectorRefInfo> selectorRefsByKey {};

    std::string dump() const;
};

}
