#include "SuperClassRefAnalyzer.h"

using namespace ObjectiveNinja;

SuperClassRefAnalyzer::SuperClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

void SuperClassRefAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_superrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_superrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    // TODO: Dynamic Address size for armv7
    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        m_info->superClassRefs.push_back({ address, m_file->readLong(address) });
    }
}
