#include "ClassRefAnalyzer.h"

#include "../../Constants.h"

#include <binaryninjaapi.h>

using namespace ObjectiveNinja;

ClassRefAnalyzer::ClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

AddressRefInfo ClassRefAnalyzer::analyzeClassRef(Address address)
{
    return {address, m_file->readLong(address)};
}

void ClassRefAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_classrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_classrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);

    // TODO: Dynamic Address size for armv7
    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        try {
            m_info->classRefs.emplace_back(analyzeClassRef(address));
        } catch (...) {
            log->LogWarn("Class ref analysis at %#x failed; skipping.", address);
        }
    }
}
