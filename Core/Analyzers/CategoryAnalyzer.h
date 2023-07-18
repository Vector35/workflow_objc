/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include "../Analyzer.h"

namespace ObjectiveNinja {

/**
 * Analyzer for extracting Objective-C category information.
 */
    class CategoryAnalyzer : public Analyzer {
	/**
	 * Analyze a method list.
	 */
	MethodListInfo analyzeMethodList(uint64_t);

    public:
	CategoryAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

	void run() override;
    };

}
