//
// Created by alexw on 2/05/2025.
//
#include "GraphHolder.h"
#include "GraphManager.h"

uint32_t GraphHolder::generateGlobalID() const
{
    uint32_t id = 0;

    auto root = parentGraph;
    while (root->parentGraph) root = root->parentGraph;

    while (root->usedGlobalIDs.contains(id)) ++id;

    root->usedGlobalIDs.insert(id);

    std::cout << "parent graph: " << root << " new ID is: " << id << std::endl;

    return id;
}

void GraphHolder::updateOutputInputPortMap()
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
                        if (auto connectedPort = graph->objectsSorted[connectedSortedIndex]->getOutputPort(
                            connectedNode.second))
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
    // Create a set of valid node pointers for fast verification
    ankerl::unordered_dense::set<AudioNode*> validSortedNodes(graph->objectsSorted.begin(), graph->objectsSorted.end());

    // Clear and resize
    graph->downstreamPortMap.clear();
    graph->downstreamPortMap.resize(graph->objectsSorted.size());

    for (size_t sortedIndex = 0; sortedIndex < graph->objectsSorted.size(); ++sortedIndex)
    {
        AudioNode* node = graph->objectsSorted[sortedIndex];
        int nodeID = node->nodeID;

        for (size_t outPort = 0; outPort < node->outputPortBuffers.size(); ++outPort)
        {
            DownstreamPortGroup group;
            group.outputPortNumber = static_cast<uint8_t>(outPort);

            for (const auto& conn : connections)
            {
                // NEW: Skip stale connections whose nodes don't exist in this graph
                if (!graph->objectIDtoIndex.contains(conn->getoNode()) ||
                    !graph->objectIDtoIndex.contains(conn->getiNode()))
                    continue;

                if (conn->getoNode() != nodeID || conn->getoPort() != static_cast<int>(outPort))
                    continue;

                int targetID = conn->getiNode();
                auto it = objectIDtoSortedIndex.find(targetID);
                if (it == objectIDtoSortedIndex.end())
                    continue;

                size_t targetSortedIndex = it->second;
                AudioNode* targetNode = graph->objectsSorted[targetSortedIndex];

                // Validate targetNode is still in sorted list
                if (!validSortedNodes.contains(targetNode))
                    continue;

                auto* inputPort = targetNode->getInputPort(conn->getiPort());
                auto* outputPort = node->getOutputPort(outPort);
                if (!inputPort || !outputPort)
                    continue;

                group.downstreamConnections.push_back({
                    .src = outputPort->getAudioBuffer(),
                    .dst = inputPort->getAudioBuffer(),
                    .bufferSize = outputPort->getAudioBufferSize(),
                    .node = targetNode,
                    .inputPort = inputPort,
                    .inputPortIndex = static_cast<uint8_t>(conn->getiPort()),
                    .targetIndex = static_cast<uint32_t>(targetSortedIndex),
                });

                inputPort->isAnyConnectedPortSignal = outputPort->isSignal();
            }
            // Only add the group if it contains at least one connection.
            if (!group.downstreamConnections.empty())
                graph->downstreamPortMap[sortedIndex].push_back(std::move(group));
        }
    }
#define INTEGRITY_CHECK
#ifdef INTEGRITY_CHECK
    for (size_t i = 0; i < graph->downstreamPortMap.size(); ++i)
    {
        AudioNode* node = graph->objectsSorted[i];
        for (auto& group : graph->downstreamPortMap[i])
        {
            for (auto& conn : group.downstreamConnections)
            {
                if (!conn.node)
                {
                    std::cerr << "[BUG] Null target node at index " << i << "\n";
                    continue;
                }

                // Check if node exists in sorted list
                auto it = std::find(graph->objectsSorted.begin(), graph->objectsSorted.end(), conn.node);
                if (it == graph->objectsSorted.end())
                {
                    std::cerr << "[BUG] targetNode not in objectsSorted! ID: " << conn.node->nodeID << "\n";
                }

                // Check port ownership
                if (conn.inputPort && conn.inputPort != conn.node->getInputPort(conn.inputPortIndex))
                {
                    std::cerr << "[BUG] inputPort mismatch on node ID: " << conn.node->nodeID
                        << " expected: " << static_cast<void*>(conn.node->getInputPort(conn.inputPortIndex))
                        << " actual: " << static_cast<void*>(conn.inputPort) << "\n";
                }

                if (conn.src && conn.src != node->getOutputPort(group.outputPortNumber)->getAudioBuffer())
                {
                    std::cerr << "[BUG] src buffer mismatch on node ID: " << node->nodeID << "\n";
                }
            }
        }
    }
#endif

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
                        std::cout << " " << port; // Replace with `port->toString()` if such a method exists
                    }
                    std::cout << std::endl;
                }
            }
        }
#endif
}
