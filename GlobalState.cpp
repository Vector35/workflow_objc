/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#include "GlobalState.h"

#include <set>
#include <unordered_map>

static std::unordered_map<BinaryViewID, MessageHandler*> g_messageHandlers;
static std::unordered_map<BinaryViewID, SharedAnalysisInfo> g_viewInfos;
static std::set<BinaryViewID> g_ignoredViews;

MessageHandler* GlobalState::messageHandler(BinaryViewRef bv)
{
    if (auto messageHandler = g_messageHandlers.find(id(bv)); messageHandler != g_messageHandlers.end()) {
        return messageHandler->second;
    } else {
        auto newMessageHandler = new MessageHandler(bv);
        g_messageHandlers[id(bv)] = newMessageHandler;
        return newMessageHandler;
    }
}

BinaryViewID GlobalState::id(BinaryViewRef bv)
{
    return bv->GetFile()->GetSessionId();
}

void GlobalState::addIgnoredView(BinaryViewRef bv)
{
    g_ignoredViews.insert(id(std::move(bv)));
}

bool GlobalState::viewIsIgnored(BinaryViewRef bv)
{
    return g_ignoredViews.count(id(std::move(bv))) > 0;
}

SharedAnalysisInfo GlobalState::analysisInfo(BinaryViewRef data)
{
    if (const auto& it = g_viewInfos.find(id(data)); it != g_viewInfos.end())
        if (data->GetStart() == it->second->imageBase)
            return it->second;

    SharedAnalysisInfo info = std::make_shared<AnalysisInfo>();

    if (auto objcStubs = data->GetSectionByName("__objc_stubs"))
    {
        info->objcStubsStartEnd = {objcStubs->GetStart(), objcStubs->GetEnd()};
        info->hasObjcStubs = true;
    }

    auto meta = data->QueryMetadata("Objective-C");
    if (!meta)
        return info;

    auto metaKVS = meta->GetKeyValueStore();
    if (metaKVS["version"]->GetUnsignedInteger() != 1)
    {
        BinaryNinja::LogError("workflow_objc: Invalid metadata version received!");
        return info;
    }
    info->imageBase = data->GetStart();
    for (const auto& selAndImps : metaKVS["selRefImplementations"]->GetArray())
        info->selRefToImp[selAndImps->GetArray()[0]->GetUnsignedInteger()] = selAndImps->GetArray()[1]->GetUnsignedIntegerList();
    for (const auto& selAndImps : metaKVS["selImplementations"]->GetArray())
        info->selToImp[selAndImps->GetArray()[0]->GetUnsignedInteger()] = selAndImps->GetArray()[1]->GetUnsignedIntegerList();

    g_viewInfos[id(data)] = info;
    return info;
}

bool GlobalState::hasAnalysisInfo(BinaryViewRef data)
{
    return data->QueryMetadata("Objective-C") != nullptr;
}

bool GlobalState::hasFlag(BinaryViewRef bv, const std::string& flag)
{
    return bv->QueryMetadata(flag);
}

void GlobalState::setFlag(BinaryViewRef bv, const std::string& flag)
{
    bv->StoreMetadata(flag, new BinaryNinja::Metadata("YES"));
}
