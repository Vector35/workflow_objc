/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "BinaryNinja.h"

/**
 * All type-related things.
 */
namespace CustomTypes {

const std::string FastPointer = "fptr_t";
const std::string RelativePointer = "rptr_t";
const std::string TaggedPointer = "tptr_t";

const std::string ID = "id";
const std::string Selector = "SEL";

const std::string BOOL = "BOOL";
const std::string CGFloat = "CGFloat";
const std::string NSInteger = "NSInteger";
const std::string NSUInteger = "NSUInteger";

const std::string CFString = "CFString";

const std::string Class = "objc_class_t";
const std::string ClassRO = "objc_class_ro_t";
const std::string InterfaceVariable = "objc_ivar_t";
const std::string InterfaceVariableList = "objc_ivar_t";
const std::string Method = "objc_method_t";
const std::string MethodList = "objc_method_list_t";
const std::string MethodListEntry = "objc_method_entry_t";
const std::string Property = "objc_property_t";
const std::string PropertyList = "objc_property_list_t";
const std::string PropertyListEntry = "objc_property_entry_t";
const std::string Protocol = "objc_protocol_t";
const std::string ProtocolList = "objc_protocol_list_t";

/**
 * Define all Objective-C-related types for a view.
 */
void defineAll(BinaryNinja::Ref<BinaryNinja::BinaryView>);

}
