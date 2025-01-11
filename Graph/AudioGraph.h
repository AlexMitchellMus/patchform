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

#include "../external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include "../external/concurrentqueue/concurrentqueue.h"

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
private:
    std::vector<std::unique_ptr<AudioNode>> nodes;
    std::vector<AudioNode*> sortedNodes;

    NodeContext* context;

public:
    AudioGraph(NodeContext* context) : context(context)
    {
    }

    void loadPatch(const json& patch) {
        auto createObject = [this](json node)
        {
            auto const object = node["type"].get<std::string>();

            switch (hash(object))
            {
            case hash("Add"):
                {
                    auto const value = node.value("value", 0.0f);
                    nodes.push_back(std::make_unique<Add>(context, value));
                }
                break;
            case hash("Count"):
                {
                    auto const min = node.value("min", 0.0f);
                    auto const max = node.value("max", std::numeric_limits<int>::max());
                    nodes.push_back(std::make_unique<Count>(context, min, max));
                }
                break;
            case hash("Print"):
                {
                    nodes.push_back(std::make_unique<Print>(context));
                }
                break;
            case hash("If"):
                {
                    auto const ifVal = node.value("if", 0.0f);
                    auto const rtnVal = node.value("return", 0.0f);
                    nodes.push_back(std::make_unique<If>(context, ifVal, rtnVal));
                }
                break;
            case hash("Envelope"):
                {
                    auto const attackVal = node.value("attack", 0.0f);
                    auto const decayVal = node.value("decay", 0.0f);

                    //auto const attackCurve = node.value("attackCurve", 1.5f);
                    //auto const decayCurve = node.value("decayCurve", 2.0f);
                    nodes.push_back(std::make_unique<Envelope>(context, attackVal, decayVal));
                }
                break;
            case hash("Metro"):
                {
                    auto const value = node.value("hz", 1.0f);
                    nodes.push_back(std::make_unique<Metronome>(context, value));
                }
                break;
            case hash("Value"):
                {
                    auto const value = node.value("value", 0.0f);
                    nodes.push_back(std::make_unique<Value>(context, value));
                }
                break;
            case hash("LFO"):
                {
                    auto const rate = node.value("rate", 1.0f);
                    nodes.push_back(std::make_unique<LFO>(context, rate));
                }
                break;
            case hash("Volume"):
                {
                    nodes.push_back(std::make_unique<Volume>(context));
                }
                break;
            case hash("Oscillator"):
                {
                    auto const waveform = node.value("waveform", "sine");
                    auto const freq = node.value("freq", 440);
                    nodes.push_back(std::make_unique<Oscillator>(context, waveform, freq));
                }
                break;
            case hash("AudioOut"):
                {
                    nodes.push_back(std::make_unique<AudioOut>(context));
                }
                break;
            default:
                break;
            }
        };

        // Create nodes
        for (const auto& node : patch["nodes"]) {
            createObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"]) {
            connect(connection["sourceNode"], connection["sourcePort"], connection["targetNode"], connection["targetPort"]);
        }

        sortNodes();
    }

    // Connect nodes dynamically by addressing them by order of addition
    void connect(int oNode, int oPort, int iNode, int iPort) {
        nodes.at(iNode)->linkInputPort(nodes.at(oNode)->getOutputPort(), iPort);
    }

    // This helper scans ALL nodes to find which nodes are downstream of `node`.
    std::vector<AudioNode*> getDownstreamNodes(AudioNode* node, const std::vector<std::unique_ptr<AudioNode>>& allNodes)
    {
        std::vector<AudioNode*> result;
        AudioPort* myOutputPort = node->getOutputPort();

        // Iterate by reference: auto& or const auto&
        for (auto& otherNode : allNodes)
        {
            if (otherNode.get() == node)
                continue; // skip self

            // Check each named input port
            for (const auto& inputPort : otherNode->getInputPorts())
            {
                // Each inputPort can have multiple connections
                for (auto* connected : inputPort.connectedPorts)
                {
                    // If otherNode’s input is connected to *this* node’s output,
                    // we have an edge: node -> otherNode
                    if (connected == myOutputPort)
                    {
                        result.push_back(otherNode.get());
                        goto NextOtherNode;
                    }
                }
            }
            NextOtherNode:;
        }

        return result;
    }


    // DFS-based topological sort that builds adjacency from "node -> its downstream nodes".
    void topologicalSort(std::vector<AudioNode*>& sortedNodes)
    {
        std::vector<AudioNode*> stack;

        // Recursive DFS lambda
        std::function<void(AudioNode*)> dfs = [&](AudioNode* node)
        {
            if (node->state == AudioNode::State::Visiting) {
                std::cerr << "Cycle detected at node: " << node->getName() << std::endl;
                return;
            }
            if (node->state == AudioNode::State::Visited) {
                return;
            }

            node->state = AudioNode::State::Visiting;

            // Get all nodes that depend on this node's output
            auto downstreamNodes = getDownstreamNodes(node, nodes);

            for (AudioNode* downstream : downstreamNodes) {
                dfs(downstream);
            }

            node->state = AudioNode::State::Visited;
            stack.push_back(node);
        };

        // Perform DFS for each unvisited node
        for (auto& node : nodes) {
            if (node && node->state == AudioNode::State::Unvisited) {
                dfs(node.get());
            }
        }

        // Reverse the stack for topological order
        sortedNodes.assign(stack.rbegin(), stack.rend());
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

        //std::cout << "Free events: " << context->eventPool.eventPoolSize() << std::endl;
    }
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