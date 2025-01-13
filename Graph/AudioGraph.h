/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <limits>
#include <iostream>
#include <stack>
#include <chrono>
#include <queue>

#include "json.hpp"
using json = nlohmann::json;

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

#include "Logger.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
private:
    std::vector<std::unique_ptr<AudioNode>> nodes;
    std::vector<AudioNode*> sortedNodes;

    NodeContext* context;

    // Custom hash function for std::pair<int, int>
    struct PairHash {
        std::size_t operator()(const std::pair<int, int>& p) const noexcept {
            return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
        }
    };

    // Custom Adjacency List definition using the custom hash
    using AdjacencyList = std::unordered_map<std::pair<int, int>, std::vector<std::pair<int, int>>, PairHash>;
    using InputDependencyMap = std::unordered_map<std::pair<int, int>, int, PairHash>; // Tracks in-degree of (node, inputPort)

public:
    AudioGraph(NodeContext* context) : context(context)
    {
    }

    void loadPatch(const json& patch)
    {
        // Create nodes
        for (const auto& node : patch["nodes"]) {
            addObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"]) {
            connect(connection["sourceNode"], connection["sourcePort"], connection["targetNode"], connection["targetPort"]);
        }

        sortNodes();
    }

    template <typename NodeType, typename... Args>
    void addNode(Args&&... args) {
        auto nodeIndex = nodes.size();
        auto node = std::make_unique<NodeType>(context, std::forward<Args>(args)...);

        // // Retrieve and store ports in the adjacency list
        for (int portNum = 0; portNum < node->getNumOutputs(); portNum++) {
            adjacencyList[{nodeIndex, portNum}] = {}; // Initialize input port
        }
        for (int portNum = 0; portNum < node->getNumInputs(); portNum++) {
            inputDependencyMap[{nodeIndex, portNum}] = {}; // Initialize output port
        }

        nodes.push_back(std::move(node));
    };

    bool addObject(json node)
    {
        auto const object = node["type"].get<std::string>();

        switch (hash(object))
        {
        case hash("Add"):
            {
                auto const value = node.value("value", 0.0f);
                addNode<Add>(value);
            }
            break;
        case hash("Count"):
            {
                auto const min = node.value("min", 0.0f);
                auto const max = node.value("max", std::numeric_limits<int>::max());
                addNode<Count>(min, max);
            }
            break;
        case hash("Print"):
            {
                addNode<Print>();
            }
            break;
        case hash("If"):
            {
                auto const ifVal = node.value("if", 0.0f);
                auto const rtnVal = node.value("return", 0.0f);
                addNode<If>(ifVal, rtnVal);
            }
            break;
        case hash("Env"):
        case hash("Envelope"):
            {
                auto const attackVal = node.value("attack", 0.0f);
                auto const decayVal = node.value("decay", 0.0f);

                //auto const attackCurve = node.value("attackCurve", 1.5f);
                //auto const decayCurve = node.value("decayCurve", 2.0f);
                addNode<Envelope>(attackVal, decayVal);
            }
            break;
        case hash("Metro"):
        case hash("Metronome"):
            {
                auto const value = node.value("hz", 1.0f);
                addNode<Metronome>(value);
            }
            break;
        case hash("Val"):
        case hash("Value"):
            {
                auto const value = node.value("value", 0.0f);
                addNode<Value>(value);
            }
            break;
        case hash("LFO"):
            {
                auto const rate = node.value("rate", 1.0f);
                addNode<LFO>(rate);
            }
            break;
        case hash("Volume"):
            {
                addNode<Volume>();
            }
            break;
        case hash("Osc"):
        case hash("Oscillator"):
            {
                auto const waveform = node.value("waveform", "sine");
                auto const freq = node.value("freq", 440);
                addNode<Oscillator>(waveform, freq);
            }
            break;
        case hash("AOut"):
        case hash("AudioOut"):
            {
                addNode<AudioOut>();
            }
            break;
        default:
            // Unknown object name, return error
            return false;
        }
        return true;
    };

    // Connect nodes dynamically by addressing them by order of addition
    void connect(int oNode, int oPort, int iNode, int iPort) {
        nodes.at(iNode)->linkInputPort(nodes.at(oNode)->getOutputPort(), iPort);

        // Add connection to adjacency list
        adjacencyList[{oNode, oPort}].emplace_back(iNode, iPort);

        // Increment input dependencies for the target node's input port
        inputDependencyMap[{iNode, iPort}]++;
    }

    void printAdjacencyList(const AdjacencyList& adjacencyList) {
        std::cout << "Adjacency List:\n";
        for (const auto& [outputPort, connections] : adjacencyList) {
            auto [oNode, oPort] = outputPort; // Decompose the key
            std::cout << "Node " << oNode << ", Port " << oPort << " -> ";
            for (const auto& [iNode, iPort] : connections) {
                std::cout << "[Node " << iNode << ", Port " << iPort << "] ";
            }
            std::cout << "\n";
        }
    }

    // Topological sort using the provided adjacency list and input dependency map.
    void topologicalSort(std::vector<AudioNode*>& sortedNodes)
    {
        std::vector<bool> isVisited(nodes.size(), false); // Vector for isVisited

        // Recursive DFS lambda
        std::function<void(AudioNode*, int)> dfs = [&](AudioNode* node, int nodeIndex)
        {
            if (isVisited[nodeIndex])
            {
                return;
            }

            isVisited[nodeIndex] = true;

            auto adjacencyIt = adjacencyList.find({nodeIndex, 0}); // 0 for inputPort index
            if (adjacencyIt != adjacencyList.end())
            {
                for (const auto& downstreamNodePair : adjacencyIt->second)
                {
                    int downstreamNodeIndex = downstreamNodePair.first;
                    if (downstreamNodeIndex >= 0 && downstreamNodeIndex < nodes.size())
                    {
                        dfs(nodes[downstreamNodeIndex].get(), downstreamNodeIndex);
                    }
                }
            }

            // Add the node to sorted list after processing its downstream nodes
            sortedNodes.push_back(node);
        };

        // Perform DFS on all unvisited nodes
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (!isVisited[i])
            {
                dfs(nodes[i].get(), static_cast<int>(i));
            }
        }

        // Reverse the sortedNodes vector to get the correct topological order
        std::reverse(sortedNodes.begin(), sortedNodes.end());
    }

    void sortNodes()
    {
#define GRAPH_STATS
#ifdef GRAPH_STATS
        auto start = std::chrono::high_resolution_clock::now();
#endif

        topologicalSort(sortedNodes);

#ifdef GRAPH_STATS
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << sortedNodes.size() << " objects in graph, sort took " << elapsedNs << " ns.\n";

        //printAdjacencyList(adjacencyList);
#endif


//#define DEBUG_SORT
#ifdef DEBUG_SORT
        std::cout << "======== presort =======" << std::endl;
        for (auto& node : nodes)
        {
            std::cout << "node: " << node->getName() << std::endl;
        }

        std::cout << "======== sorted =======" << std::endl;
        for (auto node : sortedNodes)
            std::cout << "node graph: " << node->getName() << std::endl;
#endif
    }

    void process(float* buffer, unsigned long frameCount)
    {
        for (auto& node : sortedNodes) {
            node->process(buffer, frameCount);
        }

        for (auto& node : sortedNodes) {
            if (auto outPort = node->getOutputPort())
                outPort->clearEvents();
        }

        context->eventPool.releaseAllEvents();
    }

protected:

    AdjacencyList adjacencyList;
    InputDependencyMap inputDependencyMap;

};

class Graphs
{
protected:
    std::unique_ptr<AudioGraph> activeGraph;
    std::unique_ptr<AudioGraph> transitioningGraph;
    NodeContext* ctx;
    std::vector<float> fadeOutBuffer;
    std::vector<float> fadeInBuffer;
    bool isTransitioning = false;

public:
    Graphs(NodeContext* context)
        : ctx(context)
    {
        // Resize fade buffers to match the frame count, one frame xfade for now
        fadeOutBuffer.resize(ctx->frameCount);
        fadeInBuffer.resize(ctx->frameCount);

        // Fill the fade buffers with linear fade values
        for (unsigned long i = 0; i < ctx->frameCount; ++i)
        {
            fadeOutBuffer[i] = 1.0f - (static_cast<float>(i) / ctx->frameCount);
            fadeInBuffer[i] = static_cast<float>(i) / ctx->frameCount;
        }

        Logger::getInstance().startProcessingThread();
    }

    ~Graphs()
    {
        Logger::getInstance().stopProcessingThread();
    }

    void setActiveGraph(const json& patch)
    {
        auto newGraph = std::make_unique<AudioGraph>(ctx);
        newGraph->loadPatch(patch);

        if (activeGraph)
        {
            std::cout << "transition to new graph" << std::endl;
            // Start transition if there's an active graph
            transitioningGraph = std::move(newGraph);
            isTransitioning = true;
        }
        else
        {
            // If no active graph, directly assign
            activeGraph = std::move(newGraph);
        }
    }

    void process(float* buffer, unsigned long frameCount)
    {
//#define DSP_TIMING
#ifdef DSP_TIMING
        //=====================
        // 1) Timing the DSP
        //=====================
        static auto lastPrintTime = std::chrono::high_resolution_clock::now();

        static double accumulatedCallbackTimeMs = 0.0;  // Sum of times in ms
        static int    callCount                 = 0;    // Number of callbacks since last print

        auto startTime = std::chrono::high_resolution_clock::now();
#endif

        if (isTransitioning)
        {
            // Temporary buffers for processing
            std::vector<float> activeBuffer(frameCount, 0.0f);
            std::vector<float> transitionBuffer(frameCount, 0.0f);

            // Process each graph into its temporary buffer
            if (activeGraph)
                activeGraph->process(activeBuffer.data(), frameCount);

            if (transitioningGraph)
                transitioningGraph->process(transitionBuffer.data(), frameCount);

            // Apply the fade to the buffers
            for (unsigned long i = 0; i < frameCount; ++i)
            {
                float fadeFactor = i / static_cast<float>(frameCount);

                // Mix the faded buffers into the output buffer
                buffer[i] = (activeBuffer[i] * (1 - fadeFactor)) + (transitionBuffer[i] * fadeFactor);
            }

            activeGraph = std::move(transitioningGraph);
            isTransitioning = false;
        }
        else if (activeGraph)
        {
            // Only process the active graph if no transition is occurring
            activeGraph->process(buffer, frameCount);
        }

#ifdef DSP_TIMING
        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate how long (in ms) the callback took
        double callbackTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

        // Accumulate for averaging
        accumulatedCallbackTimeMs += callbackTimeMs;
        callCount++;

        //=====================
        // 2) Check if 1 second has passed
        //=====================
        auto now = std::chrono::high_resolution_clock::now();
        double elapsedSec = std::chrono::duration<double>(now - lastPrintTime).count();

        if (elapsedSec >= 1.0)  // Once a second
        {
            // 2a) Compute average callback duration in ms
            double averageMs = accumulatedCallbackTimeMs / callCount;

            // 2b) Compute percentage of available time used
            //     - Time available per callback (in ms)
            //       = (frameCount / sampleRate) * 1000
            double periodMs   = 1000.0 * (static_cast<double>(frameCount) / ctx->sampleRate);
            double usagePct   = (averageMs / periodMs) * 100.0;

            // 2c) Print results
            std::cout
                << "Average callback time over last second: "
                << averageMs << " ms, which is "
                << usagePct << "% of available time.\n";

            // 2d) Reset counters for next 1-second interval
            lastPrintTime             = now;
            accumulatedCallbackTimeMs = 0.0;
            callCount                 = 0;
        }
#endif
    }
};