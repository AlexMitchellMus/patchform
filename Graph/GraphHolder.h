/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <utility>
#include <vector>
#include <iostream>
#include <xutility>
#include <set>
#include <regex>

#include <filesystem>

#include "json.hpp"
using json = nlohmann::json;

#include "unordered_dense.h"
#include "../Nodes/AllNodes.h"
#include "../Utility/ppl_string.hpp"
#include "AdjacencyMap.h"
#include "Edge.h"
#include "../Nodes/NodeRegistry.h"
#include <simde/x86/avx2.h>

#undef max

#include "Graph.h"

class GraphHolder
{

public:
    GraphHolder(std::shared_ptr<NodeContext> ctx)
        : context(std::move(ctx))
    {
        graph = std::make_unique<Graph>(context);
    };

    GraphHolder(const GraphHolder& other)
    : context(other.context)
    , objects(other.objects)
    , objectIDMap(other.objectIDMap)
    , connections(other.connections)
    , graph(std::make_unique<Graph>(other.context))
    {}

    Graph* getGraph() const
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

    // Call cleanup on all objects that have been removed from the graph when swapping with new graph
    void processCleanup() const
    {
        for (auto* node : graph->objectsToCleanup)
        {
            node->cleanupAudio();
        }
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

            // connections use unique ID's for nodes
            // FIXME: Is this really correct? we use the overloaded connect to connect with the stringID
            connect(objects[source]->nodeID, connection["sourcePort"], objects[target]->nodeID, connection["targetPort"]);
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
                            if (auto connectedPort = graph->objectsSorted[connectedSortedIndex]->getOutputPort(connectedNode.second))
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
                    if (conn->getoNode() == nodeID && conn->getoPort() == static_cast<int>(outPort))
                    {
                        int targetID = conn->getiNode();
                        if (objectIDtoSortedIndex.contains(targetID))
                        {
                            size_t targetSortedIndex = objectIDtoSortedIndex[targetID];
                            AudioNode* targetNode = graph->objectsSorted[targetSortedIndex];
                            int targetPort = conn->getiPort();

                            auto* inputPort = targetNode->getInputPort(targetPort);
                            if (!inputPort) continue;

                            if (auto* outputPort = node->getOutputPort(outPort))
                            {
                                float* dst = inputPort->getAudioBuffer();

                                group.downstreamConnections.push_back({
                                    .src = outputPort->getAudioBuffer(),
                                    .dst = inputPort->getAudioBuffer(),
                                    .bufferSize = outputPort->getAudioBufferSize(),
                                    .node = targetNode,
                                    .inputPort = inputPort,
                                    .inputPortIndex = targetPort,
                                    .targetIndex = static_cast<uint32_t>(targetSortedIndex),
                                });

                                inputPort->isAnyConnectedPortSignal = outputPort->isSignal();
                            }
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
        std::cout << "removing an object: " << nodeID << std::endl;
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
        node->setNodeDirty = [node]()
        {
            node->context->messageQueue.enqueue([node](Graph& runningGraph)
            {
                auto& vec = runningGraph.objectsSorted;
                const auto it = std::ranges::find(vec, node);
                if (it == vec.end())
                    return; // not found

                const int index = static_cast<int>(std::distance(vec.begin(), it));
                runningGraph.activeEventNodes[index / 64] |= (1ULL << (index % 64));
            });
        };

        node->pushOutputEvents = [](const std::vector<std::unique_ptr<AudioPort>>& outputPorts, Graph& graph, const int index)
        {
            const auto& groups = graph.downstreamPortMap[index];

            for (const auto& group : groups)
            {
                if (group.outputPortNumber >= outputPorts.size())
                    continue;

                const auto& events = outputPorts[group.outputPortNumber]->getEvents();
                if (events.empty()) continue;

                for (const auto& conn : group.downstreamConnections)
                {
                    for (Event* event : events)
                        conn.node->pushEvent(conn.inputPortIndex, event);

                    graph.activeEventNodes[conn.targetIndex >> 6] |= (1ULL << (conn.targetIndex & 63));
                }
            }
        };

        node->pushOutputAudio = [](Graph& graph, const int index)
        {
            auto& groups = graph.downstreamPortMap[index];

            for (auto& group : groups)
            {
                for (auto& conn : group.downstreamConnections)
                {
                    float* __restrict conDest = conn.dst;
                    const float* __restrict conSrc = conn.src;
                    size_t n = conn.bufferSize;

                    size_t i = 0;
                    for (; i + 8 <= n; i += 8)
                    {
                        simde__m256 dstVec = simde_mm256_load_ps(conDest + i);   // aligned
                        simde__m256 srcVec = simde_mm256_load_ps(conSrc + i);   // aligned
                        dstVec = simde_mm256_add_ps(dstVec, srcVec);
                        simde_mm256_store_ps(conDest + i, dstVec);              // aligned
                    }

                    for (; i < n; ++i)
                        conDest[i] += conSrc[i];
                }
            }
        };
    }

    AudioNode* addNode(const std::string& finalID, AudioNode* node)
    {
        if (!node) return nullptr;

        if (node->nodeCreationData.contains("pos") && node->nodeCreationData["pos"].is_array() &&
            node->nodeCreationData["pos"].size() >= 2)
        {
            node->canvasPos = pptk::Point(
                node->nodeCreationData["pos"][0].get<float>(),
                node->nodeCreationData["pos"][1].get<float>()
            );
        }

        uint32_t nodeID = generateID();
        node->nodeID = nodeID;
        node->nodeIDString = finalID;
        objectIDMap[finalID] = nodeID;

        setSummingFunctionForNode(node);
        objects.push_back(std::unique_ptr<AudioNode>(node));  // take ownership

        return node;
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

        //  Use the static NodeRegistry for reflection-based lookup of node names and aliases
        auto* nodePtr = NodeRegistry::getInstance().createNode(object.str(), context, node);
        if (!nodePtr) {
            std::cerr << "Unknown node type: " << object << "\n";
            return nullptr;
        }
        return addNode(idString, nodePtr);
    };

    void printGraph()
    {
        for (const auto& obj : objects)
        {
            std::cout << obj->nodeID << " [" << obj->getShortName() << "]" << std::endl;
        }
    }

    std::unique_ptr<Graph> graph;

private:
    std::vector<std::shared_ptr<Edge>> connections;
    std::vector<std::shared_ptr<AudioNode>> objects;

    ankerl::unordered_dense::map<std::string, uint32_t> objectIDMap;

    std::shared_ptr<NodeContext> context;

    std::vector<std::shared_ptr<AudioNode>> removedObjects;
};