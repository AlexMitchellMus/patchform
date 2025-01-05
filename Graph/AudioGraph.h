
#include <vector>
#include <iostream>
#include <stack>
#include <chrono>

#include "../external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

#pragma once

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

    void loadPatch(json patch) {
        auto createObject = [this](json node)
        {
            auto const object = node["type"].get<std::string>();

            switch (hash(object))
            {
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
                    nodes.push_back(std::make_unique<Metro>(context, value));
                }
                break;
            case hash("Value"):
                {
                    auto const value = node.value("value", 0.0f);
                    nodes.push_back(std::make_unique<ValueNode>(context, value));
                }
                break;
            case hash("LFO"):
                {
                    auto const rate = node.value("rate", 1.0f);
                    nodes.push_back(std::make_unique<LFONode>(context, rate));
                }
                break;
            case hash("Volume"):
                {
                    nodes.push_back(std::make_unique<VolumeNode>(context));
                }
                break;
            case hash("Oscillator"):
                {
                    auto const waveform = node.value("waveform", "sine");
                    nodes.push_back(std::make_unique<Oscillator>(context, waveform));
                }
                break;
            case hash("AudioOut"):
                {
                    nodes.push_back(std::make_unique<AudioOutNode>(context));
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
        std::stack<AudioNode*> stack;
        std::unordered_map<AudioNode*, bool> visited;
        std::unordered_map<AudioNode*, bool> inStack; // for cycle detection

        // Recursive DFS lambda
        std::function<void(AudioNode*)> dfs = [&](AudioNode* node)
        {
            if (inStack[node]) {
                std::cerr << "Cycle detected at node: " << node->getName() << std::endl;
                return;
            }
            if (visited[node]) {
                return;
            }

            visited[node] = true;
            inStack[node] = true;

            // Get all nodes that depend on this node's output
            auto downstreamNodes = getDownstreamNodes(node, nodes);

            for (auto* downstream : downstreamNodes) {
                if (!visited[downstream]) {
                    dfs(downstream);
                }
            }

            inStack[node] = false;
            stack.push(node);
        };

        // Initiate DFS from every node that isn’t visited yet
        for (auto& node : nodes) {
            if (node && !visited[node.get()]) {
                dfs(node.get());
            }
        }

        // Pop from the stack to sortedNodes, then reverse for final topological order
        while (!stack.empty()) {
            sortedNodes.push_back(stack.top());
            stack.pop();
        }
        //std::reverse(sortedNodes.begin(), sortedNodes.end());
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

    void setActiveGraph(json patch)
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