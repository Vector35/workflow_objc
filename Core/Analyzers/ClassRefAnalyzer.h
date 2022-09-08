#pragma once

#include "../Analyzer.h"

namespace ObjectiveNinja {

/**
 * Analyzer for extracting Objective-C class reference information.
 */
class ClassRefAnalyzer : public Analyzer {
    /**
     * Analyze a class reference.
     */
     AddressRefInfo analyzeClassRef(Address);

public:
    ClassRefAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

    void run() override;
};
}
