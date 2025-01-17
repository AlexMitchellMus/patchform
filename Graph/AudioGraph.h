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
#include "AdjacencyMap.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
public:
    AudioGraph(NodeContext* context)
        : context(context)
    {
    }

    AudioGraph(AudioGraph* other)
    : context(other->context)
    , adjacencyMap(other->adjacencyMap)
    {
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

    void sortNodes(const std::vector<std::shared_ptr<AudioNode>>& objectList)
    {
#define GRAPH_STATS
#ifdef GRAPH_STATS
        auto start = std::chrono::high_resolution_clock::now();
#endif

        objectsListCopy.reserve(objectList.size());
        objectsListCopy.clear();

        for (auto& obj : objectList) {
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
            node->process(buffer, frameCount, *this);
        }

        for (auto& node : objectsSorted) {
            if (auto outPort = node->getOutputPort())
                outPort->clearEvents();
        }

        context->eventPool.releaseAllEvents();
    }

    bool flagForDeletion = false;

    std::vector<AudioNode*> objectsListCopy;
    std::vector<AudioNode*> objectsSorted;

    // Only for sorting
    std::vector<int> zeroInDegreeNodes;
    std::vector<int> inDegree;

    bool addAdjacency(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto outputKey = PortHelpers::getKey(oNode, oPort);
        auto inputKey = PortHelpers::getKey(iNode, iPort);

        if (adjacencyMap.containsAdjacency(inputKey, outputKey))
        {
            std::cout << "Connection already exits!" << std::endl;
            return false;
        }

        adjacencyMap.addAdjacency(inputKey, outputKey);
        return true;
    }

    void removeAdjacency(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto outputKey = PortHelpers::getKey(oNode, oPort);
        auto inputKey = PortHelpers::getKey(iNode, iPort);

        if (adjacencyMap.containsAdjacency(inputKey, outputKey)) {
            adjacencyMap.removeAdjacency(inputKey, outputKey);
        } else {
            std::cout << "Connection doesnt exist!" << std::endl;
        }
    }
    AdjacencyMap adjacencyMap;
    NodeContext* context;
};

class GraphHolder
{
    std::vector<std::shared_ptr<AudioNode>> mainObjectList;
    ankerl::unordered_dense::map<std::string, uint32_t> objectIDMap;
    NodeContext* context;

public:
    std::unique_ptr<AudioGraph> graph;

    GraphHolder(NodeContext* ctx)
        : context(ctx)
    {
        graph = std::make_unique<AudioGraph>(ctx);
    };

    GraphHolder(const GraphHolder* other)
    : context(other->context) // Reuse the same context
    , mainObjectList(other->mainObjectList)
    , objectIDMap(other->objectIDMap)
{
        graph = std::make_unique<AudioGraph>(other->graph.get());
}

    AudioGraph* getGraph() const
    {
        return graph.get();
    }

    void loadPatch(const json& patch, bool logVerbose)
    {
        // Create nodes
        for (const auto& node : patch["nodes"])
        {
            addObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"])
        {
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

    // Create connections from idString:port pairs
    bool connect(const std::string& oObj, int oPort, const std::string& iObj, int iPort) {
        if (objectIDMap.contains(oObj) && objectIDMap.contains(iObj))
        {
            connect(objectIDMap[oObj], oPort, objectIDMap[iObj], iPort);
            return true;
        }
        return false;
    }

    // Create connections with the object index
    void connect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        graph->addAdjacency(oNode, oPort, iNode, iPort);
    }

    // Create connections from idString:port pairs
    bool disconnect(const std::string& oObj, int oPort, const std::string& iObj, int iPort) {
        if (objectIDMap.contains(oObj) && objectIDMap.contains(iObj))
        {
            disconnect(objectIDMap[oObj], oPort, objectIDMap[iObj], iPort);
            return true;
        }
        return false;
    }

    // Create connections with the object index
    void disconnect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        graph->removeAdjacency(oNode, oPort, iNode, iPort);
    }

    void process(float* buffer, unsigned long frameCount)
    {
        graph->process(buffer, frameCount);
    }

    void sortNodes()
    {
        graph->sortNodes(mainObjectList);
    }

    void printAdjacencyList()
    {
        graph->printAdjacencyList();
    }

    void setSummingFunctionForNode(AudioNode* node)
    {
        auto nodeID = node->nodeID;
        node->sumInputBuffers = [nodeID](const std::vector<std::unique_ptr<AudioPort>>& inputPorts,
                                               const AudioGraph& runningGraph)
        {
            for (size_t portID = 0; portID < inputPorts.size(); ++portID)
            {
                auto& port = inputPorts[portID];

                if (port->isSignal())
                {
                    // Clear and resize audio buffer only for signal ports
                    port->setSize(runningGraph.context->frameCount);
                }

                port->clearEvents();
                auto& summingEventBuffer = port->getEvents();
                auto summingAudioBuffer = port->getAudioBuffer();

                // Retrieve connections for the current port
                auto it = runningGraph.adjacencyMap.getBackward().find(PortHelpers::getKey(nodeID, portID));
                if (it == runningGraph.adjacencyMap.getBackward().end())
                {
                    continue;
                }

                for (uint32_t connKey : it->second)
                {
                    auto connectedNode = runningGraph.objectsListCopy[PortHelpers::getNodeID(connKey)];
                    auto connection = connectedNode->getOutputPort();
                    const auto outputBuffer = connection->getAudioBuffer();

                    if (port->isSignal() && connection->isSignal())
                    {
                        // Update signal status
                        port->isAnyConnectedPortSignal = true;
                        std::transform(
                            outputBuffer, outputBuffer + runningGraph.context->frameCount,
                            summingAudioBuffer, summingAudioBuffer,
                            std::plus<>());
                    }

                    // Collect and merge events
                    auto& events = connection->getEvents();
                    summingEventBuffer.insert(summingEventBuffer.end(), events.begin(), events.end());
                }

                // Sort combined events only if there are new events
                if (!summingEventBuffer.empty())
                {
                    std::sort(summingEventBuffer.begin(), summingEventBuffer.end(), [](const Event* a, const Event* b)
                    {
                        return a->getTimeStamp() < b->getTimeStamp();
                    });
                }
            }
        };
    }

    template <typename NodeType, typename... Args>
    void addNode(const std::optional<std::string>& idString, Args&&... args) {
        auto nodeID = mainObjectList.size();
        auto node = std::make_unique<NodeType>(context, std::forward<Args>(args)...);
        node->nodeID = nodeID;

        // Determine the ID to use for the object ID map
        const std::string finalID = idString.has_value() ? idString.value() : std::to_string(nodeID);  // Fallback to node index in vector

        objectIDMap[finalID] = nodeID;

        // Function responsible for summing audio & event buffers of connected inputs for each node.
        // This dynamically looks up the connections port via the connection table.
        // TODO: cache the connected port, and only recalculate if flag is set
        setSummingFunctionForNode(node.get());
        mainObjectList.push_back(std::move(node));
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

    void printGraph()
    {
        for (const auto& obj : mainObjectList)
        {
            std::cout << obj->nodeID << " [" << obj->getShortName() << "]" << std::endl;
        }
    }
};

class GraphManager
{
public:
    GraphManager(NodeContext* context)
        : ctx(context)
    {
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
            activeGraph = std::make_unique<GraphHolder>(ctx);
        }

        // TODO: Lock the graph, or communicate via a queue

        json object;
        object["obj"] = objName;
        return activeGraph->addObject(object);
    }

    bool connect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
        if (!activeGraph) {
            std::cerr << "No active graph available to connect objects." << std::endl;
            return false;
        }

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());
        // Add the connection to the transitioning graph
        if (!transitioningGraph->connect(oObj, oPort, iObj, iPort)) {
            std::cerr << "Failed to connect objects in the transitioning graph." << std::endl;
            transitioningGraph.reset(); // Discard transitioning graph
            return false;
        }

        transitioningGraph->sortNodes();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
        return true;
    }

    bool disconnect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
        if (!activeGraph) {
            std::cerr << "No active graph available to disconnect connection." << std::endl;
            return false;
        }

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());
        // Add the connection to the transitioning graph
        if (!transitioningGraph->disconnect(oObj, oPort, iObj, iPort)) {
            std::cerr << "Failed to connect objects in the transitioning graph." << std::endl;
            transitioningGraph.reset(); // Discard transitioning graph
            return false;
        }

        transitioningGraph->sortNodes();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
        return true;
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

    void setActiveGraph(const json& patch, bool logVerbose) {
        if (swapGraph.load(std::memory_order_acquire)) {
            std::cout << "Warning: Attempted to overwrite a transitioning graph before it was swapped." << std::endl;
            return;
        }
        transitioningGraph = std::make_shared<GraphHolder>(ctx);
        transitioningGraph->loadPatch(patch, logVerbose);
        swapGraph.store(true, std::memory_order_release);
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
        if (swapGraph.load(std::memory_order_acquire)) {
            // Perform the swap on the audio thread
            activeGraph.swap(transitioningGraph);
            swapGraph.store(false, std::memory_order_release);
        }

        // Process the current graph
        auto graph = activeGraph;
        if (graph) {
            graph->process(buffer, frameCount);
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
            Logger::getInstance().log("Average callback time over last second: "
                + std::to_string(averageMs) + " ms, which is "
                + std::to_string(usagePct) + "% of available time");

            // 2d) Reset counters for next 1-second interval
            lastPrintTime             = now;
            accumulatedCallbackTimeMs = 0.0;
            callCount                 = 0;
        }
#endif
    }

protected:
    std::shared_ptr<GraphHolder> activeGraph;         // Actively processed graph
    std::shared_ptr<GraphHolder> transitioningGraph;  // New graph prepared for swapping
    std::atomic<bool> swapGraph = false;             // Signal for readiness to swap
    NodeContext* ctx;
};