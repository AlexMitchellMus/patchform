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
#include <set>

#include "json.hpp"
using json = nlohmann::json;

#include "glaze/glaze.hpp"

#include "unordered_dense.h"

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

#include "Logger.h"
#include "../Utility/ppl_string.hpp"
#include "AdjacencyMap.h"
#include "Connection.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
public:
    AudioGraph(NodeContext* context)
        : context(context)
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
                    auto [oNode, oPort] = AdjacencyMap::unpackKey(outputPort);

                    if (oNode == nodeIndex)
                    {
                        std::string namePart = "[" + node->getShortName() + "]";
                        std::string leftSide = namePart + " " + std::to_string(objectsListCopy[oNode]->nodeID) + ", Port " + std::to_string(oPort);

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
                    auto [oNode, oPort] = AdjacencyMap::unpackKey(outputPort);

                    if (oNode == nodeIndex)
                    {
                        std::string nodeAsID = std::to_string(objectsListCopy[oNode]->nodeID);
                        std::string namePart = "[" + node->getShortName() + "]";
                        std::string leftSide = namePart + " " + nodeAsID + ", Port " + std::to_string(oPort);

                        // Print the left side (name + node/port info) with alignment
                        std::cout << std::setw(maxNameWidth) << std::left << namePart << " " << std::setw(maxLeftWidth - maxNameWidth) << (nodeAsID + ", Port " + std::to_string(oPort)) << " -> ";

                        // Print connections
                        if (!connections.empty())
                        {
                            bool first = true;
                            for (const auto& portKey : connections)
                            {
                                auto [iNode, iPort] = AdjacencyMap::unpackKey(portKey);
                                if (!first)
                                {
                                    // Align continuation lines
                                    std::cout << "\n" << std::setw(maxNameWidth + maxLeftWidth - 1) << std::right;
                                }
                                first = false;
                                std::string iNodeAsID = std::to_string(objectsListCopy[iNode]->nodeID);
                                std::cout << "[" << objectsListCopy[iNode]->getShortName() << "] " << iNodeAsID << ", Port " << std::to_string(iPort) << "] ";
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
        const unsigned int nodeCount = objectsListCopy.size();

        sortedNodes.reserve(nodeCount);
        sortedNodes.clear();

        zeroInDegreeNodes.reserve(nodeCount);
        zeroInDegreeNodes.clear();

        inDegree.assign(nodeCount, 0);

        // Compute in-degrees in a single pass
        for (const auto& [inputKey, outputKeys] : adjacencyMap.getBackward())
        {
            if (int nodeIndex = AdjacencyMap::getNodeID(inputKey); nodeIndex >= 0 && nodeIndex < nodeCount)
            {
                ++inDegree[nodeIndex];
            }
        }

        for (unsigned int i = 0; i < nodeCount; ++i)
        {
            if (inDegree[i] == 0)
            {
                zeroInDegreeNodes.push_back(i);
            }
        }

        // Process nodes in topological order
        size_t processIndex = 0;
        while (processIndex < zeroInDegreeNodes.size())
        {
            const auto currentIndex = zeroInDegreeNodes[processIndex++];
            sortedNodes.push_back(objectsListCopy[currentIndex]);

            // Reduce in-degree for downstream nodes
            if (auto adjacencyIt = adjacencyMap.getForward().find(AdjacencyMap::packKey(currentIndex, 0)); adjacencyIt != adjacencyMap.getForward().end())
            {
                for (const auto& downstreamKey : adjacencyIt->second)
                {
                    int downstreamNodeIndex = AdjacencyMap::getNodeID(downstreamKey);
                    if (downstreamNodeIndex >= 0 && downstreamNodeIndex < nodeCount)
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
#define SORT_TIME
#ifdef SORT_TIME
        auto start = std::chrono::high_resolution_clock::now();
#endif

        objectsListCopy.reserve(objectList.size());
        objectsListCopy.clear();

        for (auto& obj : objectList)
        {
            objectsListCopy.push_back(obj.get());
        }

        topologicalSort(objectsSorted);

#ifdef SORT_TIME
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << objectList.size() << " objects in graph " <<  objectsSorted.size() << " objects sorted, sort took " << elapsedNs << " ns.\n";
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
        for (size_t i = 0; i < objectsSorted.size(); i++) {
            objectsSorted[i]->process(buffer, frameCount, *this, i);
        }

        for (auto& node : objectsSorted) {
            if (auto outPort = node->getOutputPort())
                outPort->clearEvents();
        }

        context->eventPool.releaseAllEvents();
    }

    bool flagForDeletion = false;

    std::vector<AudioNode*> objectsListCopy;

    ankerl::unordered_dense::map<uint32_t, uint32_t> objectIDtoIndex;

    OutputPortMap outputInputPortMap;

    std::vector<AudioNode*> objectsSorted;

    // Only for sorting
    std::vector<unsigned int> zeroInDegreeNodes;
    std::vector<unsigned int> inDegree;

    bool addAdjacency(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto outputKey = AdjacencyMap::packKey(oNode, oPort);
        auto inputKey = AdjacencyMap::packKey(iNode, iPort);

        adjacencyMap.addAdjacency(inputKey, outputKey);
        return true;
    }

    AdjacencyMap adjacencyMap;
    NodeContext* context;
};

class GraphHolder
{
    std::vector<std::shared_ptr<Connection>> connections;
    std::vector<std::shared_ptr<AudioNode>> objects;

    ankerl::unordered_dense::map<std::string, uint32_t> objectIDMap;

    NodeContext* context;

    std::vector<std::shared_ptr<AudioNode>> removedObjects;

public:

    std::unique_ptr<AudioGraph> graph;

    GraphHolder(NodeContext* ctx)
        : context(ctx)
    {
        graph = std::make_unique<AudioGraph>(ctx);
    };

    GraphHolder(const GraphHolder* other)
    : context(other->context) // Reuse the same context
    , objects(other->objects)
    , objectIDMap(other->objectIDMap)
    , connections(other->connections)
    , graph(std::make_unique<AudioGraph>(context))
{
}

    AudioGraph* getGraph() const
    {
        return graph.get();
    }

    const json graphToJSON() const
    {
        json nodes = json::array();
        for (const auto& obj : objects)
        {
            auto node = obj->nodeCreationData;
            node["id"] = obj->nodeID; // Update the id field (as it could have changed)
            nodes.push_back(node);
        }

        json conns = json::array();
        for (const auto& connection : connections)
        {
            json conn;
            conn["sourceNode"] = connection->getoNode();
            conn["sourcePort"] = connection->getoPort();
            conn["targetNode"] = connection->getiNode();
            conn["targetPort"] = connection->getiPort();
            conns.push_back(conn);
        }

        json patch = json::array({
            { "nodes", nodes },
            { "connections", conns }
        });

        return patch;
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

            std::cout << "connecting: (" << source << " id: " << objects[source]->nodeID << ") -> (" << target << " id: " << objects[target]->nodeID << ")" << std::endl;

            // connections use unique ID's for nodes
            connect(objects[source]->nodeID, connection["sourcePort"], objects[target]->nodeID, connection["targetPort"]);
        }

        updateConnections();

        sortNodes();

        updateOutputInputPortMap();

        if (logVerbose)
        {
            printAdjacencyList();
        }
    }

    void updateOutputInputPortMap()
    {
        auto start = std::chrono::high_resolution_clock::now();

        graph->outputInputPortMap.reserve(objects.size());
        graph->outputInputPortMap.clear();

        // Create a mapping from original object IDs to their indices in `objects`
        ankerl::unordered_dense::map<int, size_t> objectIDtoOriginalIndex;

        for (size_t i = 0; i < objects.size(); ++i)
        {
            objectIDtoOriginalIndex[objects[i]->nodeID] = i;
        }

        // Create a mapping from original object IDs to their indices in `objectsSorted`
        ankerl::unordered_dense::map<int, size_t> objectIDtoSortedIndex;

        for (size_t sortedIndex = 0; sortedIndex < graph->objectsSorted.size(); ++sortedIndex)
        {
            objectIDtoSortedIndex[graph->objectsSorted[sortedIndex]->nodeID] = sortedIndex;
        }

        // Populate the outputInputPortMap
        for (size_t sortedIndex = 0; sortedIndex < graph->objectsSorted.size(); ++sortedIndex)
        {
            // Find the original index of the sorted object
            int objectID = graph->objectsSorted[sortedIndex]->nodeID;
            if (objectIDtoOriginalIndex.find(objectID) == objectIDtoOriginalIndex.end())
            {
                // Object ID not found in the original index map, skip
                continue;
            }

            size_t originalIndex = objectIDtoOriginalIndex[objectID];

            // Add a new vector for this object's inputs
            graph->outputInputPortMap.emplace_back();

            for (size_t j = 0; j < objects[originalIndex]->getNumInputs(); ++j)
            {
                // Collect upstream ports for the current input
                std::vector<AudioPort*> upstreamPorts;

                auto key = AdjacencyMap::packKey(originalIndex, j);
                const auto& backwardConnections = graph->adjacencyMap.getBackward();

                if (backwardConnections.find(key) != backwardConnections.end())
                {
                    for (const auto& outputPortUpstreamNode : backwardConnections.at(key))
                    {
                        auto connectedNode = AdjacencyMap::unpackKey(outputPortUpstreamNode);

                        // Find the sorted index of the connected node
                        int connectedObjectID = objects[connectedNode.first]->nodeID;
                        if (objectIDtoSortedIndex.find(connectedObjectID) != objectIDtoSortedIndex.end())
                        {
                            size_t connectedSortedIndex = objectIDtoSortedIndex[connectedObjectID];
                            auto connectedPort = graph->objectsSorted[connectedSortedIndex]->getOutputPort();
                            upstreamPorts.push_back(connectedPort);
                        }
                    }
                }

                // Add a PortGroup for this input
                graph->outputInputPortMap.back().emplace_back(PortGroup{
                    static_cast<uint8_t>(j), std::move(upstreamPorts)
                });
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << "PortPointerMap took " << elapsedNs << " ns.\n";

//#define PORTPOINTER_DEBUG
#ifdef PORTPOINTER_DEBUG
        // Debug print
        for (size_t i = 0; i < graph->outputInputPortMap.size(); ++i)
        {
            std::cout << "Object " << i << " input ports:" << std::endl;

            for (const auto& portGroup : graph->outputInputPortMap[i])
            {
                std::cout << "  Input Port " << static_cast<int>(portGroup.inputPortNumber) << ":";

                if (portGroup.connectedPorts.empty())
                {
                    std::cout << " No connections" << std::endl;
                }
                else
                {
                    for (const auto& port : portGroup.connectedPorts)
                    {
                        // Assuming `AudioPort` has a `toString` method or similar to print its details
                        std::cout << " " << port; // Replace with `port->toString()` if such a method exists
                    }
                    std::cout << std::endl;
                }
            }
        }
#endif
    }

    void updateConnections()
    {
        graph->objectIDtoIndex.reserve(objects.size());
        graph->objectIDtoIndex.clear();

        for (size_t i = 0; i < objects.size(); i++)
        {
            graph->objectIDtoIndex[objects[i]->nodeID] = i;
        }

        graph->adjacencyMap.clear();

        for (const auto& conn : connections)
        {
            graph->addAdjacency(graph->objectIDtoIndex[conn->getoNode()], conn->getoPort(), graph->objectIDtoIndex[conn->getiNode()], conn->getiPort());
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
        auto newConnection = std::make_shared<Connection>(oNode, oPort, iNode, iPort);

        // Check if the connection already exists
        for (const auto& conn : connections)
        {
            if (conn->getHash() == newConnection->getHash())
            {
                std::cerr << "Warning: Connection already exists, not adding duplicate." << std::endl;
                return;
            }
        }

        // Add the new connection
        connections.push_back(newConnection);
    }

    // Remove connections from idString:port pairs
    bool disconnect(const std::string& oObj, int oPort, const std::string& iObj, int iPort) {
        if (objectIDMap.contains(oObj) && objectIDMap.contains(iObj))
        {
            disconnect(objectIDMap[oObj], oPort, objectIDMap[iObj], iPort);
            return true;
        }
        return false;
    }

    // Remove connections with the object index
    void disconnect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto toRemove = Connection::encodeHash(oNode, oPort, iNode, iPort);
        std::erase_if(connections, [toRemove](const auto& connection) {
            return connection->getHash() == toRemove; // Predicate to match the connection to remove
        });
    }

    void removeObject(unsigned int nodeID)
    {
        // to remove an object, we can't delete it straight away
        // as it's pointer is still used in the graph
        // So we remove it from the objects list, and place it in a 'removed list'
        // then after this we will re-build the transitioning graph with the
        // object removed

        auto removeResult = std::ranges::remove_if(objects,
            [nodeID](const std::shared_ptr<AudioNode>& obj)
            {
                return obj->nodeID == nodeID;
            }
        );

        auto newEnd = removeResult.begin();

        // 2) Move the removed items to removedObjects.
        for (auto it = newEnd; it != objects.end(); ++it)
        {
            removedObjects.push_back(std::move(*it));
        }

        // 3) Erase them from the original vector.
        objects.erase(newEnd, objects.end());

        // connections can be deleted straight away, as the transitioning graph
        // rebuilds it's connections completely

        // find connections that are connected to this node
        // remove them all

        std::erase_if(connections, [nodeID](const auto& con) {
            return con->getiNode() == nodeID || con->getoNode() == nodeID;
        });
    }

    void process(float* buffer, unsigned long frameCount)
    {
        graph->process(buffer, frameCount);
    }

    void sortNodes()
    {
        graph->sortNodes(objects);
    }

    void printAdjacencyList()
    {
        graph->printAdjacencyList();
    }

    void setSummingFunctionForNode(AudioNode* node)
    {
        auto nodeID = node->nodeID;
        node->sumInputBuffers = [nodeID](const std::vector<std::unique_ptr<AudioPort>>& inputPorts, const AudioGraph& runningGraph, const int index)
        {
#define USE_POINTER_MAP
#ifdef USE_POINTER_MAP
            const auto frameCount = runningGraph.context->frameCount;

            // Retrieve the input port map for the current node
            const auto& portGroups = runningGraph.outputInputPortMap[index];

            for (const auto& portGroup : portGroups)
            {
                const auto portID = portGroup.inputPortNumber;
                auto& port = inputPorts[portID];

                const bool isPortSignal = port->isSignal();

                if (isPortSignal)
                {
                    // Clear and resize audio buffer only for signal ports
                    port->setSize(frameCount);
                }

                port->clearEvents();
                auto& summingEventBuffer = port->getEvents();
                auto summingAudioBuffer = port->getAudioBuffer();

                for (size_t connIndex = 0; connIndex < portGroup.connectedPorts.size(); ++connIndex)
                {
                    auto* connection = portGroup.connectedPorts[connIndex];
                    const auto outputBuffer = connection->getAudioBuffer();

                    if (isPortSignal && connection->isSignal())
                    {
                        // Update port status, any connected signal overrides events
                        port->isAnyConnectedPortSignal = true;

                        if (connIndex == 0) // Check if this is the first connection
                        {
                            // For the first connection, perform direct assignment
                            std::copy(outputBuffer, outputBuffer + frameCount, summingAudioBuffer);
                        }
                        else
                        {
                            // For subsequent connections, sum the buffer
                            for (size_t i = 0; i < frameCount; ++i)
                            {
                                summingAudioBuffer[i] += outputBuffer[i];
                            }
                        }
                    }

                    // Collect and merge events
                    auto& events = connection->getEvents();
                    if (!events.empty())
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
#else
            // This method uses the adjacency map to look up the connected AudioPort to sum from
            // It uses an unorerdered map to find the correct AudioPort in realtime
            // However, we can do this when we construct the graph, and allow the running graph
            // to use the cached/baked vector
            // Any change to the graph will happen at the 'next' cycle
            // Which allows us time to construct the graph on another thread anyway

            const auto frameCount = runningGraph.context->frameCount;
            auto indexIt = runningGraph.objectIDtoIndex.find(nodeID);
            if (indexIt == runningGraph.objectIDtoIndex.end()) {
                // If nodeID wasn't found in the map, skip
                return;
            }

            // If found, the key is indexIt->second
            auto nodeIndex = indexIt->second;

            for (size_t portID = 0; portID < inputPorts.size(); ++portID)
            {
                auto& port = inputPorts[portID];
                const bool isPortSignal = port->isSignal();

                if (isPortSignal)
                {
                    // Clear and resize audio buffer only for signal ports
                    port->setSize(frameCount);
                }

                port->clearEvents();
                auto& summingEventBuffer = port->getEvents();
                auto summingAudioBuffer = port->getAudioBuffer();

                auto key = AdjacencyMap::packKey(nodeIndex, portID);

                // Retrieve connections for the current port
                auto it = runningGraph.adjacencyMap.getBackward().find(key);
                if (it == runningGraph.adjacencyMap.getBackward().end())
                {
                    continue;
                }

                bool isFirstConnection = true; // Track if this is the first connection
                for (uint32_t connKey : it->second)
                {
                    auto connectedNode = runningGraph.objectsListCopy[AdjacencyMap::getNodeID(connKey)];
                    auto connection = connectedNode->getOutputPort();
                    const auto outputBuffer = connection->getAudioBuffer();

                    if (isPortSignal && connection->isSignal()) {
                        // Update port status, any connected signal overrides events
                        port->isAnyConnectedPortSignal = true;

                        if (isFirstConnection) {
                            // For the first connection, perform direct assignment
                            std::copy(outputBuffer, outputBuffer + frameCount, summingAudioBuffer);
                            isFirstConnection = false;
                        }
                        else {
                            // For subsequent connections, sum the buffer
                            for (size_t i = 0; i < frameCount; ++i) {
                                summingAudioBuffer[i] += outputBuffer[i];
                            }
                        }
                    }

                    // Collect and merge events
                    auto& events = connection->getEvents();
                    if (!events.empty())
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
#endif
        };
    }

    template <typename NodeType>
    void addNode(const std::optional<std::string>& idString, json& nodeCreationData) {

        auto node = std::make_unique<NodeType>(context, nodeCreationData);

        const auto nodeID = generateID();

        node->nodeID = nodeID;

        // Determine the ID to use for the object ID map
        const std::string finalID = idString.has_value() ? idString.value() : std::to_string(nodeID);  // Fallback to node ID

        objectIDMap[finalID] = nodeID;

        // Function responsible for summing audio & event buffers of connected inputs for each node.
        // This dynamically looks up the connections port via the connection table.
        // TODO: cache the connected port, and only recalculate if flag is set
        setSummingFunctionForNode(node.get());
        objects.push_back(std::move(node));
    };

    uint32_t generateID() const
    {
        std::set<unsigned int> usedIDs;
        for (const auto& obj : objects) {
            usedIDs.insert(obj->nodeID); // Assuming objects have a member nodeID
        }

        // Find the lowest unused ID starting from 0
        unsigned int idCounter = 0;
        while (usedIDs.find(idCounter) != usedIDs.end()) {
            idCounter++; // Increment until an unused ID is found
        }

        return idCounter;
    }

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
                addNode<Add>(idString, node);
            }
            break;
        case hash("count"):
            {
                addNode<Count>(idString, node);
            }
            break;
        case hash("print"):
            {
                addNode<Print>(idString, node);
            }
            break;
        case hash("if"):
            {
                addNode<If>(idString, node);
            }
            break;
        case hash("env"):
        case hash("envelope"):
            {
                addNode<Envelope>(idString, node);
            }
            break;
        case hash("metro"):
        case hash("metronome"):
            {
                addNode<Metronome>(idString, node);
            }
            break;
        case hash("val"):
        case hash("value"):
            {
                addNode<Value>(idString, node);
            }
            break;
        case hash("lfo"):
            {
                addNode<LFO>(idString, node);
            }
            break;
        case hash("volume"):
            {
                addNode<Volume>(idString, node);
            }
            break;
        case hash("osc"):
        case hash("oscillator"):
            {
                addNode<Oscillator>(idString, node);
            }
            break;
        case hash("aout"):
        case hash("audioout"):
            {
                addNode<AudioOut>(idString, node);
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
        for (const auto& obj : objects)
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

    bool addObject(const json& jsonObj)
    {
        if (!activeGraph)
        {
            activeGraph = std::make_unique<GraphHolder>(ctx);
        }

        // TODO: Lock the graph, or communicate via a queue

        return activeGraph->addObject(jsonObj);
    }

    void removeObject(int id)
    {
        if (!activeGraph)
            return;

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

        transitioningGraph->removeObject(id);

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
    }

    bool connect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
        std::cout << "== adding connection from CLI" << std::endl;
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

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

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

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

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

    const json graphToJSON()
    {
        return activeGraph->graphToJSON();
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