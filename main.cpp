/*
// Copyright (c) 2024 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stack>
#include <functional>
#include <fstream>
#include <windows.h>
#include <thread>
#include <atomic>
#include <conio.h>

#include <PortAudio.h>
#include "external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include "AudioPort.h"
#include "Utility/Hash.h"
#include "Nodes/AudioNodes.h"
#include "Nodes/SineWaveNode.h"
#include "Nodes/AddNode.h"
#include "Nodes/ValueNode.h"
#include "Nodes/MetroNode.h"
#include "Nodes/EnvelopeNode.h"
#include "Nodes/LFONode.h"
#include "Nodes/VolumeNode.h"
#include "Nodes/AudioOut.h"

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
private:
    std::vector<std::unique_ptr<AudioNode>> nodes;
    std::vector<AudioNode*> sortedNodes;

    NodeContext* context;

public:
    AudioGraph(NodeContext* context) : context(context){}

    void loadPatch(json patch) {
        auto createObject = [this](json node)
        {
            auto const object = node["type"].get<std::string>();

            switch (hash(object))
            {
            case hash("Envelope"):
                {
                    // TODO: Check the value exists, otherwise will crash
                    auto const attackVal = node["attack"].get<float>();
                    auto const decayVal = node["decay"].get<float>();
                    nodes.push_back(std::make_unique<Envelope>(context, attackVal, decayVal));
                }
                break;
            case hash("Metro"):
                {
                    auto const value = node["hz"].get<float>();
                    nodes.push_back(std::make_unique<Metro>(context, value));
                }
                break;
            case hash("Value"):
                {
                    auto const value = node["value"].get<float>();
                    nodes.push_back(std::make_unique<ValueNode>(context, value));
                }
                break;
            case hash("LFO"):
                {
                    auto const rate = node["rate"].get<float>();
                    nodes.push_back(std::make_unique<LFONode>(context, rate));
                }
                break;
            case hash("Volume"):
                {
                    nodes.push_back(std::make_unique<VolumeNode>(context));
                }
                break;
            case hash("Sine"):
                {
                    nodes.push_back(std::make_unique<SineWaveNode>(context));
                }
                break;
            case hash("AudioOut"):
                {
                    nodes.push_back(std::make_unique<AudioOutNode>(context));
                }
                break;
            default:
                break;
            }
        };

        // Create nodes
        for (const auto& node : patch["nodes"]) {
            createObject(node);
        }

        // Create connections
        for (const auto& connection : patch["connections"]) {
            connect(connection["sourceNode"], connection["sourcePort"], connection["targetNode"], connection["targetPort"]);
        }

        sortNodes();
    }

    // Connect nodes dynamically by addressing them by order of addition
    void connect(int oNode, int oPort, int iNode, int iPort) {
        nodes.at(iNode)->linkInputPort(nodes.at(oNode)->getOutputPort(), iPort);
    }

    // This helper scans ALL nodes to find which nodes are downstream of `node`.
    std::vector<AudioNode*> getDownstreamNodes(AudioNode* node, const std::vector<std::unique_ptr<AudioNode>>& allNodes)
    {
        std::vector<AudioNode*> result;
        AudioPort* myOutputPort = node->getOutputPort();

        // Iterate by reference: auto& or const auto&
        for (auto& otherNode : allNodes)
        {
            if (otherNode.get() == node)
                continue; // skip self

            // Check each named input port
            for (const auto& inputPort : otherNode->getInputPorts())
            {
                // Each inputPort can have multiple connections
                for (auto* connected : inputPort.connectedPorts)
                {
                    // If otherNode’s input is connected to *this* node’s output,
                    // we have an edge: node -> otherNode
                    if (connected == myOutputPort)
                    {
                        result.push_back(otherNode.get());
                        goto NextOtherNode;
                    }
                }
            }
            NextOtherNode:;
        }

        return result;
    }


    // DFS-based topological sort that builds adjacency from "node -> its downstream nodes".
    void topologicalSort(std::vector<AudioNode*>& sortedNodes)
    {
        std::stack<AudioNode*> stack;
        std::unordered_map<AudioNode*, bool> visited;
        std::unordered_map<AudioNode*, bool> inStack; // for cycle detection

        // Recursive DFS lambda
        std::function<void(AudioNode*)> dfs = [&](AudioNode* node)
        {
            if (inStack[node]) {
                std::cerr << "Cycle detected at node: " << node->getName() << std::endl;
                return;
            }
            if (visited[node]) {
                return;
            }

            visited[node] = true;
            inStack[node] = true;

            // Get all nodes that depend on this node's output
            auto downstreamNodes = getDownstreamNodes(node, nodes);

            for (auto* downstream : downstreamNodes) {
                if (!visited[downstream]) {
                    dfs(downstream);
                }
            }

            inStack[node] = false;
            stack.push(node);
        };

        // Initiate DFS from every node that isn’t visited yet
        for (auto& node : nodes) {
            if (node && !visited[node.get()]) {
                dfs(node.get());
            }
        }

        // Pop from the stack to sortedNodes, then reverse for final topological order
        while (!stack.empty()) {
            sortedNodes.push_back(stack.top());
            stack.pop();
        }
        //std::reverse(sortedNodes.begin(), sortedNodes.end());
    }


    void sortNodes()
    {
        topologicalSort(sortedNodes);
#define DEBUG_SORT
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
        for (auto& node : sortedNodes) {
            node->process(buffer, frameCount);
        }
    }
};

class Graphs
{
protected:
    std::unique_ptr<AudioGraph> activeGraph;
    std::unique_ptr<AudioGraph> transitioningGraph;
    NodeContext* ctx;
    std::vector<float> fadeOutBuffer;
    std::vector<float> fadeInBuffer;
    bool isTransitioning = false;

public:
    Graphs(NodeContext* context)
        : ctx(context)
    {
        // Resize fade buffers to match the frane count, one frame xfade for now
        fadeOutBuffer.resize(ctx->frameCount);
        fadeInBuffer.resize(ctx->frameCount);

        // Fill the fade buffers with linear fade values
        for (unsigned long i = 0; i < ctx->frameCount; ++i)
        {
            fadeOutBuffer[i] = 1.0f - (static_cast<float>(i) / ctx->frameCount);
            fadeInBuffer[i] = static_cast<float>(i) / ctx->frameCount;
        }
    }

    void setActiveGraph(json patch)
    {
        auto newGraph = std::make_unique<AudioGraph>(ctx);
        newGraph->loadPatch(patch);

        if (activeGraph)
        {
            std::cout << "transition to new graph" << std::endl;
            // Start transition if there's an active graph
            transitioningGraph = std::move(newGraph);
            isTransitioning = true;
        }
        else
        {
            // If no active graph, directly assign
            activeGraph = std::move(newGraph);
        }
    }

    void process(float* buffer, unsigned long frameCount)
    {
        if (isTransitioning)
        {
            // Temporary buffers for processing
            std::vector<float> activeBuffer(frameCount, 0.0f);
            std::vector<float> transitionBuffer(frameCount, 0.0f);

            // Process each graph into its temporary buffer
            if (activeGraph)
                activeGraph->process(activeBuffer.data(), frameCount);

            if (transitioningGraph)
                transitioningGraph->process(transitionBuffer.data(), frameCount);

            // Apply the fade to the buffers
            for (unsigned long i = 0; i < frameCount; ++i)
            {
                float fadeFactor = i / static_cast<float>(frameCount);

                // Mix the faded buffers into the output buffer
                buffer[i] = (activeBuffer[i] * (1 - fadeFactor)) + (transitionBuffer[i] * fadeFactor);
            }

            activeGraph = std::move(transitioningGraph);
            isTransitioning = false;
        }
        else if (activeGraph)
        {
            // Only process the active graph if no transition is occurring
            activeGraph->process(buffer, frameCount);
        }
    }
};


// PortAudio Callback
static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graphs = static_cast<Graphs*>(userData);
    float* out = (float*)output;

    graphs->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cout << "under of over flow" << std::endl;
    }

    return paContinue;
}

std::atomic<bool> running(true); // Flag to control the loop

// Function to handle user input for commands and Escape key detection
void commandListener(std::function<void(std::string& patchToLoad)> callback) {
    std::string input;
    std::cout << "Press Escape to close app, type \"load file\" to load graph" << std::endl;
    while (running) {
        // Check if Escape key (VK_ESCAPE) is pressed
        if (GetAsyncKeyState(VK_ESCAPE)) {
            std::cout << "Escape key pressed. Exiting..." << std::endl;
            running = false;
            break;
        }

        // Non-blocking check for keyboard input
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') {
                if (input.rfind("load ", 0) == 0) { // Check if the command starts with "load "
                    std::string filename = input.substr(5); // Get the file name after "load "
                    std::cout << "\nLoading graph from file: " << filename << "..." << std::endl;
                    callback(filename);
                    Sleep(500);
                    std::cout << filename + ".json loaded successfully!" << std::endl;
                } else {
                    std::cout << "\nInvalid command!" << std::endl;
                }
                input.clear();
            } else {
                input += ch;
                std::cout << ch;
            }
        }

        Sleep(10);
    }
}
int main() {
    PaError err;
    unsigned long frameCount = 256;
    float sampleRate = 44100.0f;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto context = std::make_unique<NodeContext>(sampleRate, frameCount);

    Graphs graphs(context.get());

    // Set up PortAudio stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, sampleRate, frameCount, audioCallback, &graphs);
    if (err != paNoError) {
        std::cerr << "PortAudio stream setup failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto streamInfo = Pa_GetStreamInfo(stream);
    if (streamInfo != nullptr) {
        std::cout << "Sample Rate: " << streamInfo->sampleRate << std::endl;
        std::cout << "input latency: " << streamInfo->inputLatency << " output latency: " << streamInfo->outputLatency << std::endl;
    }

    auto callback = [&graphs](std::string& patchToLoad) {
        char buffer[MAX_PATH];
        DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
        if (length == 0) {
            std::cerr << "Error getting current directory." << std::endl;
        } else {
            std::cout << "Current working directory: " << buffer << std::endl;
        }

        std::ifstream file(patchToLoad + ".json");
        if (!file.is_open()) {
            std::cerr << "Could not open the file!" << std::endl;
            return;
        }

        json patch;
        file >> patch;
        file.close();

        graphs.setActiveGraph(patch);
    };

    // Start a thread for command input and pass a callback using std::bind
    std::thread commandThread(std::bind(commandListener, callback));

    // Wait for the command thread to finish
    if (commandThread.joinable()) {
        commandThread.join();
    }

    // Stop and clean up
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stop failed: " << Pa_GetErrorText(err) << std::endl;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream close failed: " << Pa_GetErrorText(err) << std::endl;
    }

    Pa_Terminate();

    return 0;
}
