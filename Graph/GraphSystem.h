/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "GraphManager.h"
#include "Utility/LinearSmoother.h"
#include "VolumeMeter.h"

#include "DSPTimer.h"
#include <atomic>
#include <memory>

class GraphSystem
{
public:
    using Graphs = std::vector<std::shared_ptr<GraphManager>>;

    GraphSystem()
    {
        mainGraphVolumeMeter = std::make_unique<VolumeMeter>(44100, 64, 2);
        setSampleRateAndBlockSize(44100, 64);
    }

    void clear()
    {
    }

    std::tuple<std::vector<Object*>, std::vector<Edge*>> loadPatch(const std::string& path, const json& patch, bool logVerbose)
    {
        cleanupDeletedGraphs();

        const auto manager = std::make_shared<GraphManager>(sampleRate, frameCount);
        auto* ptr = manager.get();

        const auto [objects, conns] = ptr->loadGraph(path, patch, logVerbose);
        if (!ptr->wasPatchLoadSuccessful())
            return {};

        graphManagersUI.push_back(manager);
        activeGraph = ptr;

        graphManagersPending = std::make_shared<Graphs>(graphManagersUI);
        graphListNeedsSwap.store(true, std::memory_order_release);
        onPatchLoaded(graphManagersUI);

        return {objects, conns};
    }

    void closeAll()
    {
        // Flag all for deletion
        for (auto& mgr : graphManagersUI)
            mgr->flaggedForDeletion.store(true);

        // Replace with empty list
        graphManagersPending = std::make_shared<Graphs>();
        graphListNeedsSwap.store(true, std::memory_order_release);

        // Wait for audio thread to acknowledge the swap
        while (graphListNeedsSwap.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        cleanupDeletedGraphs();

        graphManagersPending.reset();

        activeGraph = nullptr;
    }

    bool unloadActivePatch()
    {
        // TODO: Allow empty patch state in future (with welcome panel?)
        const int activePatches = static_cast<int>(std::ranges::count_if(graphManagersUI, [](const auto& gm) { return !gm->flaggedForDeletion.load(); }));

        if (activePatches <= 1)
            return false; // Don’t allow unloading the last remaining patch

        int indexToRemove = -1;

        for (size_t i = 0; i < graphManagersUI.size(); ++i)
        {
            if (graphManagersUI[i].get() == getActiveRootGraph())
            {
                graphManagersUI[i]->flaggedForDeletion.store(true);
                indexToRemove = static_cast<int>(i);
                break;
            }
        }

        if (indexToRemove != -1)
        {
            // Try the one before first
            for (int i = indexToRemove - 1; i >= 0; --i)
            {
                if (!graphManagersUI[i]->flaggedForDeletion.load())
                {
                    activeGraph = graphManagersUI[i].get();
                    graphListNeedsSwap.store(true, std::memory_order_release);
                    return true;
                }
            }

            // Then try the one after
            for (size_t i = indexToRemove + 1; i < graphManagersUI.size(); ++i)
            {
                if (!graphManagersUI[i]->flaggedForDeletion.load())
                {
                    activeGraph = graphManagersUI[i].get();
                    graphListNeedsSwap.store(true, std::memory_order_release);
                    return true;
                }
            }
        }
        return false;
    }

    bool graphSwapPending() const
    {
        return graphListNeedsSwap.load(std::memory_order_acquire);
    }

    void cleanupDeletedGraphs()
    {
        std::erase_if(graphManagersUI, [](const auto& mgr) {
            return mgr->flaggedForDeletion.load();
        });
    }

    void processAll(const float* inBuffer, float* outBuffer, unsigned long frameCount, std::vector<MidiMessage>& midi)
    {
        if (graphListNeedsSwap.load(std::memory_order_acquire))
        {
            std::swap(graphManagersAudio, graphManagersPending);
            graphListNeedsSwap.store(false, std::memory_order_release);
        }

        dspTimer.start();

        for (auto& mgr : *graphManagersAudio)
        {
            // FIXME: We don't want to check each graph if it's valid, however it's only for a small amount of loaded patches (hopefully)
            if (!mgr->flaggedForDeletion.load())
            {
                mgr->process(inBuffer, outBuffer, frameCount, midi);
            }
        }

        const float currentVolume = mainVolume.load(std::memory_order_relaxed);
        std::ranges::fill(gainBuffer, currentVolume);

        mainVolumeControlSmoothed.process(gainBuffer.data(), gainBuffer.data(), frameCount, true);

        for (int i = 0; i < frameCount; ++i) {
            outBuffer[i] *= gainBuffer[i];
            outBuffer[i + frameCount] *= gainBuffer[i];
        }

        dspTimer.end(frameCount, sampleRate);
        processPeak(outBuffer, frameCount);
    }

    std::function<void(Graphs&)> onPatchLoaded = [](Graphs&) {};

    std::vector<std::tuple<std::string, bool>> getLoadedPatches() const
    {
        std::vector<std::tuple<std::string, bool>> paths;
        for (const auto& mgr : graphManagersUI)
        {
            if (!mgr->flaggedForDeletion.load())
            {
                paths.emplace_back(mgr->getPatchFile(), mgr->getIsGraphDirty());
            }
        }
        return paths;
    }

    GraphManager* getActiveGraph() const { return activeGraph; }

    GraphManager* getActiveRootGraph() const
    {
        auto graphManager = activeGraph;

        while (graphManager->parentGraph)
            graphManager = graphManager->parentGraph;

        return graphManager;
    }

    std::tuple<std::vector<Object*>, std::vector<Edge*>> getGraphDump(const std::string& path) const
    {
        for (auto& mgr : graphManagersUI)
        {
            if (mgr->getPatchFile() == path)
                return {mgr->getActiveObjects(), mgr->getConnections()};
        }
        return {};
    }

    void setSampleRateAndBlockSize(int sr, unsigned long bs)
    {
        sampleRate = sr;
        frameCount = bs;

        mainVolumeControlSmoothed.setSampleRate(sr);
        mainVolumeControlSmoothed.setSmoothTime(0.03f);
        gainBuffer.assign(bs, 0.0f);
        mainVolumeControlSmoothed.clear(1.0f);

        mainGraphVolumeMeter->updateFrameSize(sr, bs, 2);
    }

    void setActiveGraph(GraphManager* manager)
    {
        if (!manager)
            return;

        activeGraph = manager;
    }

    void setActiveGraph(const std::string& path)
    {
        for (const auto& mgr : graphManagersUI)
        {
            if (mgr->getPatchFile() == path)
            {
                activeGraph = mgr.get();
                graphManagersPending = std::make_shared<Graphs>(graphManagersUI);
                graphListNeedsSwap.store(true, std::memory_order_release);
                return;
            }
        }
        std::cerr << "setActiveGraph: No graph found for path: " << path << std::endl;
    }

    float getDspTiming() const
    {
        return dspTimer.getCpuUsage();
    }

    // rmsL, peakL, peakHoldL, rmsR, peakR, peakHoldR
    moodycamel::ConcurrentQueue<std::array<float, 6>> volumeMeterQueue = moodycamel::ConcurrentQueue<std::array<float, 6>>(100);

    std::atomic<float> mainVolume = 1.0f;

private:
    LinearSmoother mainVolumeControlSmoothed;
    std::vector<float, AlignedAllocator<float, 16>> gainBuffer;

    std::unique_ptr<VolumeMeter> mainGraphVolumeMeter;

    void processPeak(const float* buffer, const unsigned long frameCount)
    {
        mainGraphVolumeMeter->process(buffer, frameCount);

        if (mainGraphVolumeMeter->shouldEmit())
        {
            float rmsL, peakL, peakHoldL, rmsR, peakR, peakHoldR;
            mainGraphVolumeMeter->getAllValues(rmsL, peakL, peakHoldL, rmsR, peakR, peakHoldR);
            volumeMeterQueue.enqueue({rmsL, peakL, peakHoldL, rmsR, peakR, peakHoldR});

            mainGraphVolumeMeter->resetEmit();
        }
    }

    DspTimer dspTimer;

    int peakFrameCounter = 0;

    Graphs graphManagersUI;
    std::shared_ptr<const Graphs> graphManagersAudio = std::make_shared<Graphs>();
    std::shared_ptr<const Graphs> graphManagersPending;
    std::atomic<bool> graphListNeedsSwap = false;

    GraphManager* activeGraph = nullptr;

    int sampleRate = 44100;
    unsigned long frameCount = 64;
};
