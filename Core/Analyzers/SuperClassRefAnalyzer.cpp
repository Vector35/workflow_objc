#include "SuperClassRefAnalyzer.h"

#include "../../Constants.h"
#include "../ExceptionUtils.h"

#include <binaryninjaapi.h>

#include <exception>
#include <ios>
#include <sstream>

using namespace ObjectiveNinja;

SuperClassRefAnalyzer::SuperClassRefAnalyzer(SharedAnalysisInfo info, SharedAbstractFile file)
    : Analyzer(std::move(info), std::move(file))
{
}

AddressRefInfo SuperClassRefAnalyzer::analyzeSuperClassRef(Address address) try
{
    return {address, m_file->readLong(address)};
} catch (...) {
    auto msg = std::ostringstream("SuperClassRefAnalyzer::analyzeSuperClassRef(", std::ios_base::app);
    msg << std::hex << std::showbase << address << ") failed";
    std::throw_with_nested(std::runtime_error(msg.str()));
}

void SuperClassRefAnalyzer::run() try
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
            ExceptionUtils::forCurrentNested(ExceptionUtils::logDebugAction(*log, 1));
        }
    }
} catch (...) {
    std::throw_with_nested(std::runtime_error("SuperClassRefAnalyzer::run() failed"));
}
