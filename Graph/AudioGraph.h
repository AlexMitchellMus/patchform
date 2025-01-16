/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <limits>
#include <iostream>
#include <iomanip>
#include <stack>
#include <chrono>
#include <xutility>
#include <queue>

#include "json.hpp"
using json = nlohmann::json;

#include "unordered_dense.h"

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

#include "PortHelpers.h"

#include "Logger.h"
#include "../Utility/ppl_string.hpp"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
private:
    NodeContext* context;

public:
    AudioGraph(NodeContext* context) : context(context)
    {
    }

    void loadPatch(const json& patch, bool logVerbose)
    {
        // Create nodes
        for (const auto& node : patch["nodes"]) {
            addObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"]) {
            // source and target ID needs to be set in the file format
            uint32_t source = connection["sourceNode"].is_string() ? objectIDMap[connection["sourceNode"].get<std::string>()] : connection["sourceNode"].get<int>();
            uint32_t target = connection["targetNode"].is_string() ? objectIDMap[connection["targetNode"].get<std::string>()] : connection["targetNode"].get<int>();

            connect(source, connection["sourcePort"], target, connection["targetPort"]);
        }

        sortNodes();

        if (logVerbose)
        {
            printAdjacencyList();
        }
    }

    template <typename NodeType, typename... Args>
    void addNode(const std::optional<std::string>& idString, Args&&... args) {
        auto nodeID = objectsList.size();
        auto node = std::make_unique<NodeType>(context, std::forward<Args>(args)...);
        node->nodeID = nodeID;

        // Determine the ID to use for the object ID map
        const std::string finalID = idString.has_value() ? idString.value() : std::to_string(nodeID);  // Fallback to node index in vector

        objectIDMap[finalID] = nodeID;

        // Function responsible for summing audio & event buffers of connected inputs for each node.
        // This dynamically looks up the connections port via the connection table.
        // TODO: cache the connected port, and only recalculate if flag is set
        node->sumInputBuffers = [this, nodeID](std::vector<std::unique_ptr<AudioPort>>& inputPorts) {
            for (size_t portID = 0; portID < inputPorts.size(); ++portID) {
                auto& port = inputPorts[portID];

                // Data ports don't process audio
                // Audio ports process both audio and data
                if (port->isSignal()) {
                    // clear the audio buffer for the current portID
                    port->setSize(context->frameCount);
                }

                port->clearEvents();
                auto& summingEventBuffer = port->getEvents();
                auto summingAudioBuffer = port->getAudioBuffer();

                // Find connections for the current port
                auto it = adjacencyMap.getBackward().find(PortHelpers::getKey(nodeID, portID));
                if (it != adjacencyMap.getBackward().end()) {
                    const auto& connections = it->second;

                    // Sum contributions from connected nodes
                    for (uint32_t connKey : connections) {
                        // Fetch the output buffer from the connected node
                        auto connection = objectsListCopy[PortHelpers::getNodeID(connKey)]->getOutputPort();
                        const auto outputBuffer = connection->getAudioBuffer();

                        if (port->isSignal()) {
                            // Set the ports preference to signal values if any of the connected ports are signal
                            port->isAnyConnectedPortSignal = port->isAnyConnectedPortSignal || connection->isSignal();
                            // Accumulate values in the buffer
                            for (size_t i = 0; i < context->frameCount; ++i) {
                                summingAudioBuffer[i] += outputBuffer[i];
                            }
                        }
                        auto& events = connection->getEvents();
                        summingEventBuffer.insert(summingEventBuffer.end(), events.begin(), events.end());

                        // Sort combined by timestamp
                        std::sort(summingEventBuffer.begin(), summingEventBuffer.end(), [](const Event* a, const Event* b) {
                            return a->getTimeStamp() < b->getTimeStamp();
                        });
                    }
                }
            }
        };
        objectsList.push_back(std::move(node));
    };

    bool addObject(json node)
    {
        auto const object = ppl::string(node["obj"].get<std::string>()).toLower();

        // Attempt to get the ID as a string,
        // if no string dump the value (int or float into string)
        // otherwise return empty value which makes the object use the index in patch
        const std::optional<std::string> idString = node.contains("id")
            ? (node["id"].is_string()
                ? std::make_optional(node["id"].get<std::string>())
                : std::make_optional(node["id"].dump()))
            : std::nullopt;

        switch (hash(object))
        {
        case hash("add"):
            {
                auto const value = node.value("value", 0.0f);
                addNode<Add>(idString, value);
            }
            break;
        case hash("count"):
            {
                auto const min = node.value("min", 0.0f);
                auto const max = node.value("max", std::numeric_limits<int>::max());
                addNode<Count>(idString, min, max);
            }
            break;
        case hash("print"):
            {
                addNode<Print>(idString);
            }
            break;
        case hash("if"):
            {
                auto const ifVal = node.value("if", 0.0f);
                auto const rtnVal = node.value("return", 0.0f);
                addNode<If>(idString, ifVal, rtnVal);
            }
            break;
        case hash("env"):
        case hash("envelope"):
            {
                auto const attackVal = node.value("attack", 0.0f);
                auto const decayVal = node.value("decay", 0.0f);

                //auto const attackCurve = node.value("attackCurve", 1.5f);
                //auto const decayCurve = node.value("decayCurve", 2.0f);
                addNode<Envelope>(idString, attackVal, decayVal);
            }
            break;
        case hash("metro"):
        case hash("metronome"):
            {
                auto const value = node.value("hz", 1.0f);
                addNode<Metronome>(idString, value);
            }
            break;
        case hash("val"):
        case hash("value"):
            {
                auto const value = node.value("value", 0.0f);
                addNode<Value>(idString, value);
            }
            break;
        case hash("lfo"):
            {
                auto const rate = node.value("rate", 1.0f);
                addNode<LFO>(idString, rate);
            }
            break;
        case hash("volume"):
            {
                addNode<Volume>(idString);
            }
            break;
        case hash("osc"):
        case hash("oscillator"):
            {
                auto const waveform = node.value("waveform", "sine");
                auto const freq = node.value("freq", 440);
                addNode<Oscillator>(idString, waveform, freq);
            }
            break;
        case hash("aout"):
        case hash("audioout"):
            {
                addNode<AudioOut>(idString);
            }
            break;
        default:
            // Unknown object name, return error
            std::cout << "Unknown object: " << object << std::endl;
            return false;
        }
        return true;
    };

    // Connect nodes dynamically by addressing them by order of addition
    void connect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto outputKey = PortHelpers::getKey(oNode, oPort);
        auto inputKey = PortHelpers::getKey(iNode, iPort);

        adjacencyMap.addAdjacency(inputKey, outputKey);
    }

    void printGraph()
    {
        for (const auto& obj : objectsList)
        {
             std::cout << obj->nodeID << " [" << obj->getShortName() << "]" << std::endl;
        }
    }

    void printAdjacencyList()
    {
        std::cout << "Adjacency List:\n";

        // Calculate maximum widths for `[name]` and the full left-hand side
        size_t maxNameWidth = 0;
        size_t maxLeftWidth = 0;

        // Precompute maximum widths for character alignment
        for (const auto& node : objectsSorted)
        {
            auto it = std::find_if(
                objectsListCopy.begin(), objectsListCopy.end(),
                [&node](const AudioNode* n) { return n == node; });

            if (it != objectsListCopy.end())
            {
                size_t nodeIndex = std::distance(objectsListCopy.begin(), it);

                for (const auto& outputPort : adjacencyMap.getForward() | std::views::keys)
                {
                    auto [oNode, oPort] = PortHelpers::getNodeAndPortID(outputPort);

                    if (oNode == nodeIndex)
                    {
                        std::string namePart = "[" + node->getShortName() + "]";
                        std::string leftSide = namePart + " " + std::to_string(oNode) + ", Port " + std::to_string(oPort);

                        maxNameWidth = std::max(maxNameWidth, namePart.length());
                        maxLeftWidth = std::max(maxLeftWidth, leftSide.length());
                    }
                }
            }
        }

        // Print the adjacency list in sorted order
        for (const auto& node : objectsSorted)
        {
            auto it = std::find_if(
                objectsListCopy.begin(), objectsListCopy.end(),
                [&node](const AudioNode* n) { return n == node; });

            if (it != objectsListCopy.end())
            {
                size_t nodeIndex = std::distance(objectsListCopy.begin(), it);

                for (const auto& [outputPort, connections] : adjacencyMap.getForward())
                {
                    auto [oNode, oPort] = PortHelpers::getNodeAndPortID(outputPort);

                    if (oNode == nodeIndex)
                    {
                        std::string namePart = "[" + node->getShortName() + "]";
                        std::string leftSide = namePart + " " + std::to_string(oNode) + ", Port " + std::to_string(oPort);

                        // Print the left side (name + node/port info) with alignment
                        std::cout << std::setw(maxNameWidth) << std::left << namePart << " " << std::setw(maxLeftWidth - maxNameWidth)
                        << (std::to_string(oNode) + ", Port " + std::to_string(oPort)) << " -> ";

                        // Print connections
                        if (!connections.empty())
                        {
                            bool first = true;
                            for (const auto& portKey : connections)
                            {
                                auto [iNode, iPort] = PortHelpers::getNodeAndPortID(portKey);
                                if (!first)
                                {
                                    // Align continuation lines
                                    std::cout << "\n" << std::setw(maxNameWidth + maxLeftWidth - 1) << std::right;
                                }
                                first = false;
                                std::cout << "[" << objectsListCopy[iNode]->getShortName() << "] " << std::to_string(iNode) << ", Port " << std::to_string(iPort) << "] ";
                            }
                        }
                        std::cout << "\n";
                    }
                }
            }
        }
    }


    // Topological sort using the provided adjacency list
    void topologicalSort(std::vector<AudioNode*>& sortedNodes)
    {
        size_t nodeCount = objectsListCopy.size();

        sortedNodes.reserve(nodeCount);
        sortedNodes.clear();

        zeroInDegreeNodes.reserve(nodeCount);
        zeroInDegreeNodes.clear();

        inDegree.assign(nodeCount, 0);

        // Compute in-degrees in a single pass
        for (const auto& [inputKey, outputKeys] : adjacencyMap.getBackward())
        {
            int nodeIndex = PortHelpers::getNodeID(inputKey);
            if (nodeIndex >= 0 && nodeIndex < static_cast<int>(nodeCount))
            {
                ++inDegree[nodeIndex];
            }
        }

        for (size_t i = 0; i < nodeCount; ++i)
        {
            if (inDegree[i] == 0)
            {
                zeroInDegreeNodes.push_back(static_cast<int>(i));
            }
        }

        // Process nodes in topological order
        size_t processIndex = 0;
        while (processIndex < zeroInDegreeNodes.size())
        {
            int currentIndex = zeroInDegreeNodes[processIndex++];
            sortedNodes.push_back(objectsListCopy[currentIndex]);

            // Reduce in-degree for downstream nodes
            auto adjacencyIt = adjacencyMap.getForward().find(PortHelpers::getKey(currentIndex, 0)); // 0 for inputPort index
            if (adjacencyIt != adjacencyMap.getForward().end())
            {
                for (const auto& downstreamKey : adjacencyIt->second)
                {
                    int downstreamNodeIndex = PortHelpers::getNodeID(downstreamKey);
                    if (downstreamNodeIndex >= 0 && downstreamNodeIndex < static_cast<int>(nodeCount))
                    {
                        if (--inDegree[downstreamNodeIndex] == 0)
                        {
                            zeroInDegreeNodes.push_back(downstreamNodeIndex);
                        }
                    }
                }
            }
        }

        // Check for cycles: If sortedNodes.size() != nodes.size(), there is a cycle
        if (sortedNodes.size() != nodeCount)
        {
            sortedNodes.clear();
            std::cout << "Cycle detected, clearing graph" << std::endl;
        }
    }

    void sortNodes()
    {
#define GRAPH_STATS
#ifdef GRAPH_STATS
        auto start = std::chrono::high_resolution_clock::now();
#endif

        objectsListCopy.reserve(objectsList.size());
        objectsListCopy.clear();

        for (auto& obj : objectsList) {
            objectsListCopy.push_back(obj.get());
        }

        topologicalSort(objectsSorted);

#ifdef GRAPH_STATS
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << objectsSorted.size() << " objects in graph, sort took " << elapsedNs << " ns.\n";
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
        for (auto& node : objectsSorted) {
            node->process(buffer, frameCount);
        }

        for (auto& node : objectsSorted) {
            if (auto outPort = node->getOutputPort())
                outPort->clearEvents();
        }

        context->eventPool.releaseAllEvents();
    }

protected:

    std::vector<std::unique_ptr<AudioNode>> objectsList;

    // Keep track of object id's and position in objectList
    ankerl::unordered_dense::map<std::string, uint32_t> objectIDMap;

    std::vector<AudioNode*> objectsListCopy;
    std::vector<AudioNode*> objectsSorted;

    // Only for sorting
    std::vector<int> zeroInDegreeNodes;
    std::vector<int> inDegree;

    struct AdjacencyMap
    {
        // Custom Adjacency List definition using the custom hash
        using AdjacencyList = ankerl::unordered_dense::map<uint32_t, std::vector<uint32_t>>;

        void addAdjacency(uint32_t inputKey, uint32_t outputKey)
        {
            forward[outputKey].emplace_back(inputKey);
            backward[inputKey].emplace_back(outputKey);
        }

        [[nodiscard]] const AdjacencyList& getForward() const
        {
            return forward;
        }

        [[nodiscard]] const AdjacencyList& getBackward() const
        {
            return backward;
        }

    private:
        AdjacencyList forward{};
        AdjacencyList backward{};
    } adjacencyMap;
};

class GraphManager
{
protected:
    std::unique_ptr<AudioGraph> activeGraph;
    std::unique_ptr<AudioGraph> transitioningGraph;
    NodeContext* ctx;
    std::vector<float> fadeOutBuffer;
    std::vector<float> fadeInBuffer;
    bool isTransitioning = false;

public:
    GraphManager(NodeContext* context)
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

    ~GraphManager()
    {
        Logger::getInstance().stopProcessingThread();
    }

    bool addObject(const std::string& objName)
    {
        if (!activeGraph)
        {
            activeGraph = std::make_unique<AudioGraph>(ctx);
        }

        // TODO: Lock the graph, or communicate via a queue

        json object;
        object["obj"] = objName;
        return activeGraph->addObject(object);
    }

    void printAdjacencyList()
    {
        if (!activeGraph) {
            std::cout << "No graph loaded" << std::endl;
            return;
        }

        activeGraph->printAdjacencyList();
    }

    void printGraph()
    {
        if (!activeGraph) {
            std::cout << "No graph loaded" << std::endl;
            return;
        }

        activeGraph->printGraph();
    }

    void setActiveGraph(const json& patch, bool logVerbose)
    {
        auto newGraph = std::make_unique<AudioGraph>(ctx);
        newGraph->loadPatch(patch, logVerbose);

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