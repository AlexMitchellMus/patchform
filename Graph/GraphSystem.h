#pragma once

#include "GraphManager.h"
#include "DSPTimer.h"

#include <filesystem>
using namespace std::filesystem;

class GraphSystem
{
public:
    using Graphs = std::vector<std::unique_ptr<GraphManager>>;

    // Load a patch and add a new graph to the system
    std::tuple<std::vector<Object*>, std::vector<Edge*>> loadPatch(const std::string& path, const json& patch, bool logVerbose)
    {
        auto manager = std::make_unique<GraphManager>(sampleRate, frameCount);
        auto* ptr = manager.get();

        const auto [objects, conns] = ptr->setActiveGraph(path, patch, logVerbose);
        if (!ptr->wasPatchLoadSuccessful())
            return { };

        graphManagers.push_back(std::move(manager));

        activeGraph = graphManagers.back().get();

        onPatchLoaded(graphManagers);

        return {objects, conns};
    }

    void setActiveGraph(const std::string& path)
    {
        for (const auto& mgr : graphManagers)
        {
            if (mgr->getPatchFile() == path)
            {
                activeGraph = mgr.get();
                return;
            }
        }
        std::cerr << "setActiveGraph: No graph found for path: " << path << std::endl;
    }

    GraphManager* getActiveGraph() const { return activeGraph; }

    void unloadActivePatch()
    {
        for (auto& mgr : graphManagers)
        {
            if (mgr.get() == activeGraph)
                mgr->flaggedForDeletion.store(true);
        }
    }

    void processAll(const float* inBuffer, float* outBuffer, unsigned long frameCount, std::vector<MidiMessage>& midi)
    {
        dspTimer.start();

        for (auto& mgr : graphManagers)
        {
            if (!mgr->flaggedForDeletion.load())
                mgr->process(inBuffer, outBuffer, frameCount, midi);
        }

        dspTimer.end(frameCount, sampleRate);

        processPeak(outBuffer, frameCount);
    }

    void setSampleRateAndBlockSize(int sr, unsigned long bs)
    {
        sampleRate = sr;
        frameCount = bs;
    }

    std::function<void(Graphs&)> onPatchLoaded = [](Graphs&){};

    std::vector<std::string> getLoadedPatches() const
    {
        std::vector<std::string> paths;
        for (const auto& mgr : graphManagers)
            paths.push_back(mgr->getPatchFile());
        return paths;
    }

    std::tuple<std::vector<Object*>, std::vector<Edge*>> getGraphDump(const std::string& path)
    {
        for (auto& mgr : graphManagers)
        {
            if (mgr->getPatchFile() == path)
            {
                return { mgr->getActiveObjects(), mgr->getConnections() };
            }
        }
        return {};
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
            volumeMeterQueue.enqueue(std::vector<float>{accumulatedPeakL, accumulatedPeakR});
            accumulatedPeakL = accumulatedPeakR = 0.0f;
        }
    }

    DspTimer dspTimer;

    int peakFrameCounter = 0;
    float accumulatedPeakL = 0.0f;
    float accumulatedPeakR = 0.0f;

    Graphs graphManagers;
    GraphManager* activeGraph = nullptr;

    int sampleRate = 44100;
    unsigned long frameCount = 64;
};
