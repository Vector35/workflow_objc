#include "ClassRefAnalyzer.h"

using namespace ObjectiveNinja;

ClassRefAnalyzer::ClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

void ClassRefAnalyzer::run()
{
    const auto sectionStart = m_file->sectionStart("__objc_classrefs");
    const auto sectionEnd = m_file->sectionEnd("__objc_classrefs");
    if (sectionStart == 0 || sectionEnd == 0)
        return;

    // TODO: Dynamic Address size for armv7
    for (auto address = sectionStart; address < sectionEnd; address += 0x8) {
        m_info->classRefs.push_back({ address, m_file->readLong(address) });
    }
}