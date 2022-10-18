/*
 * Copyright (c) 2022 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "Commands.h"

#include "Constants.h"
#include "CustomTypes.h"
#include "GlobalState.h"
#include "InfoHandler.h"
#include "Performance.h"

#include "Core/AnalysisProvider.h"
#include "Core/BinaryViewFile.h"
#include "Core/ExceptionUtils.h"

void Commands::defineTypes(BinaryViewRef bv)
{
    CustomTypes::defineAll(std::move(bv));
}

void Commands::analyzeStructures(BinaryViewRef bv)
{
    if (GlobalState::hasFlag(bv, Flag::DidRunWorkflow)
        || GlobalState::hasFlag(bv, Flag::DidRunStructureAnalysis)) {
        auto result = BinaryNinja::ShowMessageBox("Error",
            "Structure analysis has already been performed on this binary. "
            "Repeated analysis may cause unexpected behavior.* Continue?\n\n"
            "*If you undid analysis, this message can be safely ignored.",
            BNMessageBoxButtonSet::YesNoButtonSet,
            BNMessageBoxIcon::QuestionIcon);

        if (result != BNMessageBoxButtonResult::YesButton)
            return;
    }

    SharedAnalysisInfo info;
    CustomTypes::defineAll(bv);

    try {
        auto file = std::make_shared<ObjectiveNinja::BinaryViewFile>(bv);

        auto start = Performance::now();
        info = ObjectiveNinja::AnalysisProvider::infoForFile(file);
        auto elapsed = Performance::elapsed<std::chrono::milliseconds>(start);

        const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);
        log->LogInfo("Structures analyzed in %lu ms", elapsed.count());

        InfoHandler::applyInfoToView(info, bv);
    } catch (...) {
        const auto log = BinaryNinja::LogRegistry::GetLogger(PluginLoggerName);
        log->LogError("Structure analysis failed; binary may be malformed.");
        ObjectiveNinja::ExceptionUtils::forCurrentNested(
            ObjectiveNinja::ExceptionUtils::logDebugAction(*log, 1));
        log->LogError("Objective-C analysis will not be applied due to previous errors.");
    }

    GlobalState::setFlag(bv, Flag::DidRunWorkflow);
}

void Commands::registerCommands()
{
    BinaryNinja::PluginCommand::Register("Objective-C \\ Define Types",
        "", Commands::defineTypes);
    BinaryNinja::PluginCommand::Register("Objective-C \\ Analyze Structures",
        "", Commands::analyzeStructures);
}
