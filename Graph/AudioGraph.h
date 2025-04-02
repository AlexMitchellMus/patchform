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
#include <UI_ToolKit/PlatformHelpers.h>

#include "json.hpp"
using json = nlohmann::json;

#include "glaze/glaze.hpp"

#include "unordered_dense.h"

#include "../Utility/Hash.h"
#include "../Nodes/AllNodes.h"

//#include "Logger.h"
#include "../Utility/ppl_string.hpp"
#include "AdjacencyMap.h"
#include "Edge.h"

#include "DspTimer.h"

#undef max

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph
{
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
                        std::string leftSide = namePart + " " + std::to_string(objectsListCopy[oNode]->nodeID) +
                            ", Port " + std::to_string(oPort);

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
                        std::cout << std::setw(maxNameWidth) << std::left << namePart << " " <<
                            std::setw(maxLeftWidth - maxNameWidth) << (nodeAsID + ", Port " + std::to_string(oPort)) <<
                            " -> ";

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
                                std::cout << "[" << objectsListCopy[iNode]->getShortName() << "] " << iNodeAsID <<
                                    ", Port " << std::to_string(iPort) << "] ";
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

        sortedNodes.clear();
        sortedNodes.reserve(nodeCount);

        zeroInDegreeNodes.clear();
        zeroInDegreeNodes.reserve(nodeCount);

        inDegree.assign(nodeCount, 0);

        // Compute in-degrees directly from the forward map: for every outgoing edge,
        // increment the in-degree of its destination node.
        for (const auto& [sourceKey, downstreamKeys] : adjacencyMap.getForward())
        {
            for (const auto& downstreamKey : downstreamKeys)
            {
                int downstreamNodeIndex = AdjacencyMap::getNodeID(downstreamKey);
                if (downstreamNodeIndex >= 0 && downstreamNodeIndex < nodeCount)
                {
                    ++inDegree[downstreamNodeIndex];
                }
            }
        }

        // Collect all nodes with zero in-degree.
        for (unsigned int i = 0; i < nodeCount; ++i)
        {
            if (inDegree[i] == 0)
            {
                zeroInDegreeNodes.push_back(i);
            }
        }

        // Process nodes in topological order.
        size_t processIndex = 0;
        while (processIndex < zeroInDegreeNodes.size())
        {
            const auto currentIndex = zeroInDegreeNodes[processIndex++];
            sortedNodes.push_back(objectsListCopy[currentIndex]);

            // For every outgoing edge from currentIndex (from any port),
            // find all downstream nodes and decrement their in-degree.
            for (const auto& [sourceKey, downstreamKeys] : adjacencyMap.getForward())
            {
                if (AdjacencyMap::getNodeID(sourceKey) == static_cast<int>(currentIndex))
                {
                    for (const auto& downstreamKey : downstreamKeys)
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
        }

        // If we haven't processed every node, there's a cycle.
        if (sortedNodes.size() != nodeCount)
        {
            sortedNodes.clear();
            std::cout << "Cycle detected, clearing graph" << std::endl;
        }
    }

    void sortNodes(const std::vector<std::shared_ptr<AudioNode>>& objectList)
    {
//#define LOG_GRAPH_INFO
#ifdef LOG_GRAPH_INFO
        auto start = std::chrono::high_resolution_clock::now();
#endif

        objectsListCopy.reserve(objectList.size());
        objectsListCopy.clear();

        for (auto& obj : objectList)
        {
            objectsListCopy.push_back(obj.get());
        }

        topologicalSort(objectsSorted);

        // Build a word list of 64 bit words that hold a representation
        // of all the nodes that need to be processed without skipping
        // We use this so we can completely skip nodes that don't need to be processed,
        // instead of testing each one in the process loop.
        activeAudioNodes.clear();
        activeAudioNodes.assign((objectsSorted.size() + 63) / 64, 0);

        // clear and assign an empty event node bitfield word vector
        activeEventNodes.clear();
        activeEventNodes.assign((objectsSorted.size() + 63) / 64, 0);

        for (unsigned i = 0; i < objectsSorted.size(); ++i)
        {
            const auto obj = objectsSorted[i];

            // Add all persistent processing objects to the active bitfields
            // These objects will always process regardless if they have events or not
            if (obj->alwaysProcess())
            {
                activeAudioNodes[i >> 6] |= (1ULL << (i & 63)); // i / 64, i % 64
            }

            // Check if the object needs to process once on load (used for LoadEvent currently)
            // The flag is set when constructing the node (on UI thread) and
            // this runs after all nodes are constructed on the same UI thread.
            if (obj->eventOnLoad)
            {
                activeEventNodes[i >> 6] |= (1ULL << (i & 63)); // i / 64, i % 64
                obj->eventOnLoad = false;
            }
        }

#ifdef DEBUG_AUDIO_NODES_BITFIELDS
        std::cout << "=== Bit fields for active audio processes ===" << std::endl;
        for (size_t i = 0; i < activeAudioNodes.size(); ++i)
        {
            std::bitset<64> bits(activeAudioNodes[i]);
            std::cout << "Word " << i << ": \t" << bits << "\n";
        }
#endif

#ifdef LOG_GRAPH_INFO
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        std::cout << objectList.size() << " objects in graph " << objectsSorted.size() << " objects sorted, sort took "
            << elapsedNs << " ns" << std::endl;
#endif

//#define DEBUG_SORT
#ifdef DEBUG_SORT
        std::cout << "======== presort =======" << std::endl;
        for (auto& node : objectsListCopy)
        {
            std::cout << "node: " << node->getName() << std::endl;
        }

        std::cout << "======== sorted =======" << std::endl;
        for (auto node : objectsSorted)
            std::cout << "node graph: " << node->getName() << std::endl;
#endif
    }


    void process(const float* inBuffer, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage)
    {
#define SKIP_PROCESSING
#ifdef SKIP_PROCESSING
        std::function<void(AudioGraph&)> msg;
        while (context->messageQueue.try_dequeue(msg))
            msg(*this);

        unsigned i = 0;

        while (i < objectsSorted.size())
        {
            size_t word = i >> 6; // i / 64
            const size_t bit = i & 63; // i & 64

            if (const uint64_t combined = (activeEventNodes[word] | activeAudioNodes[word]) >> bit)
            {
                const auto offset = std::countr_zero(combined);
                i += offset;

                if (i >= objectsSorted.size())
                    break;

                objectsSorted[i]->process(inBuffer, buffer, midiMessage, frameCount, *this, i);

                activeEventNodes[i >> 6] &= ~(1ULL << (i & 63)); // i / 64, i % 64
                ++i;
            }
            else
            {
                // Skip to next word with set bits
                ++word;
                while (word < activeAudioNodes.size() && (activeEventNodes[word] | activeAudioNodes[word]) == 0)
                {
                    ++word;
                }

                i = word << 6; // word * 64
            }
        }
#else
        // simple loop (everything gets processed)
        for (size_t i = 0; i < objectsSorted.size(); i++)
        {
            objectsSorted[i]->process(buffer, frameCount, *this, i);
        }
#endif
        context->eventPool.releaseAllEvents();
    }

    bool flagForDeletion = false;

    std::vector<AudioNode*> objectsListCopy;

    ankerl::unordered_dense::map<uint32_t, uint32_t> objectIDtoIndex;

    OutputPortMap outputInputPortMap;
    DownStreamPortMap downstreamPortMap;

    std::vector<AudioNode*> objectsSorted;

    // bit field vector to hold which nodes are active for optimized processing
    // We have a static list (that doesn't change per cycle) of all nodes that are audio
    // And an event list that is updated during processing
    std::vector<uint64_t> activeAudioNodes;
    std::vector<uint64_t> activeEventNodes;

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
        std::vector<uint32_t> activeNodes;
        for (auto& node : objects)
        {
            activeNodes.push_back(node->nodeID);
        }

        return serializeSelectedNodes(activeNodes);
    }

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

    bool loadPatch(const json& patch, bool logVerbose)
    {
        // Create nodes
        for (const auto& node : patch["nodes"])
        {
            createObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"])
        {
            if ((connection["sourceNode"].is_string() && connection["sourceNode"].get<std::string>().empty()) ||
                (connection["targetNode"].is_string() && connection["targetNode"].get<std::string>().empty()))
                return false;
            // source and target ID needs to be set in the file format
            uint32_t source = connection["sourceNode"].is_string()
                                  ? objectIDMap[connection["sourceNode"].get<std::string>()]
                                  : objectIDMap[std::to_string(connection["sourceNode"].get<int>())];
            uint32_t target = connection["targetNode"].is_string()
                                  ? objectIDMap[connection["targetNode"].get<std::string>()]
                                  : objectIDMap[std::to_string(connection["targetNode"].get<int>())];

            //std::cout << "connecting: (" << source <<  " -> " << target << ")" << std::endl;

            // connections use unique ID's for nodes
            // FIXME: Is this really correct? we use the overloaded connect to connect with the stringID
            connect(objects[source]->nodeID, connection["sourcePort"], objects[target]->nodeID,
                    connection["targetPort"]);
        }
        return true;
    }

    void updateOutputInputPortMap()
    {
        //auto start = std::chrono::high_resolution_clock::now();

    // Reserve and clear the upstream map.
    graph->outputInputPortMap.clear();
    graph->outputInputPortMap.reserve(objects.size());

    // Also clear and resize the downstream map.
    graph->downstreamPortMap.clear();
    graph->downstreamPortMap.resize(graph->objectsSorted.size());

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

    // Populate the upstream port map (for summing audio)
    for (size_t sortedIndex = 0; sortedIndex < graph->objectsSorted.size(); ++sortedIndex)
    {
        int objectID = graph->objectsSorted[sortedIndex]->nodeID;
        if (objectIDtoOriginalIndex.find(objectID) == objectIDtoOriginalIndex.end())
        {
            continue;
        }
        size_t originalIndex = objectIDtoOriginalIndex[objectID];

        // Add a new vector for this object's inputs.
        graph->outputInputPortMap.emplace_back();

        for (size_t j = 0; j < objects[originalIndex]->getNumInputs(); ++j)
        {
            std::vector<AudioPort*> upstreamPorts;

            auto key = AdjacencyMap::packKey(originalIndex, j);
            const auto& backwardConnections = graph->adjacencyMap.getBackward();

            if (backwardConnections.find(key) != backwardConnections.end())
            {
                for (const auto& outputPortUpstreamNode : backwardConnections.at(key))
                {
                    auto connectedNode = AdjacencyMap::unpackKey(outputPortUpstreamNode);
                    int connectedObjectID = objects[connectedNode.first]->nodeID;
                    if (objectIDtoSortedIndex.find(connectedObjectID) != objectIDtoSortedIndex.end())
                    {
                        size_t connectedSortedIndex = objectIDtoSortedIndex[connectedObjectID];
                        auto connectedPort = graph->objectsSorted[connectedSortedIndex]->getOutputPort(connectedNode.second);
                        upstreamPorts.push_back(connectedPort);
                    }
                }
            }
            graph->outputInputPortMap.back().emplace_back(PortGroup{
                static_cast<uint8_t>(j), std::move(upstreamPorts)
            });
        }
    }

    // --- Populate the downstream port map ---
    // For each node in sorted order...
    for (size_t sortedIndex = 0; sortedIndex < graph->objectsSorted.size(); ++sortedIndex)
    {
        AudioNode* node = graph->objectsSorted[sortedIndex];
        int nodeID = node->nodeID;
        // For each output port on the node...
        for (size_t outPort = 0; outPort < node->outputPortBuffers.size(); ++outPort)
        {
            DownstreamPortGroup group;
            group.outputPortNumber = static_cast<uint8_t>(outPort);

            // Iterate over all connections in the graph.
            for (const auto& conn : connections)
            {
                // Check if this connection originates from this node and the current output port.
                if (conn->getoNode() == nodeID && conn->getoPort() == static_cast<int>(outPort))
                {
                    int targetID = conn->getiNode();
                    // Use objectIDtoSortedIndex to find the target node's sorted index.
                    if (objectIDtoSortedIndex.find(targetID) != objectIDtoSortedIndex.end())
                    {
                        size_t targetSortedIndex = objectIDtoSortedIndex[targetID];
                        AudioNode* targetNode = graph->objectsSorted[targetSortedIndex];
                        int targetPort = conn->getiPort();
                        group.downstreamConnections.emplace_back(targetNode, targetPort);
                    }
                }
            }
            // Only add the group if it contains at least one connection.
            if (!group.downstreamConnections.empty())
            {
                graph->downstreamPortMap[sortedIndex].push_back(std::move(group));
            }
        }
    }

        //auto end = std::chrono::high_resolution_clock::now();
        //auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

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
            graph->addAdjacency(graph->objectIDtoIndex[conn->getoNode()], conn->getoPort(),
                                graph->objectIDtoIndex[conn->getiNode()], conn->getiPort());
        }
    }

    // Create connections from idString:port pairs
    bool connect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
        if (objectIDMap.contains(oObj) && objectIDMap.contains(iObj))
        {
            connectObjIndex(objectIDMap[oObj], oPort, objectIDMap[iObj], iPort);
            return true;
        }
        return false;
    }

    bool connect(int oNode, int oPort, int iNode, int iPort)
    {
        //std::cout << "connecting: (" << oNode << " : " << oPort <<  " -> " << iNode << " : " << iPort << ")" << std::endl;

        ankerl::unordered_dense::map<uint32_t, std::string> invertedMap;

        //std::cout << "=========== objectIDMap ==========" << std::endl;
        for (const auto& [name, id] : objectIDMap)
        {
            //std::cout << "id: " << id << " name: " << name << std::endl;
            invertedMap[id] = name;
        }

        //std::cout << invertedMap.contains(oNode) << " " << invertedMap.contains(iNode) << std::endl;

        if (invertedMap.contains(oNode) && invertedMap.contains(iNode))
        {
            //std::cout << "connecting oNode " << oNode << " -> " << iNode << std::endl;
            connect(invertedMap[oNode], oPort, invertedMap[iNode], iPort);
            return true;
        }

        std::cerr << "issue connectiong: " << "connecting oNode " << oNode << " -> " << iNode << std::endl;
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
    bool disconnect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
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
        if (mapIt != objectIDMap.end())
        {
            objectIDMap.erase(mapIt);
        }

        // 3) Remove any connections associated with this node.
        std::erase_if(connections, [nodeID](const auto& con)
        {
            return con->getiNode() == nodeID || con->getoNode() == nodeID;
        });
    }


    void process(const float* inBuffer, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage)
    {
//#define DSP_FREE_ATOMS
#ifdef DSP_FREE_ATOMS
        std::cout << "--- free atoms: " << context->eventPool.getFreeListSize() << std::endl;
#endif
        graph->process(inBuffer, buffer, frameCount, midiMessage);

        for (auto obj : removedObjects)
        {
            obj->cleanupAudio();
        }
    }

    void sortNodes()
    {
        graph->sortNodes(objects);

        if (graph->objectsSorted.empty())
        {
            std::cerr << "Cycle detected, attempting to remove invalid connections..." << std::endl;

            std::vector<std::shared_ptr<Edge>> validConnections;

            for (const auto& conn : connections)
            {
                validConnections.push_back(conn);

                // Temporarily assign and test
                graph->adjacencyMap.clear();
                for (const auto& c : validConnections)
                {
                    graph->addAdjacency(graph->objectIDtoIndex[c->getoNode()], c->getoPort(),
                                        graph->objectIDtoIndex[c->getiNode()], c->getiPort());
                }

                graph->sortNodes(objects);

                if (graph->objectsSorted.empty())
                {
                    std::cerr << "Removed cycle-causing connection: "
                              << conn->getoNode() << ":" << conn->getoPort()
                              << " -> " << conn->getiNode() << ":" << conn->getiPort() << std::endl;

                    validConnections.pop_back();
                }
            }

            // Apply final valid connections
            connections = validConnections;
            updateConnections();
            graph->sortNodes(objects);
        }
    }


    void printAdjacencyList()
    {
        graph->printAdjacencyList();
    }

    void setSummingFunctionForNode(AudioNode* node)
    {
        auto nodeID = node->nodeID;

        node->setNodeDirty = [node]()
        {
            node->context->messageQueue.enqueue([node](AudioGraph& runningGraph)
            {
                auto& vec = runningGraph.objectsSorted;
                const auto it = std::ranges::find(vec, node);
                if (it == vec.end())
                    return; // not found

                const int index = static_cast<int>(std::distance(vec.begin(), it));
                runningGraph.activeEventNodes[index / 64] |= (1ULL << (index % 64));
            });
        };

        node->pushOutputEvents = [nodeID](const std::vector<std::unique_ptr<AudioPort>>& outputPorts, AudioGraph& runningGraph, const int index)
        {
#define USE_POINTER_MAP_PUSH
#define USE_POINTER_MAP

#ifdef USE_POINTER_MAP_PUSH
            // Retrieve the precomputed downstream port groups for this node.
            const auto& groups = runningGraph.downstreamPortMap[index];

            // Loop over each downstream group.
            for (const auto& group : groups)
            {
                const int outPort = group.outputPortNumber;
                auto& events = outputPorts[outPort]->getEvents();
                if (events.empty())
                    continue; // No events to push for this output port.

                // For every connection in this group, push each event.
                for (const auto& connection : group.downstreamConnections)
                {
                    AudioNode* target = connection.first;
                    const int targetPort = connection.second;
                    for (Event* event : events)
                    {
                        target->pushEvent(targetPort, event);

                        // set the corresponding bit of this node as needing processing
                        // in the process loop the node will then be processed
                        auto& vec = runningGraph.objectsSorted;
                        if (auto it = std::ranges::find(vec, target); it != vec.end())
                        {
                            const unsigned targetIndex = it - vec.begin();
                            runningGraph.activeEventNodes[targetIndex >> 6] |= (1ULL << (targetIndex & 63)); // targetIndex / 64, targetIndex % 64
                        }
                    }
                }
            }
        };
#else
            // Loop over each output port.
            for (size_t outPort = 0; outPort < outputPorts.size(); ++outPort) {
                // Get the events from this output port.
                auto& events = outputPorts[outPort]->getEvents();
                if (events.empty())
                    continue; // Nothing to push

                // Look up all downstream connections from this output port.
                // TODO: We want to use the same sort of map we made for summing audio, however this needs to be a downstream map (not upstream)
                auto key = AdjacencyMap::packKey(runningGraph.objectIDtoIndex.find(nodeID)->first, outPort);
                const auto& forward = runningGraph.adjacencyMap.getForward();
                auto it = forward.find(key);
                if (it == forward.end())
                    continue;

                // Push each event to every connected downstream node.
                for (auto downstreamKey : it->second) {
                    auto [targetIndex, targetPort] = AdjacencyMap::unpackKey(downstreamKey);
                    if (targetIndex < runningGraph.objectsListCopy.size()) {
                        AudioNode* target = runningGraph.objectsListCopy[targetIndex];
                        // Push event to the target node.
                        for (auto* event : events) {
                            target->pushEvent(targetPort, event);
                        }
                    }
                }
            }
        };
#endif

        node->sumInputBuffers = [nodeID](const std::vector<std::unique_ptr<AudioPort>>& inputPorts,
                                         const AudioGraph& runningGraph, const int index)
        {
#ifdef USE_POINTER_MAP
            const auto frameCount = runningGraph.context->frameCount;

            // Retrieve the input port map for the current node
            const auto& portGroups = runningGraph.outputInputPortMap[index];

            for (const auto& portGroup : portGroups)
            {
                const auto portID = portGroup.inputPortNumber;
                auto& port = inputPorts[portID];

                // Reset port status in case it has been disconnected
                port->isAnyConnectedPortSignal = false;

                if (!port->isSignal())
                {
                    continue;
                }

                // Clear the audio buffer
                port->clear(frameCount);

                auto summingAudioBuffer = port->getAudioBuffer();

                // Use direct copy for the first connected signal to save CPU cycles
                bool firstConnection = true;

                for (size_t connIndex = 0; connIndex < portGroup.connectedPorts.size(); ++connIndex)
                {
                    auto* connection = portGroup.connectedPorts[connIndex];
                    const auto outputBuffer = connection->getAudioBuffer();

                    if (connection->isSignal())
                    {
                        // Update port status: any connected signal overrides events
                        port->isAnyConnectedPortSignal = true;

                        if (firstConnection)
                        {
                            std::copy(outputBuffer, outputBuffer + frameCount, summingAudioBuffer);
                            firstConnection = false;
                        }
                        else
                        {
                            // Sum the buffer for subsequent connections
                            for (size_t i = 0; i < frameCount; ++i)
                            {
                                summingAudioBuffer[i] += outputBuffer[i];
                            }
                        }
                    }
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
    AudioNode* addNode(const std::string& finalID, json& nodeCreationData)
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

        // Generate a unique node ID
        uint32_t nodeID = generateID();

        // Assign both integer ID and string ID
        node->nodeID = nodeID;
        node->nodeIDString = finalID;

        // Ensure mapping is correct
        objectIDMap[finalID] = nodeID;

        //std::cout << "Node added: ID " << nodeID << " (" << finalID << ")" << std::endl;

        setSummingFunctionForNode(node.get());
        objects.push_back(std::move(node));

        return rawNode;
    }

    // Generate a unique node ID (integer)
    uint32_t generateID() const
    {
        std::set<unsigned int> usedIDs;
        for (const auto& obj : objects)
        {
            usedIDs.insert(obj->nodeID);
        }

        // Find the lowest unused ID starting from 0
        unsigned int idCounter = 0;
        while (usedIDs.find(idCounter) != usedIDs.end())
        {
            idCounter++; // Increment until an unused ID is found
        }

        return idCounter;
    }

    AudioNode* createObject(json node, bool addToGraph = true)
    {
        if (node.is_null())
            return nullptr;

        auto const object = ppl::string(node["obj"].get<std::string>()).toLower();

        // Handle both string-based IDs (e.g., "osc_1") and numeric IDs (e.g., 5)
        std::string idString;
        if (node.contains("id"))
        {
            if (node["id"].is_string())
            {
                idString = node["id"].get<std::string>(); // e.g., "osc_1"
            }
            else if (node["id"].is_number())
            {
                idString = std::to_string(node["id"].get<int>()); // Convert 5 → "5"
            }
        }
        else
        {
            idString = std::to_string(generateID()); // Assign new ID if missing
        }

        // Ensure the ID is unique
        if (objectIDMap.contains(idString))
        {
            std::string base;
            int number = 0;

            // Try to extract base and number (e.g., "osc_1" → base: "osc", number: 1)
            std::regex pattern(R"((.*?)(?:_(\d+))?$)");
            std::smatch match;

            if (std::regex_match(idString, match, pattern))
            {
                base = match[1];
                if (match[2].matched)
                {
                    number = std::stoi(match[2]);
                }
            }
            else
            {
                base = idString;
            }

            // Increment until we find an unused ID
            std::string newId;
            do
            {
                ++number;
                newId = base + "_" + std::to_string(number);
            }
            while (objectIDMap.contains(newId));

#ifdef LOG_GRAPH_INFO
            std::cerr << "Duplicate node ID detected: " << idString << ". Renaming to: " << newId << std::endl;
#endif

            idString = newId;
        }

        switch (hash(object))
        {
        case hash("add"):
            return addNode<Add>(idString, node);

        case hash("mul"):
            return addNode<Multiply>(idString, node);

        case hash("div"):
            return addNode<Divide>(idString, node);

        case hash("count"):
            return addNode<Count>(idString, node);

        case hash("dial"):
            return addNode<Dial>(idString, node);

        case hash("print"):
            return addNode<Print>(idString, node);

        case hash("if"):
            return addNode<If>(idString, node);

        case hash("ifelse"):
            return addNode<IfElse>(idString, node);

        case hash("sel"):
        case hash("select"):
            return addNode<Select>(idString, node);

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

        case hash("ain"):
        case hash("audioin"):
            return addNode<AudioIn>(idString, node);

        case hash("aout"):
        case hash("audioout"):
            return addNode<AudioOut>(idString, node);

        case hash("floatbox"):
            return addNode<FloatBox>(idString, node);

        case hash("radiobox"):
            return addNode<RadioBox>(idString, node);

        case hash("ping"):
            return addNode<Ping>(idString, node);

        case hash("scope"):
            return addNode<Scope>(idString, node);

        case hash("spec"):
            return addNode<Spec>(idString, node);

        case hash("bpf"):
        case hash("bandpassfilter"):
            return addNode<BandPassFilter>(idString, node);

        case hash("mtof"):
        case hash("midi2freq"):
            return addNode<MidiToFreq>(idString, node);

        case hash("chg"):
        case hash("change"):
        case hash("changed"):
            return addNode<Changed>(idString, node);

        case hash("loadevent"):
            return addNode<LoadEvent>(idString, node);

        case hash("reverb_fdn"):
        case hash("fdn"):
            return addNode<ReverbFDN>(idString, node);

        case hash("midinotein"):
        case hash("notein"):
            return addNode<MidiNoteIn>(idString, node);

        case hash("activemidinotes"):
            return addNode<ActiveMidiNotes>(idString, node);

        case hash("filtertag"):
            return addNode<FilterTag>(idString, node);

        case hash("get"):
            return addNode<Get>(idString, node);

        case hash("rnd"):
        case hash("random"):
            return addNode<Random>(idString, node);

        case hash("lb"):
        case hash("listbox"):
            return addNode<ListBox>(idString, node);

        case hash("pack"):
            return addNode<Pack>(idString, node);

        case hash("tag"):
            return addNode<TagEvent>(idString, node);

        case hash("strip"):
            return addNode<Strip>(idString, node);

        case hash("comment"):
            return addNode<Comment>(idString, node);

        case hash("intify"):
            return addNode<Intify>(idString, node);

        case hash("evdelay"):
            return addNode<EventDelay>(idString, node);

        case hash("drive"):
            return addNode<Drive>(idString, node);

        case hash("freqbins"):
            return addNode<FreqBins>(idString, node);

        case hash("freqresynth"):
            return addNode<FreqResynth>(idString, node);

        case hash("bincombine"):
            return addNode<BinCombine>(idString, node);

        default:
            // Unknown object name, return error
            std::cout << "Unknown object: " << object << std::endl;
            return nullptr;
        }
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
    GraphManager(int sampleRate, unsigned long frameCount)
        : ctx(std::make_unique<NodeContext>(sampleRate, frameCount))
    {
        //Logger::getInstance().startProcessingThread();
    }

    ~GraphManager()
    {
        //Logger::getInstance().stopProcessingThread();
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
        std::cout << "adding object from UI" << std::endl;

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
            return {};

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
            return {};

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

    std::vector<Edge*> connectMultiple(const std::vector<std::tuple<int, int, int, int>>& conns)
    {
        std::cout << "connecting from UI: " << std::endl;
        if (!activeGraph)
        {
            std::cerr << "No active graph available to connect objects." << std::endl;
            return {};
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

    bool disconnect(const std::string& oObj, int oPort, const std::string& iObj, int iPort)
    {
        if (!activeGraph)
        {
            std::cerr << "No active graph available to disconnect connection." << std::endl;
            return false;
        }

        transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());
        // Add the connection to the transitioning graph
        if (!transitioningGraph->disconnect(oObj, oPort, iObj, iPort))
        {
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
        if (!activeGraph)
        {
            std::cout << "No graph loaded" << std::endl;
            return;
        }

        activeGraph->printAdjacencyList();
    }

    void printGraph()
    {
        if (!activeGraph)
        {
            std::cout << "No graph loaded" << std::endl;
            return;
        }

        activeGraph->printGraph();
    }

    std::tuple<std::vector<Object*>, std::vector<Edge*>> setActiveGraph(const std::string& patchPath, const json& patch,
                                                                        const bool logVerbose)
    {
        patchLoadSuccess = false;

        if (swapGraph.load(std::memory_order_acquire))
        {
            std::cout << "Warning: Attempted to overwrite a transitioning graph before it was swapped." << std::endl;
            return {};
        }

        filePath = patchPath;

        transitioningGraph = std::make_shared<GraphHolder>(ctx.get());

        if (!transitioningGraph->loadPatch(patch, logVerbose))
        {
            transitioningGraph.reset();
            std::cerr << "Corrupt patch, failed to load." << std::endl;
            return {};
        }

        patchLoadSuccess = true;

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

        return {loadedObjects, connections};
    }

    bool wasPatchLoadSuccessful()
    {
        return patchLoadSuccess;
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

    const json copySelected(const std::vector<uint32_t>& selectedNodeIDs)
    {
        if (activeGraph)
        {
            return activeGraph->serializeSelectedNodes(selectedNodeIDs);
        }
    }

std::tuple<std::vector<Object*>, std::vector<Object*>, std::vector<Edge*>> pasteGraph(const json& patch)
{
    std::vector<Object*> pastedObjects;

    // Ensure we have an active graph.
    if (!activeGraph)
    {
        activeGraph = std::make_unique<GraphHolder>(ctx.get());
    }

    // Create a transitioning graph from the active graph.
    transitioningGraph = std::make_shared<GraphHolder>(activeGraph.get());

    // Mapping from the pasted node ID (string) to the new node's integer ID.
    std::unordered_map<std::string, int> idMapping;

    // Create new nodes from the "nodes" array.
    if (patch.contains("nodes") && patch["nodes"].is_array())
    {
        for (const auto& nodeJson : patch["nodes"])
        {
            std::string oldId;
            // Look for "id" first, then "nodeID" if needed.
            if (nodeJson.contains("id"))
            {
                oldId = nodeJson["id"].get<std::string>();
            }
            else if (nodeJson.contains("nodeID"))
            {
                if (nodeJson["nodeID"].is_string())
                    oldId = nodeJson["nodeID"].get<std::string>();
                else if (nodeJson["nodeID"].is_number())
                    oldId = std::to_string(nodeJson["nodeID"].get<int>());
            }

            // Create a new node via the transitioning graph.
            AudioNode* newAudioNode = transitioningGraph->createObject(nodeJson);
            if (newAudioNode)
            {
                // Get or create the UI for this node.
                Object* newObj = reinterpret_cast<Object*>(newAudioNode->getOrCreateUI());
                pastedObjects.push_back(newObj);

                // Save the mapping: the pasted id maps to the new node's id.
                if (!oldId.empty())
                {
                    idMapping[oldId] = newObj->nodeID;
                }
            }
        }
    }

    // Re-establish connections among pasted nodes using the new node IDs.
    if (patch.contains("connections") && patch["connections"].is_array())
    {
        for (const auto& edgeJson : patch["connections"])
        {
            std::string oldSourceId, oldTargetId;
            // Extract source node identifier.
            if (edgeJson.contains("sourceNode"))
            {
                if (edgeJson["sourceNode"].is_string())
                    oldSourceId = edgeJson["sourceNode"].get<std::string>();
                else if (edgeJson["sourceNode"].is_number())
                    oldSourceId = std::to_string(edgeJson["sourceNode"].get<int>());
            }
            // Extract target node identifier.
            if (edgeJson.contains("targetNode"))
            {
                if (edgeJson["targetNode"].is_string())
                    oldTargetId = edgeJson["targetNode"].get<std::string>();
                else if (edgeJson["targetNode"].is_number())
                    oldTargetId = std::to_string(edgeJson["targetNode"].get<int>());
            }

            int oPort = edgeJson.value("sourcePort", 0);
            int iPort = edgeJson.value("targetPort", 0);

            // Only reconnect if both endpoints were pasted.
            if (!oldSourceId.empty() && !oldTargetId.empty() &&
                idMapping.find(oldSourceId) != idMapping.end() &&
                idMapping.find(oldTargetId) != idMapping.end())
            {
                int newOId = idMapping[oldSourceId];
                int newIId = idMapping[oldTargetId];

                transitioningGraph->connect(newOId, oPort, newIId, iPort);
            }
            else
            {
                std::cerr << "Warning: Could not map edge from \"" << oldSourceId
                          << "\" to \"" << oldTargetId << "\"" << std::endl;
            }
        }
    }

    // Finalize graph updates.
    transitioningGraph->updateConnections();
    transitioningGraph->sortNodes();
    transitioningGraph->updateOutputInputPortMap();

    auto loadedObjects = getObjects();
    auto connections = transitioningGraph->getConnections();

    // Mark the transitioning graph ready to replace the active graph.
    swapGraph.store(true, std::memory_order_release);

    return { pastedObjects, loadedObjects, connections };
}


    void process(const float* inBuffer, float* outBuffer, unsigned long frameCount, std::vector<MidiMessage>& message)
    {
        dspTimer.start();

#ifdef DEBUG_MIDI
    if (message.message.empty()) {
        std::cout << "Empty MIDI message received" << std::endl;
        return;
    }

    // The first byte is the status byte
    unsigned char status = message.message[0];
    // High nibble gives the message type, low nibble gives the channel (0-indexed)
    unsigned char messageType = status & 0xF0;
    unsigned char channel = (status & 0x0F) + 1;  // Channels 1-16 for human readability

    std::cout << "MIDI message at timestamp " << message.timestamp << ": ";

    switch (messageType)
    {
        case 0x80: // Note Off
        {
            if (message.message.size() >= 3) {
                unsigned char note = message.message[1];
                unsigned char velocity = message.message[2];
                std::cout << "Note Off on channel " << static_cast<int>(channel)
                          << ", note " << static_cast<int>(note)
                          << ", velocity " << static_cast<int>(velocity);
            } else {
                std::cout << "Invalid Note Off message";
            }
            break;
        }
        case 0x90: // Note On
        {
            if (message.message.size() >= 3) {
                unsigned char note = message.message[1];
                unsigned char velocity = message.message[2];
                if (velocity == 0)
                    std::cout << "Note Off (via Note On with zero velocity) on channel " << static_cast<int>(channel)
                              << ", note " << static_cast<int>(note);
                else
                    std::cout << "Note On on channel " << static_cast<int>(channel)
                              << ", note " << static_cast<int>(note)
                              << ", velocity " << static_cast<int>(velocity);
            } else {
                std::cout << "Invalid Note On message";
            }
            break;
        }
        case 0xA0: // Polyphonic Key Pressure (Aftertouch)
        {
            if (message.message.size() >= 3) {
                unsigned char note = message.message[1];
                unsigned char pressure = message.message[2];
                std::cout << "Polyphonic Key Pressure on channel " << static_cast<int>(channel)
                          << ", note " << static_cast<int>(note)
                          << ", pressure " << static_cast<int>(pressure);
            } else {
                std::cout << "Invalid Polyphonic Key Pressure message";
            }
            break;
        }
        case 0xB0: // Control Change
        {
            if (message.message.size() >= 3) {
                unsigned char controller = message.message[1];
                unsigned char value = message.message[2];
                std::cout << "Control Change on channel " << static_cast<int>(channel)
                          << ", controller " << static_cast<int>(controller)
                          << ", value " << static_cast<int>(value);
            } else {
                std::cout << "Invalid Control Change message";
            }
            break;
        }
        case 0xC0: // Program Change
        {
            if (message.message.size() >= 2) {
                unsigned char program = message.message[1];
                std::cout << "Program Change on channel " << static_cast<int>(channel)
                          << ", program " << static_cast<int>(program);
            } else {
                std::cout << "Invalid Program Change message";
            }
            break;
        }
        case 0xD0: // Channel Pressure (Aftertouch)
        {
            if (message.message.size() >= 2) {
                unsigned char pressure = message.message[1];
                std::cout << "Channel Pressure on channel " << static_cast<int>(channel)
                          << ", pressure " << static_cast<int>(pressure);
            } else {
                std::cout << "Invalid Channel Pressure message";
            }
            break;
        }
        case 0xE0: // Pitch Bend
        {
            if (message.message.size() >= 3) {
                // Pitch Bend uses two data bytes (LSB and MSB) to form a 14-bit value
                unsigned char lsb = message.message[1];
                unsigned char msb = message.message[2];
                int pitchValue = (static_cast<int>(msb) << 7) | static_cast<int>(lsb);
                std::cout << "Pitch Bend on channel " << static_cast<int>(channel)
                          << ", value " << pitchValue;
            } else {
                std::cout << "Invalid Pitch Bend message";
            }
            break;
        }
        default:
        {
            std::cout << "Unknown MIDI message (status: 0x" << std::hex << static_cast<int>(status)
                      << std::dec << ")";
            break;
        }
    }

    std::cout << std::endl;
#endif
        if (swapGraph.load(std::memory_order_acquire))
        {
            // Perform the swap on the audio thread
            activeGraph.swap(transitioningGraph);
            swapGraph.store(false, std::memory_order_release);
        }

        // Process the current graph
        if (activeGraph)
        {
            activeGraph->process(inBuffer, outBuffer, frameCount, message);
        }

        dspTimer.end(frameCount, ctx->sampleRate);

        processPeak(outBuffer, frameCount);

    }

    float getDspTiming()
    {
        return dspTimer.getCpuUsage();
    }

    // Queue size would be largest 8 if 64 buffrer size at 44100 hz and a video refresh rate of 120 hz
    moodycamel::ConcurrentQueue<float> volumeMeterQueue = moodycamel::ConcurrentQueue<float>(100);

private:
    DspTimer dspTimer;

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

    std::unique_ptr<NodeContext> ctx;

    std::shared_ptr<GraphHolder> activeGraph; // Actively processed graph
    std::shared_ptr<GraphHolder> transitioningGraph; // New graph prepared for swapping
    std::atomic<bool> swapGraph = false; // Signal for readiness to swap


    bool patchLoadSuccess = false;
};
