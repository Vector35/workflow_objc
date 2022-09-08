/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "ClassAnalyzer.h"

#include <array>
#include <functional>

using namespace ObjectiveNinja;

ClassAnalyzer::ClassAnalyzer(SharedAnalysisInfo info,
    SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

ClassInfo ClassAnalyzer::analyzeClass(Address address)
{
    auto ci = ClassInfo {};
    ci.address = address;
    ci.data.address = arp(m_file->readLong(ci.address + 0x20));

    // Sometimes the lower two bits of the data address are used as flags
    // for Swift/Objective-C classes. They should be ignored, unless you
    // want incorrect analysis...
    ci.data.address &= ~ABI::FastPointerDataMask;

    ci.name.address = arp(m_file->readLong(ci.data.address + 0x18));
    ci.name.referenced = m_file->readStringAt(ci.name.address);

    ci.methodList.address = arp(m_file->readLong(ci.data.address + 0x20));
    if (ci.methodList.address)
        ci.methodList.referenced = analyzeMethodList(ci.methodList.address);

    return ci;
}

MethodInfo ClassAnalyzer::analyzeMethod(Address address, bool hasRelativeOffsets, bool hasDirectSelectors)
{
    auto mi = MethodInfo {};
    mi.address = address;

    m_file->seek(mi.address);

    if (hasRelativeOffsets) {
        mi.selectorName.address = mi.address + static_cast<int32_t>(m_file->readInt());
        mi.type.address = mi.address + 0x4 + static_cast<int32_t>(m_file->readInt());
        mi.impl.address = mi.address + 0x8 + static_cast<int32_t>(m_file->readInt());
    } else {
        mi.selectorName.address = arp(m_file->readLong());
        mi.type.address = arp(m_file->readLong());
        mi.impl.address = arp(m_file->readLong());
    }

    if (!hasRelativeOffsets || hasDirectSelectors) {
        mi.selectorName.referenced = m_file->readStringAt(mi.selectorName.address);
    } else {
        auto selectorNamePointer = arp(m_file->readLong(mi.selectorName.address));
        mi.selectorName.referenced = m_file->readStringAt(selectorNamePointer);
    }

    mi.type.referenced = m_file->readStringAt(mi.type.address);

    m_info->methodImpls[mi.selectorName.address] = mi.impl;

    return mi;
}

MethodListInfo ClassAnalyzer::analyzeMethodList(Address address)
{
    auto mli = MethodListInfo {};
    mli.address = address;
    mli.entsize = m_file->readShort(mli.address);
    mli.flags = m_file->readShort(mli.address + 0x2);

    auto methodCount = m_file->readInt(mli.address + 0x4);
    auto methodSize = mli.entsize;

    for (unsigned i = 0; i < methodCount; ++i) {

        mli.methods.emplace_back(analyzeMethod(mli.address + 0x8 + (i * methodSize), mli.hasRelativeOffsets(), mli.hasDirectSelectors()));
    }

    return mli;
}

void ClassAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_classlist");
    const auto sectionEnd = m_file->sectionEnd("__objc_classlist");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    for (auto address = sectionStart; address < sectionEnd; address += 8) {
        m_info->classes.emplace_back(address, analyzeClass(arp(m_file->readLong(address))));
    }
}
