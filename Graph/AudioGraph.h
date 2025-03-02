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
#include "Edge.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
public:
    explicit AudioGraph(NodeContext* context)
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

        std::cout << objectList.size() << " objects in graph " <<  objectsSorted.size() << " objects sorted, sort took " << elapsedNs << " ns" << std::endl;
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
    std::vector<std::shared_ptr<Edge>> connections;
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

    std::vector<AudioNode*> getObjects() const
    {
        std::vector<AudioNode*> nodes;
        for (auto& node : objects)
        {
            nodes.push_back(node.get());
        }
        return nodes;
    }

    const json graphToJSON() const
    {
        ankerl::unordered_dense::map<uint32_t, std::string> invertedMap;
        for (const auto& [name, id] : objectIDMap) {
            invertedMap[id] = name;
        }

        json nodes = json::array();
        for (const auto& obj : objects)
        {
            auto node = obj->getSerializedNode();
            node["id"] = invertedMap[obj->nodeID];
            node["pos"] = { obj->canvasPos.x, obj->canvasPos.y }; // Position will also have changed.
            nodes.push_back(node);
        }

        json conns = json::array();
        for (const auto& connection : connections)
        {
            json conn;
            conn["sourceNode"] = invertedMap[connection->getoNode()];
            conn["sourcePort"] = connection->getoPort();
            conn["targetNode"] = invertedMap[connection->getiNode()];
            conn["targetPort"] = connection->getiPort();
            conns.push_back(conn);
        }

        json patch;
        patch["nodes"] = nodes;
        patch["connections"] = conns;

        return patch;
    }

    void loadPatch(const json& patch, bool logVerbose)
    {
        // Create nodes
        for (const auto& node : patch["nodes"])
        {
            createObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"])
        {
            // source and target ID needs to be set in the file format
            uint32_t source = connection["sourceNode"].is_string() ? objectIDMap[connection["sourceNode"].get<std::string>()] : objectIDMap[std::to_string(connection["sourceNode"].get<int>())];
            uint32_t target = connection["targetNode"].is_string() ? objectIDMap[connection["targetNode"].get<std::string>()] : objectIDMap[std::to_string(connection["targetNode"].get<int>())];

            //std::cout << "connecting: (" << source <<  " -> " << target << ")" << std::endl;

            // connections use unique ID's for nodes
            // FIXME: Is this really correct? we use the overloaded connect to connect with the stringID
            connect(objects[source]->nodeID, connection["sourcePort"], objects[target]->nodeID, connection["targetPort"]);
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

        //std::cout << "PortPointerMap took " << elapsedNs << " ns.\n";

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

    std::vector<Edge*> getConnections() const
    {
        auto connectionCopy = std::vector<Edge*>();

        for (const auto& conn : connections)
        {
            connectionCopy.push_back(conn.get());
        }
        return connectionCopy;
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
            connectObjIndex(objectIDMap[oObj], oPort, objectIDMap[iObj], iPort);
            return true;
        }
        return false;
    }

    bool connect(int oNode, int oPort, int iNode, int iPort) {
        ankerl::unordered_dense::map<uint32_t, std::string> invertedMap;
        //std::cout << "=========== objectIDMap ==========" << std::endl;
        for (const auto& [name, id] : objectIDMap) {
            //std::cout << "id: " << id << " name: " << name << std::endl;
            invertedMap[id] = name;
        }

        if (invertedMap.contains(oNode) && invertedMap.contains(iNode))
        {
            connect(invertedMap[oNode], oPort, invertedMap[iNode], iPort);
            return true;
        }
        return false;
    }

    // Create connections with the object index
    void connectObjIndex(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto newConnection = std::make_shared<Edge>(oNode, oPort, iNode, iPort);

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

    // remove the connection using the connection hash
    void removeEdge(uint64_t connEdgeHash)
    {
        std::erase_if(connections, [connEdgeHash](const auto& connection)
        {
            return connection->getHash() == connEdgeHash; // Predicate to match the connection to remove
        });
    }

    // Remove connections with the object index
    void disconnect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto toRemove = Edge::encodeHash(oNode, oPort, iNode, iPort);
        removeEdge(toRemove);
    }

    void removeObject(unsigned int nodeID)
    {
        // 1) Remove the object from the objects vector and move it to removedObjects.
        auto removeResult = std::ranges::remove_if(objects,
            [nodeID](const std::shared_ptr<AudioNode>& obj)
            {
                return obj->nodeID == nodeID;
            }
        );
        auto newEnd = removeResult.begin();

        // Move removed objects into removedObjects.
        for (auto it = newEnd; it != objects.end(); ++it)
        {
            removedObjects.push_back(std::move(*it));
        }
        objects.erase(newEnd, objects.end());

        // 2) Find and remove the entry from objectIDMap by matching the value.
        auto mapIt = std::find_if(objectIDMap.begin(), objectIDMap.end(),
            [nodeID](const auto& entry) { return entry.second == nodeID; }
        );
        if (mapIt != objectIDMap.end()) {
            objectIDMap.erase(mapIt);
        }

        // 3) Remove any connections associated with this node.
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
                    // Clear audio buffer only for signal ports
                    port->clear(frameCount);
                }

                port->clearEvents();
                auto& summingEventBuffer = port->getEvents();
                auto summingAudioBuffer = port->getAudioBuffer();

                // Reset this port incase it has been disconnected
                // TODO: move this outside of process - do it in graph construction!
                port->isAnyConnectedPortSignal = false;

                // Track the first signal connection, we use direct copy of the buffer here
                // As this saves CPU for single connection ports
                bool firstConnection = true;

                for (size_t connIndex = 0; connIndex < portGroup.connectedPorts.size(); ++connIndex)
                {
                    auto* connection = portGroup.connectedPorts[connIndex];
                    const auto outputBuffer = connection->getAudioBuffer();

                    if (isPortSignal && connection->isSignal())
                    {
                        // Update port status, any connected signal overrides events
                        port->isAnyConnectedPortSignal = true;

                        if (firstConnection) {
                            std::copy(outputBuffer, outputBuffer + frameCount, summingAudioBuffer);
                            firstConnection = false;
                        }
                        else {
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
            // It uses an unordered map to find the correct AudioPort in realtime
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
    AudioNode* addNode(const std::optional<std::string>& idString, json& nodeCreationData)
    {
        auto node = std::make_unique<NodeType>(context, nodeCreationData);
        auto rawNode = node.get();

        if (nodeCreationData.contains("pos") && nodeCreationData["pos"].is_array() &&
            nodeCreationData["pos"].size() >= 2)
        {
            node->canvasPos = pptk::Point(
                nodeCreationData["pos"][0].get<float>(),
                nodeCreationData["pos"][1].get<float>()
            );
        }

        const auto nodeID = generateID();

        // Preserve the custom id from the JSON if provided, else use nodeID converted to a string.
        const std::string finalID = (idString.has_value() && !idString->empty())
            ? idString.value()
            : std::to_string(nodeID);
        objectIDMap[finalID] = nodeID;

        node->nodeID = nodeID;
        std::cout << "node id: "  << nodeID << " id string: " << finalID << std::endl;
        node->nodeIDString = finalID;

        setSummingFunctionForNode(node.get());
        objects.push_back(std::move(node));

        return rawNode;
    }


    uint32_t generateID() const
    {
        std::set<unsigned int> usedIDs;
        for (const auto& obj : objects) {
            usedIDs.insert(obj->nodeID);
        }

        // Find the lowest unused ID starting from 0
        unsigned int idCounter = 0;
        while (usedIDs.find(idCounter) != usedIDs.end()) {
            idCounter++; // Increment until an unused ID is found
        }

        return idCounter;
    }

    AudioNode* createObject(json node, bool addToGraph = true)
    {
        if (node.is_null())
            return nullptr;

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
            return addNode<Add>(idString, node);

        case hash("count"):
            return addNode<Count>(idString, node);

        case hash("dial"):
            return addNode<Dial>(idString, node);

        case hash("print"):
            return addNode<Print>(idString, node);

        case hash("if"):
            return addNode<If>(idString, node);

        case hash("env"):
        case hash("envelope"):
            return addNode<Envelope>(idString, node);

        case hash("metro"):
        case hash("metronome"):
            return addNode<Metronome>(idString, node);

        case hash("val"):
        case hash("value"):
            return addNode<Value>(idString, node);

        case hash("lfo"):
            return addNode<LFO>(idString, node);

        // TODO: Remove old name, but leave in vol & volume for now
        case hash("vol"):
        case hash("volume"):
        case hash("gain"):
            return addNode<Gain>(idString, node);

        case hash("osc"):
        case hash("oscillator"):
            return addNode<Oscillator>(idString, node);

        case hash("aout"):
        case hash("audioout"):
            return addNode<AudioOut>(idString, node);

        case hash("floatbox"):
            return addNode<FloatBox>(idString, node);

        case hash("ping"):
            return addNode<Ping>(idString, node);

        case hash("scope"):
            return addNode<Scope>(idString, node);

        case hash("spec"):
            return addNode<Spec>(idString, node);

        case hash("bpf"):
        case hash("bandpassfilter"):
            return addNode<BandPassFilter>(idString, node);

        default:
            // Unknown object name, return error
            std::cout << "Unknown object: " << object << std::endl;
            return nullptr;
        }
    };

    // Get the json string for only the selected nodes and the selected objects interconnected connections
    json serializeSelectedNodes(const std::vector<uint32_t>& selectedNodeIDs) const
    {
        json nodes = json::array();
        json conns = json::array();

        // Create a set for quick lookup of selected node IDs
        std::set<uint32_t> selectedNodesSet(selectedNodeIDs.begin(), selectedNodeIDs.end());

        // Build an inverted map from nodeID to its string ID (if available)
        ankerl::unordered_dense::map<uint32_t, std::string> invertedMap;
        for (const auto& [name, id] : objectIDMap)
        {
            invertedMap[id] = name;
        }

        // Serialize nodes whose nodeID is in the selected set.
        for (const auto& nodePtr : objects)
        {
            if (selectedNodesSet.find(nodePtr->nodeID) != selectedNodesSet.end())
            {
                json nodeJson = nodePtr->getSerializedNode();
                // Use the string ID if available, otherwise fall back to nodeID as a string.
                nodeJson["id"] = invertedMap.contains(nodePtr->nodeID)
                                     ? invertedMap.at(nodePtr->nodeID)
                                     : std::to_string(nodePtr->nodeID);
                nodeJson["pos"] = {nodePtr->canvasPos.x, nodePtr->canvasPos.y};
                nodes.push_back(nodeJson);
            }
        }

        // Serialize connections only if both source and target nodes are selected.
        for (const auto& conn : connections)
        {
            if (selectedNodesSet.find(conn->getoNode()) != selectedNodesSet.end() &&
                selectedNodesSet.find(conn->getiNode()) != selectedNodesSet.end())
            {
                json connJson;
                connJson["sourceNode"] = invertedMap.contains(conn->getoNode())
                                             ? invertedMap.at(conn->getoNode())
                                             : std::to_string(conn->getoNode());
                connJson["sourcePort"] = conn->getoPort();
                connJson["targetNode"] = invertedMap.contains(conn->getiNode())
                                             ? invertedMap.at(conn->getiNode())
                                             : std::to_string(conn->getiNode());
                connJson["targetPort"] = conn->getiPort();
                conns.push_back(connJson);
            }
        }

        json patch;
        patch["nodes"] = nodes;
        patch["connections"] = conns;
        return patch;
    }

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
    GraphManager(int sampleRate, unsigned long frameCount)
        : ctx(std::make_unique<NodeContext>(sampleRate, frameCount))
    {
        Logger::getInstance().startProcessingThread();
    }

    ~GraphManager()
    {
        Logger::getInstance().stopProcessingThread();
    }

    AudioNode* addObject(const std::string& objName, bool addToGraph = true)
    {
        if (!activeGraph)
        {
            activeGraph = std::make_unique<GraphHolder>(ctx.get());
        }

        // TODO: Lock the graph, or communicate via a queue

        json object;
        object["obj"] = objName;
        return activeGraph->createObject(object, addToGraph);
    }

    AudioNode* addObject(const json& jsonObj)
    {
        std::cout << "adding object from UI" <<  std::endl;

        if (!activeGraph)
        {
            activeGraph = std::make_unique<GraphHolder>(ctx.get());
        }

        // TODO: Lock the graph, or communicate via a queue

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

        auto newNode = transitioningGraph->createObject(jsonObj);

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);

        return newNode;
    }

    std::vector<Edge*> removeObject(int id)
    {
        if (!activeGraph)
            return { };

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

        transitioningGraph->removeObject(id);

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        auto connectionState = transitioningGraph->getConnections();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);

        return connectionState;
    }

    std::vector<Edge*> removeObjects(std::vector<int>& ids, std::vector<uint64_t>& edgeHashes)
    {
        if (!activeGraph)
            return { };

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

        for (auto id : ids)
        {
            transitioningGraph->removeObject(id);
        }

        for (auto edgeHash : edgeHashes)
        {
            transitioningGraph->removeEdge(edgeHash);
        }

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        auto connectionState = transitioningGraph->getConnections();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);

        return connectionState;
    }

    std::vector<Edge*> connect(const int oObj, int oPort, const int iObj, int iPort)
    {
        std::cout << "connecting from UI: " << std::endl;
        if (!activeGraph) {
            std::cerr << "No active graph available to connect objects." << std::endl;
            return { };
        }

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());
        // Add the connection to the transitioning graph
        if (!transitioningGraph->connect(oObj, oPort, iObj, iPort)) {
            std::cerr << "Failed to connect objects in the transitioning graph." << std::endl;
            transitioningGraph.reset(); // Discard transitioning graph
            return activeGraph->getConnections();
        }

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        auto allConnections = transitioningGraph->getConnections();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
        return allConnections;
    }

    std::vector<Edge*> connectMultiple(std::vector<std::tuple<int, int, int, int>> conns)
    {
        std::cout << "connecting from UI: " << std::endl;
        if (!activeGraph) {
            std::cerr << "No active graph available to connect objects." << std::endl;
            return { };
        }

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

        int failedCount = 0;

        // Add the connection to the transitioning graph
        for (auto& [oObj, oPort, iObj, iPort] : conns)
        {
            if (!transitioningGraph->connect(oObj, oPort, iObj, iPort))
            {
                failedCount++;
            }
        }

        if (failedCount == conns.size())
        {
            std::cerr << "Failed to multi-connect any objects in the transitioning graph." << std::endl;
            transitioningGraph.reset(); // Discard transitioning graph
            return activeGraph->getConnections();
        }

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        auto allConnections = transitioningGraph->getConnections();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
        return allConnections;
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

    std::tuple<std::vector<Object*>, std::vector<Edge*>> setActiveGraph(const std::string& patchPath, const json& patch, const bool logVerbose) {
        if (swapGraph.load(std::memory_order_acquire)) {
            std::cout << "Warning: Attempted to overwrite a transitioning graph before it was swapped." << std::endl;
            return { };
        }

        filePath = patchPath;

        transitioningGraph = std::make_shared<GraphHolder>(ctx.get());

        transitioningGraph->loadPatch(patch, logVerbose);

        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        if (logVerbose)
        {
            transitioningGraph->printAdjacencyList();
        }

        auto loadedObjects = getObjects();
        auto connections = transitioningGraph->getConnections();

        swapGraph.store(true, std::memory_order_release);

        return { loadedObjects, connections };
    }

    std::vector<Object*> getObjects()
    {
        std::vector<Object*> objects;

        for (auto* aNode : transitioningGraph->getObjects())
        {
            if (auto object = reinterpret_cast<Object*>(aNode->getOrCreateUI()))
            {
                objects.push_back(object);
            }
        }
        return objects;
    }

    const std::string& getPatchFile()
    {
        return filePath;
    }

    const json graphToJSON()
    {
        return activeGraph->graphToJSON();
    }

    const json copySelectedToClipboard(const std::vector<uint32_t>& selectedNodeIDs)
    {
        if (activeGraph)
        {
            return activeGraph->serializeSelectedNodes(selectedNodeIDs);
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

        processPeak(buffer, frameCount);

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

    // Queue size would be largest 8 if 64 buffrer size at 44100 hz and a video refresh rate of 120 hz
    moodycamel::ConcurrentQueue<float> volumeMeterQueue = moodycamel::ConcurrentQueue<float>(100);

private:

    // Take the average peak and send it to the GUI when the GUI requests an update
    void processPeak(const float* buffer, unsigned long frameCount)
    {
        float peak = 0.0f;
        for (unsigned long i = 0; i < frameCount; i++)
        {
            peak = std::max(peak, std::abs(buffer[i]));
        }

        volumeMeterQueue.enqueue(peak);
    }

protected:
    std::string filePath;

    std::shared_ptr<GraphHolder> activeGraph;         // Actively processed graph
    std::shared_ptr<GraphHolder> transitioningGraph;  // New graph prepared for swapping
    std::atomic<bool> swapGraph = false;             // Signal for readiness to swap
    std::unique_ptr<NodeContext> ctx;
};