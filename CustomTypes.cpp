/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "CustomTypes.h"

namespace CustomTypes {

using namespace BinaryNinja;

/**
 * Helper method for defining structure types.
 */
std::pair<QualifiedName, Ref<Type>> finalizeStructureBuilder(Ref<BinaryView> bv, StructureBuilder sb, std::string name)
{
    auto classTypeStruct = sb.Finalize();

    QualifiedName classTypeName(name);
    auto classTypeId = Type::GenerateAutoTypeId("objc", classTypeName);
    auto classType = Type::StructureType(classTypeStruct);
    auto classQualName = bv->DefineType(classTypeId, classTypeName, classType);

    return {classQualName, classType};
}

/**
 * Helper method for defining typedefs.
 */
inline std::pair<QualifiedName, Ref<Type>> defineTypedef(Ref<BinaryView> bv, const QualifiedName name, Ref<Type> type)
{
    auto typeID = Type::GenerateAutoTypeId("objc", name);
    auto typedefQualName = bv->DefineType(typeID, name, type);
    return {typedefQualName, Type::NamedType(bv, typedefQualName)};
}

void defineAll(Ref<BinaryView> bv)
{
    int addrSize = bv->GetAddressSize();

    auto type = defineTypedef(bv, {CustomTypes::FastPointer}, Type::PointerType(addrSize, Type::VoidType()));
    auto fastPointerType = type.second;
    type = defineTypedef(bv, {CustomTypes::RelativePointer}, Type::IntegerType(4, true));
    auto relativePointerType = type.second;
    type = defineTypedef(bv, {CustomTypes::TaggedPointer}, Type::PointerType(addrSize, Type::VoidType()));
    auto taggedPointerType = type.second;

    type = defineTypedef(bv, {CustomTypes::ID}, Type::PointerType(addrSize, Type::VoidType()));
    type = defineTypedef(bv, {CustomTypes::Selector}, Type::PointerType(addrSize, Type::IntegerType(1, false)));

    type = defineTypedef(bv, {CustomTypes::BOOL}, Type::IntegerType(1, false));
    type = defineTypedef(bv, {CustomTypes::NSInteger}, Type::IntegerType(addrSize, true));
    type = defineTypedef(bv, {CustomTypes::NSUInteger}, Type::IntegerType(addrSize, false));
    type = defineTypedef(bv, {CustomTypes::CGFloat}, Type::FloatType(addrSize));

    StructureBuilder cfstringStructBuilder;
    cfstringStructBuilder.AddMember(taggedPointerType, "isa");
    cfstringStructBuilder.AddMember(Type::IntegerType(addrSize, false), "flags");
    cfstringStructBuilder.AddMember(taggedPointerType, "data");
    cfstringStructBuilder.AddMember(Type::IntegerType(addrSize, false), "size");
    type = finalizeStructureBuilder(bv, cfstringStructBuilder, CustomTypes::CFString);

    StructureBuilder methodEntryBuilder;
    methodEntryBuilder.AddMember(relativePointerType, "name");
    methodEntryBuilder.AddMember(relativePointerType, "types");
    methodEntryBuilder.AddMember(relativePointerType, "imp");
    type = finalizeStructureBuilder(bv, methodEntryBuilder, CustomTypes::MethodListEntry);

    StructureBuilder methodBuilder;
    methodBuilder.AddMember(taggedPointerType, "name");
    methodBuilder.AddMember(taggedPointerType, "types");
    methodBuilder.AddMember(taggedPointerType, "imp");
    type = finalizeStructureBuilder(bv, methodBuilder, CustomTypes::Method);

    StructureBuilder methListBuilder;
    methListBuilder.AddMember(Type::IntegerType(2, false), "entsize");
    methListBuilder.AddMember(Type::IntegerType(2, false), "flags");
    methListBuilder.AddMember(Type::IntegerType(4, false), "count");
    type = finalizeStructureBuilder(bv, methListBuilder, CustomTypes::MethodList);

    StructureBuilder classROBuilder;
    classROBuilder.AddMember(Type::IntegerType(4, false), "flags");
    classROBuilder.AddMember(Type::IntegerType(4, false), "start");
    classROBuilder.AddMember(Type::IntegerType(4, false), "size");
    if (addrSize == 8)
        classROBuilder.AddMember(Type::IntegerType(4, false), "reserved");
    classROBuilder.AddMember(taggedPointerType, "ivar_layout");
    classROBuilder.AddMember(taggedPointerType, "name");
    classROBuilder.AddMember(taggedPointerType, "methods");
    classROBuilder.AddMember(taggedPointerType, "protocols");
    classROBuilder.AddMember(taggedPointerType, "ivars");
    classROBuilder.AddMember(taggedPointerType, "weak_ivar_layout");
    classROBuilder.AddMember(taggedPointerType, "properties");
    type = finalizeStructureBuilder(bv, classROBuilder, CustomTypes::ClassRO);

    StructureBuilder classBuilder;
    classBuilder.AddMember(taggedPointerType, "isa");
    classBuilder.AddMember(taggedPointerType, "super");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "cache");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "vtable");
    classBuilder.AddMember(fastPointerType, "data");
    type = finalizeStructureBuilder(bv, classBuilder, CustomTypes::Class);

    StructureBuilder instanceVariableBuilder;
    instanceVariableBuilder.AddMember(Type::PointerType(addrSize, Type::IntegerType(4, false)), "offset");
    instanceVariableBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "name");
    instanceVariableBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "type");
    instanceVariableBuilder.AddMember(Type::IntegerType(4, false), "alignment");
    instanceVariableBuilder.AddMember(Type::IntegerType(4, false), "size");
    type = finalizeStructureBuilder(bv, instanceVariableBuilder, CustomTypes::InterfaceVariable);

    StructureBuilder instanceVariableListBuilder;
    instanceVariableListBuilder.AddMember(Type::IntegerType(4, false), "entsize");
    instanceVariableListBuilder.AddMember(Type::IntegerType(4, false), "count");
    type = finalizeStructureBuilder(bv, instanceVariableListBuilder, CustomTypes::InterfaceVariableList);

}

}
