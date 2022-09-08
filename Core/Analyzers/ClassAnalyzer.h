/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "../Analyzer.h"

namespace ObjectiveNinja {

/**
 * Analyzer for extracting Objective-C class information.
 */
class ClassAnalyzer : public Analyzer {
    /**
     * Analyze a class.
     */
    ClassInfo analyzeClass(Address);

    /**
     * Analyze a method.
     */
    MethodInfo analyzeMethod(Address, bool hasRelativeOffsets, bool hasDirectSelectors);

    /**
     * Analyze a method list.
     */
    MethodListInfo analyzeMethodList(Address);

public:
    ClassAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

    void run() override;
};

}
