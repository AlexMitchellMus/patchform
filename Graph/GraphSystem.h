#pragma once

#include "GraphManager.h"
#include "DSPTimer.h"
#include <filesystem>
#include <atomic>
#include <memory>

class GraphSystem
{
public:
    using Graphs = std::vector<std::shared_ptr<GraphManager>>;

    std::tuple<std::vector<Object*>, std::vector<Edge*>> loadPatch(const std::string& path, const json& patch, bool logVerbose)
    {
        cleanupDeletedGraphs();

        auto manager = std::make_shared<GraphManager>(sampleRate, frameCount);
        auto* ptr = manager.get();

        const auto [objects, conns] = ptr->setActiveGraph(path, patch, logVerbose);
        if (!ptr->wasPatchLoadSuccessful())
            return {};

        graphManagersUI.push_back(manager);
        activeGraph = ptr;

        graphListNeedsSwap.store(true, std::memory_order_release);
        onPatchLoaded(graphManagersUI);

        return {objects, conns};
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
            if (graphManagersUI[i].get() == activeGraph)
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
            graphManagersAudio = std::make_shared<Graphs>(graphManagersUI);
            graphListNeedsSwap.store(false, std::memory_order_release);
        }

        dspTimer.start();

        for (auto& mgr : *graphManagersAudio)
        {
            // FIXME: We don't want to check each graph if it's valid, however it's only for a small amount of loaded patches (hopefully)
            if (!mgr->flaggedForDeletion.load())
                mgr->process(inBuffer, outBuffer, frameCount, midi);
        }

        dspTimer.end(frameCount, sampleRate);
        processPeak(outBuffer, frameCount);
    }

    std::function<void(Graphs&)> onPatchLoaded = [](Graphs&) {};

    std::vector<std::string> getLoadedPatches() const
    {
        std::vector<std::string> paths;
        for (const auto& mgr : graphManagersUI)
        {
            if (!mgr->flaggedForDeletion.load())
                paths.push_back(mgr->getPatchFile());
        }
        return paths;
    }

    GraphManager* getActiveGraph() const { return activeGraph; }

    std::tuple<std::vector<Object*>, std::vector<Edge*>> getGraphDump(const std::string& path)
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
    }

    void setActiveGraph(const std::string& path)
    {
        for (const auto& mgr : graphManagersUI)
        {
            if (mgr->getPatchFile() == path)
            {
                activeGraph = mgr.get();
                return;
            }
        }
        std::cerr << "setActiveGraph: No graph found for path: " << path << std::endl;
    }

    float getDspTiming() const
    {
        return dspTimer.getCpuUsage();
    }

    moodycamel::ConcurrentQueue<std::vector<float>> volumeMeterQueue = moodycamel::ConcurrentQueue<std::vector<float>>(100);

private:
    void processPeak(const float* buffer, unsigned long frameCount)
    {
        constexpr int kUpdateInterval = 4;
        const float* right = buffer + frameCount;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            accumulatedPeakL = std::max(accumulatedPeakL, std::abs(buffer[i]));
            accumulatedPeakR = std::max(accumulatedPeakR, std::abs(right[i]));
        }

        if (++peakFrameCounter >= kUpdateInterval)
        {
            peakFrameCounter = 0;
            volumeMeterQueue.enqueue({accumulatedPeakL, accumulatedPeakR});
            accumulatedPeakL = accumulatedPeakR = 0.0f;
        }
    }

    DspTimer dspTimer;

    int peakFrameCounter = 0;
    float accumulatedPeakL = 0.0f;
    float accumulatedPeakR = 0.0f;

    Graphs graphManagersUI;
    std::shared_ptr<const Graphs> graphManagersAudio = std::make_shared<Graphs>();
    std::atomic<bool> graphListNeedsSwap = false;

    GraphManager* activeGraph = nullptr;

    int sampleRate = 44100;
    unsigned long frameCount = 64;
};
