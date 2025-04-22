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

#include "../Nodes/AudioNodeBase.h"
#include "unordered_dense.h"
#include "NodeContext.h"
#include "AdjacencyMap.h"

#undef max

// Graph to manage nodes and process them in the correct order
class Graph
{
public:
    explicit Graph(const std::shared_ptr<NodeContext>& ctx)
        : context(ctx)
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
                    // Allow feedback node to create cycles
                    if (!objectsListCopy[downstreamNodeIndex]->canFeedback())
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
        std::function<void(Graph&)> msg;
        while (context->messageQueue.try_dequeue(msg))
            msg(*this);

        const size_t total = objectsSorted.size();
        unsigned i = 0;

        while (i < total)
        {
            const size_t word = i >> 6;
            const uint64_t combined = (activeEventNodes[word] | activeAudioNodes[word]) >> (i & 63);

            if (!combined)
            {
                i = (word + 1) << 6;
                continue;
            }

            const unsigned offset = std::countr_zero(combined);
            i += offset;

            if (i >= total)
                break;

            objectsSorted[i]->process(inBuffer, buffer, midiMessage, frameCount, *this, i);

            activeEventNodes[i >> 6] &= ~(1ULL << (i & 63));
            ++i;
        }

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
    std::shared_ptr<NodeContext> context;

    std::vector<AudioNode*> objectsToCleanup;
};