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
 * Analyzer for extracting Objective-C protocol information.
 */
class ProtocolAnalyzer : public Analyzer {
    /**
     * Analyze a protocol.
     */
    SharedProtocolInfo analyzeProtocol(Address);

    /**
     * Analyze a protocol list.
     */
    ProtocolListInfo analyzeProtocolList(Address);

    /**
     * Analyze a property list.
     */
    PropertyListInfo analyzePropertyList(Address);

public:
    ProtocolAnalyzer(SharedAnalysisInfo, SharedAbstractFile);

    void run() override;
};

}
