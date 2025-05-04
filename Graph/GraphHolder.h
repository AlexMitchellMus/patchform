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

class GraphHolder {
public:
    explicit GraphHolder(GraphManager* parent);

    GraphHolder(const GraphHolder& other);

    [[nodiscard]] Graph* getGraph() const
    {
        return graph.get();
    }

    [[nodiscard]] std::vector<AudioNode*> getObjects() const
    {
        std::vector<AudioNode*> nodes;
        for (auto& node : objects)
        {
            nodes.push_back(node.get());
        }
        return nodes;
    }

    [[nodiscard]] json graphToJSON() const
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

        // Allow empty patch nodes
        if (!patch.contains("nodes"))
            return false;

        for (const auto& node : patch["nodes"])
        {
            if (const auto* nodePtr = createObject(node))
                objectIDMap[node["id"].get<std::string>()] = nodePtr->nodeID;
        }

        // Create connections

        // Allow empty patch connections
        if (!patch.contains("connections"))
            return false;

        for (const auto& connection : patch["connections"])
        {
            std::string srcKey = connection["sourceNode"];
            std::string dstKey = connection["targetNode"];

            if (!objectIDMap.contains(srcKey) || !objectIDMap.contains(dstKey)) {
                std::cerr << "Missing ID: " << srcKey << " -> " << dstKey << "\n";
                continue;
            }

            const uint32_t srcID = objectIDMap[srcKey];
            const uint32_t dstID = objectIDMap[dstKey];

            connectObjIndex(srcID, connection["sourcePort"], dstID, connection["targetPort"]);
        }
        return true;
    }

    void updateOutputInputPortMap();

    void printConnectionTable() const
    {
        std::cerr << "=== Connection Table ===\n";
        for (const auto& conn : connections)
        {
            std::cerr << "Conn: " << conn->getoNode()
                      << ":" << conn->getoPort()
                      << " -> " << conn->getiNode()
                      << ":" << conn->getiPort() << "\n";
        }

        std::cerr << "\n=== Adjacency Map (forward) ===\n";
        for (const auto& [key, targets] : graph->adjacencyMap.getForward())
        {
            auto [fromIndex, fromPort] = AdjacencyMap::unpackKey(key);
            std::cerr << "From " << objects[fromIndex]->nodeID << ":" << fromPort << " -> ";
            for (auto targetKey : targets)
            {
                auto [toIndex, toPort] = AdjacencyMap::unpackKey(targetKey);
                std::cerr << objects[toIndex]->nodeID << ":" << toPort << " ";
            }
            std::cerr << "\n";
        }
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

    void updateConnections() const
    {
        graph->objectIDtoIndex.clear();
        for (size_t i = 0; i < objects.size(); ++i)
            graph->objectIDtoIndex[objects[i]->nodeID] = static_cast<uint32_t>(i);

        graph->adjacencyMap.clear();

        for (const auto& conn : connections)
        {
            auto oIt = graph->objectIDtoIndex.find(conn->getoNode());
            auto iIt = graph->objectIDtoIndex.find(conn->getiNode());

            if (oIt != graph->objectIDtoIndex.end() && iIt != graph->objectIDtoIndex.end())
            {
                graph->addAdjacency(oIt->second, conn->getoPort(), iIt->second, conn->getiPort());
            }
            else
            {
                std::cerr << "Skipping invalid connection: "
                          << conn->getoNode() << ":" << conn->getoPort()
                          << " -> " << conn->getiNode() << ":" << conn->getiPort() << "\n";
            }
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
        return connectObjIndex(oNode, oPort, iNode, iPort);
    }

    // Create connections with the object index
    bool connectObjIndex(uint32_t oNode, uint32_t oPort, uint32_t iNode, uint32_t iPort)
    {
        const uint64_t hash = Edge::encodeHash(oNode, oPort, iNode, iPort);
        for (const auto& conn : connections)
        {
            if (conn->getHash() == hash)
                return false;
        }

        connections.push_back(std::make_shared<Edge>(oNode, oPort, iNode, iPort));
        return true;
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
            if (connection->getHash() == connEdgeHash) {
                std::cout << "Removing connection: "
                          << connection->getoNode() << ":" << connection->getoPort()
                          << " -> " << connection->getiNode() << ":" << connection->getiPort()
                          << "\n";
                return true;
            }
            return false;
        });
    }

    // Remove connections with the object index
    void disconnect(const uint32_t oNode, const uint32_t oPort, const uint32_t iNode, const uint32_t iPort)
    {
        auto toRemove = Edge::encodeHash(oNode, oPort, iNode, iPort);
        removeEdge(toRemove);
    }

    void removeInvalidConnections()
    {
        std::erase_if(connections, [&](const std::shared_ptr<Edge>& conn)
        {
            const auto* outNode = getNodeByID(conn->getoNode());
            const auto* inNode  = getNodeByID(conn->getiNode());

            if (!outNode || conn->getoPort() >= outNode->getNumOutputs())
                return true;

            if (!inNode || conn->getiPort() >= inNode->getNumInputs())
                return true;

            // Visibility check
            if (!outNode->outputPortVisibility().test(conn->getoPort()))
                return true;

            if (!inNode->inputPortVisibility().test(conn->getiPort()))
                return true;

            return false;
        });
    }

    [[nodiscard]] AudioNode* getNodeByID(const uint32_t id) const
    {
        if (const auto it = graph->objectIDtoIndex.find(id); it != graph->objectIDtoIndex.end())
            return objects[it->second].get();

        return nullptr;
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
        const auto mapIt = std::find_if(objectIDMap.begin(), objectIDMap.end(),
                                  [nodeID](const auto& entry) { return entry.second == nodeID; }
        );
        if (mapIt != objectIDMap.end())
        {
            objectIDMap.erase(mapIt);
        }

        // 3) Remove any connections associated with this node.
        std::erase_if(connections, [nodeID](const auto& con)
        {
            if (con->getiNode() == nodeID || con->getoNode() == nodeID)
            {
                return true;
            }
            return false;
        });
    }


    void process(const float* inBuffer, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage);

    void sortNodes()
    {
        graph->objectIDtoIndex.clear();
        for (size_t i = 0; i < objects.size(); ++i)
            graph->objectIDtoIndex[objects[i]->nodeID] = static_cast<uint32_t>(i);

        graph->adjacencyMap.clear();
        for (const auto& c : connections)
        {
            graph->addAdjacency(c->getoNode(), c->getoPort(), c->getiNode(), c->getiPort());
        }

        if (graph->sortNodes(objects))  // returns true if sort succeeded
            return;

        std::cerr << "Cycle detected, attempting to remove invalid connections..." << std::endl;

        std::vector<std::shared_ptr<Edge>> validConnections;
        ankerl::unordered_dense::set<uint64_t> seenHashes;

        for (const auto& conn : connections)
        {
            uint64_t hash = conn->getHash();
            if (seenHashes.contains(hash))
                continue;

            seenHashes.insert(hash);
            validConnections.push_back(conn);

            graph->objectIDtoIndex.clear();
            for (size_t i = 0; i < objects.size(); ++i)
                graph->objectIDtoIndex[objects[i]->nodeID] = static_cast<uint32_t>(i);

            graph->adjacencyMap.clear();
            for (const auto& c : validConnections)
            {
                auto oIt = graph->objectIDtoIndex.find(c->getoNode());
                auto iIt = graph->objectIDtoIndex.find(c->getiNode());
                if (oIt != graph->objectIDtoIndex.end() && iIt != graph->objectIDtoIndex.end())
                    graph->addAdjacency(oIt->second, c->getoPort(), iIt->second, c->getiPort());
            }

            if (!graph->sortNodes(objects))
            {
                std::cerr << "Removed cycle-causing connection: "
                          << conn->getoNode() << ":" << conn->getoPort()
                          << " -> " << conn->getiNode() << ":" << conn->getiPort() << std::endl;

                validConnections.pop_back();
                seenHashes.erase(hash);
            }
        }

        connections = std::move(validConnections);
        updateConnections(); // rebuild full adjacency
        graph->sortNodes(objects); // final sort after full rebuild
    }


    void printAdjacencyList() const
    {
        graph->printAdjacencyList();
    }

    void setSummingFunctionForNode(AudioNode* node);

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

        const uint32_t nodeID = generateGlobalID();
        const auto stringID = finalID.empty() ? std::to_string(nodeID) : finalID;
        node->nodeID = nodeID;
        node->nodeIDString = stringID;

        objectIDMap[stringID] = nodeID;

        const auto index = static_cast<uint32_t>(objects.size());
        graph->objectIDtoIndex[nodeID] = index;

        setSummingFunctionForNode(node);
        objects.push_back(std::unique_ptr<AudioNode>(node));

        return node;
    }

    [[nodiscard]] uint32_t generateGlobalID() const;

    AudioNode* createObject(json node)
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
            idString = "";
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
        if (nodePtr)
        {
            nodePtr->setGraphManagerParent(parentGraph);
            nodePtr->postCreate();
        } else
        {
            std::cerr << "Unknown node type: " << object << "\n";
            return nullptr;
        }
        return addNode(idString, nodePtr);
    };

    void printGraph() const
    {
        for (const auto& obj : objects)
        {
            std::cout << obj->nodeID << " [" << obj->getShortName() << "]" << std::endl;
        }
    }

    std::unique_ptr<Graph> graph;

    GraphManager* parentGraph = nullptr;

    // Used for sub-patches
    std::vector<AudioPort*> subInputs;
    std::vector<AudioPort*> subOutputs;
    std::vector<AudioPort*> subInputOuterPorts;
    std::vector<AudioPort*> subOutputOuterPorts;
    std::vector<int> subInputSortedIndices;
    std::vector<AudioNode*> subInputNodes;

private:
    std::vector<std::shared_ptr<Edge>> connections;
    std::vector<std::shared_ptr<AudioNode>> objects;
    std::vector<std::shared_ptr<AudioNode>> removedObjects;

    ankerl::unordered_dense::map<std::string, uint32_t> objectIDMap;

    std::shared_ptr<NodeContext> context;


};