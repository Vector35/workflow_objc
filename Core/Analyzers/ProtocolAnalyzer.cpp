/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "ProtocolAnalyzer.h"

#include <array>
#include <functional>

using namespace ObjectiveNinja;

ProtocolAnalyzer::ProtocolAnalyzer(SharedAnalysisInfo info,
    SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

SharedProtocolInfo ProtocolAnalyzer::analyzeProtocol(Address address)
{
    if (m_info->protocolsByKey.count(address)) {
        return m_info->protocolsByKey[address];
    }

    auto pi = std::make_shared<ProtocolInfo>();
    pi->address = address;

    m_file->seek(pi->address);

    pi->isa.address = arp(m_file->readLong());
    pi->name.address = arp(m_file->readLong());
    pi->protocolList.address = arp(m_file->readLong());
    pi->instanceMethodList.address = arp(m_file->readLong());
    pi->classMethodList.address = arp(m_file->readLong());
    pi->optionalInstanceMethodList.address = arp(m_file->readLong());
    pi->optionalClassMethodList.address = arp(m_file->readLong());
    pi->propertyList.address = arp(m_file->readLong());
    pi->size = m_file->readInt();
    pi->flags = m_file->readInt();
    pi->extendedMethodTypeList.address = arp(m_file->readLong());
    pi->demangledName.address = arp(m_file->readLong());
    pi->classPropertyList.address = arp(m_file->readLong());

    pi->name.referenced = m_file->readStringAt(pi->name.address);
    if (pi->protocolList.address)
        pi->protocolList.referenced = analyzeProtocolList(pi->protocolList.address);
    // if (pi->instanceMethodList.address)
    //     pi->instanceMethodList = analyzeMethodList(pi->instanceMethodList.address);
    // if (pi->classMethodList.address)
    //     pi->classMethodList = analyzeMethodList(pi->classMethodList.address);
    // if (pi->optionalInstanceMethodList.address)
    //     pi->optionalInstanceMethodList = analyzeMethodList(pi->optionalInstanceMethodList.address);
    // if (pi->optionalClassMethodList.address)
    //     pi->optionalClassMethodList = analyzeMethodList(pi->optionalClassMethodList.address);
    if (pi->propertyList.address)
        pi->propertyList.referenced = analyzePropertyList(pi->propertyList.address);
    if (pi->extendedMethodTypeList.address) {
        auto listAddress = pi->extendedMethodTypeList.address;
        std::array<std::reference_wrapper<MethodListInfo>, 4> methodLists {
            pi->instanceMethodList.referenced, pi->classMethodList.referenced,
            pi->optionalInstanceMethodList.referenced, pi->optionalClassMethodList.referenced,
        };
        for (MethodListInfo& methodList : methodLists) {
            for (auto& mi : methodList.methods) {
                mi.extendedType.list.address = listAddress;
                mi.extendedType.entry.address = arp(m_file->readLong(mi.extendedType.list.address));
                mi.extendedType.entry.referenced = m_file->readStringAt(mi.extendedType.entry.address);
                listAddress += 0x8;
            }
        }
    }
    if (pi->demangledName.address)
        pi->demangledName.referenced = m_file->readStringAt(pi->demangledName.address);
    if (pi->classPropertyList.address)
        pi->classPropertyList.referenced = analyzePropertyList(pi->classPropertyList.address);

    m_info->protocolsByKey[pi->address] = pi;

    return pi;
}

ProtocolListInfo ProtocolAnalyzer::analyzeProtocolList(Address address)
{
    ProtocolListInfo pli;
    pli.address = address;

    auto protocolAddressCount = m_file->readLong(pli.address);
    auto protocolAddressSize = 0x8;

    for (auto i = 0; i < protocolAddressCount; ++i) {
        auto listPointer = pli.address + 0x8 + (i * protocolAddressSize);
        auto pi = analyzeProtocol(arp(m_file->readLong(listPointer)));
        m_info->protocols.emplace_back(0, pi);
        pli.protocols.emplace_back(listPointer, std::move(pi));
    }

    return pli;
}

PropertyListInfo ProtocolAnalyzer::analyzePropertyList(Address address) {
    PropertyListInfo pli;
    pli.address = address;
    pli.entsize = m_file->readShort(pli.address);
    pli.flags = m_file->readShort(pli.address + 0x2);

    auto propertyCount = m_file->readInt(pli.address + 0x4);
    auto propertySize = pli.entsize;

    for (unsigned i = 0; i < propertyCount; ++i) {
        PropertyInfo pi;
        pi.address = pli.address + 0x8 + (i * propertySize);

        m_file->seek(pi.address);

        if (pli.hasRelativeOffsets()) {
            pi.name.address = pi.address + static_cast<int32_t>(m_file->readInt());
            pi.attributes.address = pi.address + 0x4 + static_cast<int32_t>(m_file->readInt());
        } else {
            pi.name.address = arp(m_file->readLong());
            pi.attributes.address = arp(m_file->readLong());
        }

        pi.name.referenced = m_file->readStringAt(pi.name.address);
        pi.attributes.referenced = m_file->readStringAt(pi.attributes.address);

        m_info->propertiesByKey[pi.name.address] = pi.attributes;

        pli.properties.emplace_back(pi);
    }

    return pli;
}

void ProtocolAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_protolist");
    const auto sectionEnd = m_file->sectionEnd("__objc_protolist");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    for (auto address = sectionStart; address < sectionEnd; address += 8) {
        m_info->protocols.emplace_back(address, analyzeProtocol(arp(m_file->readLong(address))));
    }
}
