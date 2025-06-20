/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "Constants.h"
#include "DataRenderers.h"
#include "Workflow.h"
#include "ArchitectureHooks.h"

extern "C" {

BN_DECLARE_CORE_ABI_VERSION

BINARYNINJAPLUGIN void CorePluginDependencies()
{
	BinaryNinja::AddOptionalPluginDependency("arch_x86");
	BinaryNinja::AddOptionalPluginDependency("arch_armv7");
	BinaryNinja::AddOptionalPluginDependency("arch_arm64");
}

BINARYNINJAPLUGIN bool CorePluginInit()
{
	RelativePointerDataRenderer::Register();

	Workflow::registerActivities();

	std::vector<BinaryNinja::Ref<BinaryNinja::Architecture>> targets = {
		BinaryNinja::Architecture::GetByName("aarch64"),
		BinaryNinja::Architecture::GetByName("x86_64")
	};
	for (auto& target : targets) {
		if (target)
		{
			auto* currentHook = new CFStringArchitectureHook(target);
			target->Register(currentHook);
		}
	}

	BinaryNinja::LogRegistry::CreateLogger(PluginLoggerName);

	auto settings = BinaryNinja::Settings::Instance();
	settings->RegisterSetting("analysis.objectiveC.resolveDynamicDispatch",
		R"({
		"title" : "Resolve Dynamic Dispatch Calls",
		"type" : "boolean",
		"default" : false,
		"aliases": ["core.function.objectiveC.assumeMessageSendTarget", "core.function.objectiveC.rewriteMessageSendTarget"],
		"description" : "Replaces objc_msgSend calls with direct calls to the first found implementation when the target method is visible. May produce false positives when multiple classes implement the same selector or when selectors conflict with system framework methods."
		})");

	return true;
}
}
