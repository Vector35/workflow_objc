#include "SuperClassRefAnalyzer.h"

#include "../../Constants.h"

#include <binaryninjaapi.h>

using namespace ObjectiveNinja;

SuperClassRefAnalyzer::SuperClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

AddressRefInfo SuperClassRefAnalyzer::analyzeSuperClassRef(Address address)
{
    return {address, m_file->readLong(address)};
}

void SuperClassRefAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_superrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_superrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);

    // TODO: Dynamic Address size for armv7
    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        try {
            m_info->superClassRefs.emplace_back(analyzeSuperClassRef(address));
        } catch (...) {
            log->LogWarn("Super-class ref analysis at %#x failed; skipping.", address);
        }
    }
}
