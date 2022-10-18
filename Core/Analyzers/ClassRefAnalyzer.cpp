#include "ClassRefAnalyzer.h"

#include "../../Constants.h"
#include "../ExceptionUtils.h"

#include <binaryninjaapi.h>

#include <exception>
#include <ios>
#include <sstream>

using namespace ObjectiveNinja;

ClassRefAnalyzer::ClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

AddressRefInfo ClassRefAnalyzer::analyzeClassRef(Address address) try
{
    return {address, m_file->readLong(address)};
} catch (...) {
    auto msg = std::ostringstream("ClassRefAnalyzer::analyzeClassRef(", std::ios_base::app);
    msg << std::hex << std::showbase << address << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

void ClassRefAnalyzer::run() try
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
            ExceptionUtils::forCurrentNested(ExceptionUtils::logDebugAction(*log, 1));
        }
    }
} catch (...) {
    std::throw_with_nested(std::runtime_error("ClassRefAnalyzer::run() failed"));
}
