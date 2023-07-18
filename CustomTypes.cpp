/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
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
inline void defineTypedef(Ref<BinaryView> bv, const QualifiedName name, Ref<Type> type)
{
    auto typeID = Type::GenerateAutoTypeId("objc", name);
    bv->DefineType(typeID, name, type);
}

void defineAll(Ref<BinaryView> bv)
{
    int addrSize = bv->GetAddressSize();

    defineTypedef(bv, {CustomTypes::TaggedPointer}, Type::PointerType(addrSize, Type::VoidType()));
    defineTypedef(bv, {CustomTypes::FastPointer}, Type::PointerType(addrSize, Type::VoidType()));
    defineTypedef(bv, {CustomTypes::RelativePointer}, Type::IntegerType(4, true));

    defineTypedef(bv, {"id"}, Type::PointerType(addrSize, Type::VoidType()));
    defineTypedef(bv, {"SEL"}, Type::PointerType(addrSize, Type::IntegerType(1, false)));

    defineTypedef(bv, {"BOOL"}, Type::IntegerType(1, false));
    defineTypedef(bv, {"NSInteger"}, Type::IntegerType(addrSize, true));
    defineTypedef(bv, {"NSUInteger"}, Type::IntegerType(addrSize, false));
    defineTypedef(bv, {"CGFloat"}, Type::FloatType(addrSize));

    StructureBuilder cfstringStructBuilder;
    cfstringStructBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType() ), "isa");
    cfstringStructBuilder.AddMember(Type::IntegerType(addrSize, false), "flags");
    cfstringStructBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType() ), "data");
    cfstringStructBuilder.AddMember(Type::IntegerType(addrSize, false), "size");
    auto type = finalizeStructureBuilder(bv, cfstringStructBuilder, "CFString");

    StructureBuilder methodEntry;
    methodEntry.AddMember(Type::IntegerType(4, true), "name");
    methodEntry.AddMember(Type::IntegerType(4, true), "types");
    methodEntry.AddMember(Type::IntegerType(4, true), "imp");
    type = finalizeStructureBuilder(bv, methodEntry, "objc_method_entry_t");

    StructureBuilder method;
    method.AddMember(Type::PointerType(addrSize, Type::VoidType()), "name");
    method.AddMember(Type::PointerType(addrSize, Type::VoidType()), "types");
    method.AddMember(Type::PointerType(addrSize, Type::VoidType()), "imp");
    type = finalizeStructureBuilder(bv, method, "objc_method_t");

    StructureBuilder methList;
    methList.AddMember(Type::IntegerType(4, false), "obsolete");
    methList.AddMember(Type::IntegerType(4, false), "count");
    type = finalizeStructureBuilder(bv, methList, "objc_method_list_t");

    StructureBuilder classROBuilder;
    classROBuilder.AddMember(Type::IntegerType(4, false), "flags");
    classROBuilder.AddMember(Type::IntegerType(4, false), "start");
    classROBuilder.AddMember(Type::IntegerType(4, false), "size");
    if (addrSize == 8)
        classROBuilder.AddMember(Type::IntegerType(4, false), "reserved");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "ivar_layout");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "name");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "methods");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "protocols");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "ivars");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "weak_ivar_layout");
    classROBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "properties");
    type = finalizeStructureBuilder(bv, classROBuilder, "objc_class_ro_t");

    StructureBuilder classBuilder;
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "isa");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "super");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "cache");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "vtable");
    classBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "data");
    type = finalizeStructureBuilder(bv, classBuilder, "objc_class_t");

    StructureBuilder categoryBuilder;
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "category_name");
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "class");
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "inst_methods");
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "class_methods");
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "protocols");
    categoryBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "properties");
    type = finalizeStructureBuilder(bv, categoryBuilder, "objc_category_t");

    StructureBuilder ivarBuilder;
    ivarBuilder.AddMember(Type::PointerType(addrSize, Type::IntegerType(4, false)), "offset");
    ivarBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "name");
    ivarBuilder.AddMember(Type::PointerType(addrSize, Type::VoidType()), "type");
    ivarBuilder.AddMember(Type::IntegerType(4, false), "alignment");
    ivarBuilder.AddMember(Type::IntegerType(4, false), "size");
    type = finalizeStructureBuilder(bv, ivarBuilder, "objc_ivar_t");

    StructureBuilder ivarList;
    ivarList.AddMember(Type::IntegerType(4, false), "entsize");
    ivarList.AddMember(Type::IntegerType(4, false), "count");
    type = finalizeStructureBuilder(bv, ivarList, "objc_ivar_list_t");

}

}
