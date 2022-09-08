#pragma once

#include "../Analyzer.h"

namespace ObjectiveNinja {

/**
 * Analyzer for extracting Objective-C super-class information.
 */
class SuperClassRefAnalyzer : public Analyzer {

public:
    SuperClassRefAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

    void run() override;
};
}
