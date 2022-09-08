#pragma once

#include "../Analyzer.h"

namespace ObjectiveNinja {

/**
 * Analyzer for extracting Objective-C super-class reference information.
 */
class SuperClassRefAnalyzer : public Analyzer {
    /**
     * Analyze a super-class reference.
     */
     AddressRefInfo analyzeSuperClassRef(Address);

public:
    SuperClassRefAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

    void run() override;
};
}
