/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "SelectorAnalyzer.h"

#include "../../Constants.h"
#include "../ExceptionUtils.h"

#include <binaryninjaapi.h>

#include <exception>
#include <ios>
#include <sstream>

using namespace ObjectiveNinja;

SelectorAnalyzer::SelectorAnalyzer(SharedAnalysisInfo info,
    SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

SelectorNameInfo SelectorAnalyzer::analyzeSelectorName(Address address) try
{
    return {address, m_file->readStringAt(address)};
} catch (...) {
    auto msg = std::ostringstream("SelectorAnalyzer::analyzeSelectorName(", std::ios_base::app);
    msg << std::hex << std::showbase << address << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

void SelectorAnalyzer::run() try
{
    const auto sectionStart = m_file->sectionStart("__objc_selrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_selrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);

    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        try {
            auto unresolvedSelectorNameAddress = m_file->readLong(address);
            auto selectorNameAddress = arp(unresolvedSelectorNameAddress);
            try {
                auto sri = std::make_shared<SelectorRefInfo>(address, unresolvedSelectorNameAddress, SelectorAnalyzer::analyzeSelectorName(selectorNameAddress));

                m_info->selectorRefs.emplace_back(sri);

                m_info->selectorRefsByKey[sri->referenced.unresolvedAddress] = sri;
                m_info->selectorRefsByKey[sri->address] = sri;
            } catch (...) {
                log->LogWarn("Selector analysis at %#x (%#x) failed; skipping.", address, selectorNameAddress);
                ExceptionUtils::forCurrentNested(ExceptionUtils::logDebugAction(*log, 1));
            }
        } catch (...) {
            log->LogWarn("Selector analysis at %#x failed; skipping.", address);
            ExceptionUtils::forCurrentNested(ExceptionUtils::logDebugAction(*log, 1));
        }
    }
} catch (...) {
    std::throw_with_nested(std::runtime_error("SelectorAnalyzer::run() failed"));
}
