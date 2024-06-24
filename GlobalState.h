/*
 * Copyright (c) 2022-2023 Jon Palmisciano. All rights reserved.
 *
 * Use of this source code is governed by the BSD 3-Clause license; the full
 * terms of the license can be found in the LICENSE.txt file.
 */

#pragma once

#include <condition_variable>
#include "BinaryNinja.h"

#include "MessageHandler.h"

/**
 * Namespace to hold metadata flag key constants.
 */
namespace Flag {

constexpr auto DidRunWorkflow = "objectiveNinja.didRunWorkflow";
constexpr auto DidRunStructureAnalysis = "objectiveNinja.didRunStructureAnalysis";

}

struct AnalysisInfo {
    std::uint64_t imageBase;
    bool hasObjcStubs = false;
    std::pair<uint64_t, uint64_t> objcStubsStartEnd;
    std::unordered_map<uint64_t, std::vector<uint64_t>> selRefToImp;
    std::unordered_map<uint64_t, std::vector<uint64_t>> selToImp;
};

typedef std::shared_ptr<AnalysisInfo> SharedAnalysisInfo;

class ReadWriteLock
{
public:
    void lockRead()
    {
        std::unique_lock<std::mutex> lock(mutex);
        readersCond.wait(lock, [this]() { return !writerActive; });
        ++readersActive;
    }

    void unlockRead()
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (--readersActive == 0)
            writersCond.notify_one();
    }

    void lockWrite()
    {
        std::unique_lock<std::mutex> lock(mutex);
        writersCond.wait(lock, [this]() { return !writerActive && readersActive == 0; });
        writerActive = true;
    }

    void unlockWrite()
    {
        std::unique_lock<std::mutex> lock(mutex);
        writerActive = false;
        writersCond.notify_one();
        readersCond.notify_all();
    }

private:
    std::mutex mutex;
    std::condition_variable readersCond;
    std::condition_variable writersCond;
    int readersActive = 0;
    bool writerActive = false;
};

class ReaderLock {
public:
    ReaderLock(ReadWriteLock& lock) : lock(lock) { lock.lockRead(); }
    ~ReaderLock() { lock.unlockRead(); }

private:
    ReadWriteLock& lock;
};

class WriterLock {
public:
    WriterLock(ReadWriteLock& lock) : lock(lock) { lock.lockWrite(); }
    ~WriterLock() { lock.unlockWrite(); }

private:
        ReadWriteLock& lock;
};

/**
 * Global state/storage interface.
 */
class GlobalState {
    /**
     * Get the ID for a view.
     */
    static BinaryViewID id(BinaryViewRef);

public:
    /**
     * Get the analysis info for a view.
     */
    static SharedAnalysisInfo analysisInfo(BinaryViewRef);

    /**
     * Get ObjC Message Handler for a view
     */
    static MessageHandler* messageHandler(BinaryViewRef);

    /**
     * Check if analysis info exists for a view.
     */
    static bool hasAnalysisInfo(BinaryViewRef);

    /**
     * Add a view to the list of ignored views.
     */
    static void addIgnoredView(BinaryViewRef);

    /**
     * Check if a view is ignored.
     */
    static bool viewIsIgnored(BinaryViewRef);

    /**
     * Check if the a metadata flag is present for a view.
     */
    static bool hasFlag(BinaryViewRef, const std::string&);

    /**
     * Set a metadata flag for a view.
     */
    static void setFlag(BinaryViewRef, const std::string&);
};
