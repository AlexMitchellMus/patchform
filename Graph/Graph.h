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
    std::cout << "SORTING, adjacnecy map size: " << adjacencyMap.getForward().size() << std::endl;

    sortedNodes.clear();
    const size_t nodeCount = objectsListCopy.size();
    if (nodeCount == 0) return;
    sortedNodes.reserve(nodeCount);

    std::vector<int> inDegree(nodeCount, 0);
    std::vector<size_t> zeroInDegreeIndices;
    zeroInDegreeIndices.reserve(nodeCount);

    // Compute in-degrees
    for (const auto& [inputKey, sources] : adjacencyMap.getBackward())
    {
        auto [dstIndex, dstPort] = AdjacencyMap::unpackKey(inputKey);
        for (const auto& sourceKey : sources)
        {
            auto [srcIndex, srcPort] = AdjacencyMap::unpackKey(sourceKey);
            if (!objectsListCopy[srcIndex]->canFeedback())
            {
                ++inDegree[dstIndex];
            }
        }
    }

    for (size_t i = 0; i < nodeCount; ++i)
    {
        if (inDegree[i] == 0)
            zeroInDegreeIndices.push_back(i);
    }

    ankerl::unordered_dense::set<AudioNode*> insertedFeedbacks;
    size_t processIndex = 0;

    while (processIndex < zeroInDegreeIndices.size())
    {
        const size_t currentIndex = zeroInDegreeIndices[processIndex++];
        AudioNode* currentNode = objectsListCopy[currentIndex];
        sortedNodes.push_back(currentNode);

        // Insert upstream feedback nodes
        for (const auto& [inputKey, sources] : adjacencyMap.getBackward())
        {
            auto [inputIndex, inputPort] = AdjacencyMap::unpackKey(inputKey);
            if (inputIndex != currentIndex) continue;

            for (const auto& sourceKey : sources)
            {
                auto [srcIndex, srcPort] = AdjacencyMap::unpackKey(sourceKey);
                AudioNode* src = objectsListCopy[srcIndex];
                if (src->canFeedback() && !insertedFeedbacks.contains(src))
                {
                    sortedNodes.push_back(src);
                    insertedFeedbacks.insert(src);
                }
            }
        }

        // Decrement downstream in-degrees
        for (const auto& [sourceKey, downstreamKeys] : adjacencyMap.getForward())
        {
            auto [srcIndex, srcPort] = AdjacencyMap::unpackKey(sourceKey);
            if (srcIndex != currentIndex) continue;

            for (const auto& downstreamKey : downstreamKeys)
            {
                auto [dstIndex, dstPort] = AdjacencyMap::unpackKey(downstreamKey);
                if (--inDegree[dstIndex] == 0)
                {
                    zeroInDegreeIndices.push_back(dstIndex);

                }
            }
        }
    }

    if (sortedNodes.size() != nodeCount)
    {
        bool allFeedbackOK = true;
        for (const auto& [sourceKey, downstreamKeys] : adjacencyMap.getForward())
        {
            auto [srcIndex, srcPort] = AdjacencyMap::unpackKey(sourceKey);
            AudioNode* src = objectsListCopy[srcIndex];

            for (const auto& dstKey : downstreamKeys)
            {
                auto [dstIndex, dstPort] = AdjacencyMap::unpackKey(dstKey);
                AudioNode* dst = objectsListCopy[dstIndex];

                if (std::find(sortedNodes.begin(), sortedNodes.end(), dst) == sortedNodes.end())
                {
                    if (!src->canFeedback())
                    {
                        allFeedbackOK = false;
                        break;
                    }
                }
            }
            if (!allFeedbackOK) break;
        }

        if (!allFeedbackOK)
        {
            sortedNodes.clear();
            std::cout << "Unresolvable cycle — graph cleared\n";
        }
        else
        {
            std::cout << "All cycles involve feedback — graph valid\n";
        }
    }
    else
    {
        std::cout << "Complete topological sort successful\n";
    }
}


    bool sortNodes(const std::vector<std::shared_ptr<AudioNode>>& objectList)
    {
        //#define LOG_GRAPH_INFO
#ifdef LOG_GRAPH_INFO
    auto start = std::chrono::high_resolution_clock::now();
#endif

        // Convert shared_ptr list to raw pointers for processing
        objectsListCopy.reserve(objectList.size());
        objectsListCopy.clear();

        for (size_t i = 0; i < objectList.size(); ++i)
        {
            if (objectList[i])
                objectsListCopy.push_back(objectList[i].get());
        }

        // Perform topological sort on the nodes
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

#define DEBUG_SORT
#ifdef DEBUG_SORT
        std::cout << "======== presort =======" << std::endl;
        int pos = 0;
        for (auto& node : objectsListCopy)
        {
            std::cout << "node: " << node->getName() << " ID: " << node->nodeID << " index in objectsListCopy: " << pos << std::endl;
            pos++;
        }

        std::cout << "======== sorted =======" << std::endl;
        pos = 0;
        for (auto node : objectsSorted)
        {
            std::cout << "node graph: " << node->getName() << " ID: " << node->nodeID << " index in objectsSorted: " << pos << std::endl;
            pos++;
        }
#endif
        return !objectsSorted.empty();
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

    // Takes objectIDs, converts to vector indices, then stores adjacency using those indices
    bool addAdjacency(int oNodeID, int oPort, int iNodeID, int iPort)
    {
        auto oIt = objectIDtoIndex.find(oNodeID);
        auto iIt = objectIDtoIndex.find(iNodeID);
        if (oIt == objectIDtoIndex.end() || iIt == objectIDtoIndex.end())
        {
            //std::cerr << "FAILED addAdjacency: " << oNodeID << ":" << oPort << " -> " << iNodeID << ":" << iPort << "\n";
            return false;
        }

        uint32_t oIndex = oIt->second;
        uint32_t iIndex = iIt->second;

        //std::cerr << "ADJ INSERT: ID "
        //          << oNodeID << ":" << oPort << " (idx " << oIndex << ") -> "
        //          << iNodeID << ":" << iPort << " (idx " << iIndex << ")\n";

        auto outputKey = AdjacencyMap::packKey(oIndex, oPort);
        auto inputKey = AdjacencyMap::packKey(iIndex, iPort);

        adjacencyMap.addAdjacency(inputKey, outputKey);
        return true;
    }

    AdjacencyMap adjacencyMap;
    std::shared_ptr<NodeContext> context;

    std::vector<AudioNode*> objectsToCleanup;
};
