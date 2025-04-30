/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <iostream>
#include <UI_ToolKit/PlatformHelpers.h>
#include <filesystem>

#include "json.hpp"
using json = nlohmann::json;

#include "glaze/glaze.hpp"
#include "../Nodes/AllNodes.h"
#include "Edge.h"
#include "GraphHolder.h"

#undef max

class GraphManager
{
public:
    GraphManager(int sampleRate, unsigned long frameCount)
        : ctx(std::make_shared<NodeContext>(sampleRate, frameCount))
    {
    }

    explicit GraphManager(GraphManager* otherGM)
        : parentGraph(otherGM)
    {
        assert(otherGM && "GraphManager* cannot be nullptr");
        ctx = std::make_shared<NodeContext>(otherGM->ctx->sampleRate, otherGM->ctx->frameCount);
    }

    ~GraphManager()
    {
    }

    AudioNode* addObject(const json& jsonObj)
    {
        std::cout << "adding object" << std::endl;
        if (!activeGraph)
        {
            activeGraph = std::make_shared<GraphHolder>(ctx, this);
        }

        // TODO: Lock the graph, or communicate via a queue

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

        auto newNode = transitioningGraph->createObject(jsonObj);

        setDirty(true);

        updateAndFinalizeGraph();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);

        return newNode;
    }

    std::vector<Edge*> removeObject(int id)
    {
        if (!activeGraph)
            return {};

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

        transitioningGraph->removeObject(id);

        setDirty(true);

        updateAndFinalizeGraph();

        auto connectionState = transitioningGraph->getConnections();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);

        return connectionState;
    }

    std::vector<Edge*> removeObjects(std::vector<int>& ids, std::vector<uint64_t>& edgeHashes)
    {
        if (!activeGraph)
            return {};

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

        for (auto id : ids)
        {
            transitioningGraph->removeObject(id);
        }

        for (auto edgeHash : edgeHashes)
        {
            transitioningGraph->removeEdge(edgeHash);
        }

        setDirty(true);

        updateAndFinalizeGraph();

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

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

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

        setDirty(true);

        updateAndFinalizeGraph();

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

        setDirty(true);

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);
        // Add the connection to the transitioning graph
        if (!transitioningGraph->disconnect(oObj, oPort, iObj, iPort))
        {
            std::cerr << "Failed to connect objects in the transitioning graph." << std::endl;
            transitioningGraph.reset(); // Discard transitioning graph
            return false;
        }

        updateAndFinalizeGraph();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
        return true;
    }

    void prepareObjectsToCleanup() const
    {
        if (!activeGraph || !transitioningGraph)
            return;

        auto oldObjs = activeGraph->getObjects();
        auto newObjs = transitioningGraph->getObjects();

        ankerl::unordered_dense::set<AudioNode*> newNodes;
        for (auto* node : newObjs)
            newNodes.insert(node);

        auto* graph = activeGraph->getGraph();
        graph->objectsToCleanup.clear();

        for (auto* node : oldObjs)
        {
            if (!newNodes.contains(node))
                graph->objectsToCleanup.push_back(node);
        }
    }

    void regenerateGraph()
    {
        if (!activeGraph)
            return;

        transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

        setDirty(true);

        updateAndFinalizeGraph();

        graphModifiedCallback();

        // Mark the transitioning graph as ready to replace the active graph
        swapGraph.store(true, std::memory_order_release);
    }

    void updateAndFinalizeGraph()
    {
        prepareObjectsToCleanup();
        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();
        transitioningGraph->removeInvalidConnections();

        if (owningSubpatch)
        {
            owningSubpatch->rebuildPortsFromGraph(*transitioningGraph);

            if (parentGraph)
            {
                parentGraph->regenerateGraph();
            }
        }
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

    std::tuple<std::vector<Object*>, std::vector<Edge*>> loadGraph(const std::string& patchPath, const json& patch,
                                                                        const bool logVerbose, std::function<void(std::shared_ptr<GraphHolder>&)> populateInletOutlets = [](std::shared_ptr<GraphHolder>&){})
    {
        patchLoadSuccess = false;

        if (swapGraph.load(std::memory_order_acquire))
        {
            std::cout << "Warning: Attempted to overwrite a transitioning graph before it was swapped." << std::endl;
            return {};
        }

        setDirty(false);

        filePath = patchPath;

        transitioningGraph = std::make_shared<GraphHolder>(ctx, this);

        if (!transitioningGraph->loadPatch(patch, logVerbose))
        {
            transitioningGraph.reset();
            std::cerr << "Corrupt patch, failed to load." << std::endl;
            return {};
        }

        prepareObjectsToCleanup();

        patchLoadSuccess = true;
        transitioningGraph->updateConnections();
        transitioningGraph->sortNodes();
        transitioningGraph->updateOutputInputPortMap();

        populateInletOutlets(transitioningGraph);

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
        if (!transitioningGraph)
            return objects;
        for (auto* aNode : transitioningGraph->getObjects())
        {
            if (auto object = reinterpret_cast<Object*>(aNode->getOrCreateUI()))
            {
                objects.push_back(object);
            }
        }
        return objects;
    }

    std::vector<Object*> getActiveObjects()
    {
        std::vector<Object*> objects;
        for (auto* aNode : activeGraph->getObjects())
        {
            if (auto object = reinterpret_cast<Object*>(aNode->getOrCreateUI()))
            {
                objects.push_back(object);
            }
        }
        return objects;
    }

    GraphHolder* getActiveGraph()
    {
        return activeGraph.get();
    }

    std::vector<Edge*> getConnections() const
    {
        return activeGraph ? activeGraph->getConnections() : std::vector<Edge*>{};
    }

    const std::string& getPatchFile()
    {
        return filePath;
    }

    json graphToJSON()
    {
        setDirty(false);
        return activeGraph->graphToJSON();
    }

    json copySelected(const std::vector<uint32_t>& selectedNodeIDs)
    {
        if (activeGraph)
        {
            return activeGraph->serializeSelectedNodes(selectedNodeIDs);
        }
        return { };
    }

std::tuple<std::vector<Object*>, std::vector<Object*>, std::vector<Edge*>> pasteGraph(const json& patch)
{
    std::vector<Object*> pastedObjects;

    // Ensure we have an active graph.
    if (!activeGraph)
    {
        activeGraph = std::make_shared<GraphHolder>(ctx, this);
    }

    // Create a transitioning graph from the active graph.
    transitioningGraph = std::make_shared<GraphHolder>(*activeGraph);

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

    updateAndFinalizeGraph();

    auto loadedObjects = getObjects();
    auto connections = transitioningGraph->getConnections();

    // Mark the transitioning graph ready to replace the active graph.
    swapGraph.store(true, std::memory_order_release);

    return { pastedObjects, loadedObjects, connections };
}

    void swapGraphState()
    {
        if (swapGraph.load(std::memory_order_acquire))
        {
            if (activeGraph)
                activeGraph->processCleanup();

            // Perform the swap on the audio thread
            activeGraph.swap(transitioningGraph);

            swapGraph.store(false, std::memory_order_release);
        }
    }


    void process(const float* inBuffer, float* outBuffer, unsigned long frameCount, std::vector<MidiMessage>& message)
    {
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
        swapGraphState();

        // Process the current graph
        if (activeGraph)
        {
            activeGraph->process(inBuffer, outBuffer, frameCount, message);
        }
    }

    void setFilePath(const std::string& newPath)
    {
        filePath = newPath;
    }

    std::atomic<bool> flaggedForDeletion = false;

    // Queue size would be largest 8 if 64 buffrer size at 44100 hz and a video refresh rate of 120 hz
    moodycamel::ConcurrentQueue<std::array<float, 2>> volumeMeterQueue = moodycamel::ConcurrentQueue<std::array<float, 2>>(100);

    std::function<void()> graphModifiedCallback;

    bool getIsGraphDirty() const
    {
        return isGraphDirty;
    }

    void setDirty(const bool shouldBeDirty)
    {
        if (isGraphDirty != shouldBeDirty)
        {
            isGraphDirty = shouldBeDirty;
            if (graphModifiedCallback)
                graphModifiedCallback();
        }
    }

    GraphManager* parentGraph = nullptr;

private:
    // Take the average peak and send it to the GUI
    void processPeak(const float* buffer, unsigned long frameCount)
    {
        constexpr int kUpdateInterval = 4;
        const float* right = buffer + frameCount;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            accumulatedPeakL = std::max(accumulatedPeakL, std::abs(buffer[i]));
            accumulatedPeakR = std::max(accumulatedPeakR, std::abs(right[i]));
        }

        if (++peakFrameCounter >= kUpdateInterval)
        {
            peakFrameCounter = 0;
            volumeMeterQueue.enqueue({accumulatedPeakL, accumulatedPeakR});
            accumulatedPeakL = accumulatedPeakR = 0.0f;
        }
    }

protected:
    std::string filePath;

    bool isGraphDirty = false;

    std::shared_ptr<NodeContext> ctx;

    std::shared_ptr<GraphHolder> activeGraph; // Actively processed graph
    std::shared_ptr<GraphHolder> transitioningGraph; // New graph prepared for swapping
    std::atomic<bool> swapGraph = false; // Signal for readiness to swap

    bool patchLoadSuccess = false;

    int peakFrameCounter = 0;
    float accumulatedPeakL = 0.0f;
    float accumulatedPeakR = 0.0f;

    friend class Subpatch;
    Subpatch* owningSubpatch = nullptr;
};
