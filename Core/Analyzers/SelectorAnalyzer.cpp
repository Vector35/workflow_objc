/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "SelectorAnalyzer.h"

using namespace ObjectiveNinja;

SelectorAnalyzer::SelectorAnalyzer(SharedAnalysisInfo info,
    SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

SelectorNameInfo SelectorAnalyzer::analyzeSelectorName(Address address)
{
    return {address, m_file->readStringAt(address)};
}

void SelectorAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_selrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_selrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        auto unresolvedSelectorNameAddress = m_file->readLong(address);
        auto sri = std::make_shared<SelectorRefInfo>(address, unresolvedSelectorNameAddress, SelectorAnalyzer::analyzeSelectorName(arp(unresolvedSelectorNameAddress)));

        m_info->selectorRefs.emplace_back(sri);

        m_info->selectorRefsByKey[sri->referenced.unresolvedAddress] = sri;
        m_info->selectorRefsByKey[sri->address] = sri;
    }
}
