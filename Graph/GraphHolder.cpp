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

void GraphHolder::process(const float* inBuffer, float* buffer, unsigned long frameCount,
                          std::vector<MidiMessage>& midiMessage)
{
    //#define DSP_FREE_ATOMS
#ifdef DSP_FREE_ATOMS
    std::cout << "--- free atoms: " << context->eventPool.getFreeListSize() << std::endl;
#endif
    std::function<void(Graph&)> msg;
    while (parentGraph->messageQueue.try_dequeue(msg))
        msg(*graph);

    graph->process(inBuffer, buffer, frameCount, midiMessage);
}


void GraphHolder::setSummingFunctionForNode(AudioNode* node)
{
    // This will enqueue a message into the graph (or graph->supatch etc)
    // The graph then will run that message, see the above ^ process function
    node->setNodeDirty = [mgr = node->getGraphManagerParent(), nodeID = node->nodeID, nodePtr = node]()
    {
        mgr->messageQueue.enqueue(
            [nodeID, nodePtr](Graph& runningGraph)
            {
                auto it = std::ranges::lower_bound(
                    runningGraph.nodeIDToSortedIndex, nodeID,
                    {}, [](const auto& pair) { return pair.first; });

                if (it == runningGraph.nodeIDToSortedIndex.end() || it->first != nodeID)
                {
                    std::cerr << "error - nodeID not found in nodeIDToSortedIndex" << std::endl;
                    return;
                }

                int index = it->second;

                assert(index >= 0 && index < static_cast<int>(runningGraph.objectsSorted.size()));
                assert(runningGraph.objectsSorted[index] != nullptr);
                assert(runningGraph.objectsSorted[index]->nodeID == nodeID);
                assert(runningGraph.objectsSorted[index] == nodePtr); // <- pointer match

                // activeEventNodes are index from the sorted graph
                runningGraph.activeEventNodes[index >> 6] |= (1ULL << (index & 63));
            });
    };

    node->pushOutputEventsFromPointers = [](const std::vector<AudioPort*>& outputPorts, Graph& graph, int index)
    {
        const auto& downstream = graph.downstreamPortMap[index];
        for (const auto& group : downstream)
        {
            uint8_t portIndex = group.outputPortNumber;
            if (portIndex >= outputPorts.size())
                continue;

            auto* outerPort = outputPorts[portIndex];
            if (!outerPort) continue;

            if (outerPort->isSignal())
            {
                const float* src = outerPort->getAudioBuffer();
                for (const auto& conn : group.downstreamConnections)
                {
                    float* dst = conn.dst;
                    size_t n = conn.bufferSize;
                    for (size_t s = 0; s < n; ++s)
                        dst[s] += src[s];
                }
            }
            else
            {
                for (const auto& conn : group.downstreamConnections)
                {
                    for (const auto& ev : outerPort->getEvents())
                    {
                        conn.inputPort->addEvent(ev);
                    }
                    graph.activeEventNodes[conn.targetIndex >> 6] |= (1ULL << (conn.targetIndex & 63));
                }
            }
        }
    };

    node->pushOutputEvents = [](const std::vector<std::unique_ptr<AudioPort>>& outputPorts, Graph& graph,
                                const int index, AudioNode* _this)
    {
        assert(graph.objectsSorted[index] == _this);

        const auto& groups = graph.downstreamPortMap[index];

        for (const auto& group : groups)
        {
            if (group.outputPortNumber >= outputPorts.size())
                continue;

            assert(group.outputPortNumber < outputPorts.size());
            assert(outputPorts[group.outputPortNumber] != nullptr && outputPorts[group.outputPortNumber]->getParentNode() == _this);
            assert(!port->isInput);
            assert(_this->outputPortVisibility().test(group.outputPortNumber));

            const auto& events = outputPorts[group.outputPortNumber]->getEvents();
            if (events.empty()) continue;

            for (const auto& conn : group.downstreamConnections)
            {
                assert(conn.inputPort != nullptr);
                assert(conn.inputPort->isInput);
                assert(graph.objectsSorted[conn.targetIndex] == conn.node);
                assert(graph.objectsSorted[conn.targetIndex]->nodeID == conn.node->nodeID);

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
                    simde__m256 dstVec = simde_mm256_load_ps(conDest + i); // aligned
                    simde__m256 srcVec = simde_mm256_load_ps(conSrc + i); // aligned
                    dstVec = simde_mm256_add_ps(dstVec, srcVec);
                    simde_mm256_store_ps(conDest + i, dstVec); // aligned
                }

                for (; i < n; ++i)
                    conDest[i] += conSrc[i];
            }
        }
    };
}
