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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// NodeContext to store common properties for nodes like sample rate
class NodeContext {
public:
    float sampleRate;  // Sample rate for the node
    int frameCount;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {}
};

// Abstract AudioNode class
class AudioNode {
protected:
    std::vector<AudioPort> inputPorts;
    AudioPort* outputPort = nullptr;
    NodeContext* context;
    std::string name;
    bool hasNoOutputPort = false;

public:
    AudioNode(NodeContext* context, std::string nodeName)
        : context(context)
        , name(nodeName)
    {}

    virtual ~AudioNode()
    {
    }

    void setHasNoOutputPort()
    {
        hasNoOutputPort = true;
    }

    std::string getName() { return name; }

    // Add an input port (for dependency)
    void addInputPort(std::string portName)
    {
        inputPorts.emplace_back(this, portName);
    }

    void linkOutputPort(AudioPort* inputPort)
    {
        outputPort = inputPort;
    }

    void process(float* buffer, unsigned long frameCount)
    {
        // check if the node has an output port to write to
        // output nodes (currently only AudioOutput) don't have an output port
        if (outputPort || hasNoOutputPort)
            processAudio(buffer, frameCount);
    }

    void clearBuffers(int frameCount)
    {
        for (auto& port : inputPorts)
        {
            port.clear(frameCount);
        }
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) = 0;

    // Method to get input ports for sorting
    AudioPort* getInputPort(int index)
    {
        return &inputPorts.at(index);
    }

    const std::vector<AudioPort>& getInputPorts() const { return inputPorts; }
};

// SineWaveNode that generates sine wave audio
class SineWaveNode : public AudioNode {
    float phase = 0.0f;

public:
    SineWaveNode(NodeContext* context) : AudioNode(context, "SineWaveNode")
    {
        addInputPort("frequency");
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort->getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] += (0.5f * std::sin(phase));
            phase += 2.0f * M_PI * inputPorts[0].getAudioBuffer()[i] / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};

// AddNode that sums two signals
class AddNode : public AudioNode {

public:
    AddNode(NodeContext* context) : AudioNode(context, "AddNode")
    {
        addInputPort("A");
        addInputPort("B");
    }

    void processAudio(float* out, unsigned long frameCount) override {
        if (inputPorts.size() >= 2) {
            const float* buffer1 = inputPorts[0].getAudioBuffer();
            const float* buffer2 = inputPorts[1].getAudioBuffer();
            auto outputBuffer = outputPort->getAudioBuffer();

            for (unsigned long i = 0; i < frameCount; i++) {
                outputBuffer[i] += buffer1[i] + buffer2[i];  // Directly write to output buffer
            }
        }
    }
};

// ValueNode that provides a constant value (e.g., for frequency modulation)
class ValueNode : public AudioNode
{
    float value;

public:
    ValueNode(NodeContext* context, float value) : AudioNode(context, "ValueNode"), value(value) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort->getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] += value;
        }
    }
};

class Metro : public AudioNode
{
    uint64_t sampleCounter = 0;
    uint64_t tickInterval;

public:
    Metro(NodeContext* context, float hz) : AudioNode(context, "Metro")
    {
        tickInterval = static_cast<uint64_t>(context->sampleRate / hz);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        unsigned long samplesProcessed = 0;

        while (samplesProcessed < frameCount)
        {
            unsigned long samplesUntilNextTick = static_cast<unsigned long>(tickInterval - sampleCounter);

            if (samplesUntilNextTick >= (frameCount - samplesProcessed))
            {
                sampleCounter += frameCount - samplesProcessed;
                break;
            }

            unsigned long tickPosition = samplesProcessed + samplesUntilNextTick;
            outputPort->addEvent(tickPosition);

            sampleCounter = 0;
            samplesProcessed = tickPosition + 1;
        }
    }
};

class Envelope : public AudioNode
{
    float attackVal;
    float decayVal;
    float envValue = 0.0f;
    bool isAttack = true;  // Track whether the envelope is in attack phase

public:
    Envelope(NodeContext* context, float attackVal, float decayVal)
        : AudioNode(context, "Envelope")
        , attackVal(attackVal * (context->sampleRate / 1000))
        , decayVal(decayVal * (context->sampleRate / 1000))
    {
        addInputPort("Events");
        addInputPort("Signal");
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto output = outputPort->getAudioBuffer();
        auto events = inputPorts[0].getEvents();
        auto signal = inputPorts[1].getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++)
        {
            if (!events->empty() && events->back().timeStamp == i) {
                envValue = 0.0f;  // Reset envelope at the start of the event
                isAttack = true;  // Start attack phase
                events->pop_back();
            }

            if (isAttack)
            {
                // Attack phase: Ramp up from 0 to 1
                envValue += (1.0f / attackVal);
                if (envValue >= 1.0f) {
                    envValue = 1.0f;
                    isAttack = false;  // Switch to decay phase after reaching 1
                }
            }
            else
            {
                // Decay phase: Ramp down from 1 towards 0
                envValue -= (1.0f / decayVal);
                if (envValue <= 0.0f) {
                    envValue = 0.0f;
                }
            }

            // Apply envelope to the signal
            output[i] += signal[i] * envValue;
        }
    }
};

// LFONode that modulates a value (e.g., frequency modulation)
class LFONode : public AudioNode {
    float frequency;
    float phase = 0.0f;

public:
    LFONode(NodeContext* context, float frequency) : AudioNode(context, "LFONode"), frequency(frequency) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort->getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] += 0.5f * std::sin(phase);
            phase += 2.0f * M_PI * frequency / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};

// VolumeNode that multiplies the outputs of two input nodes
class VolumeNode : public AudioNode {
public:
    VolumeNode(NodeContext* context) : AudioNode(context, "VolumeNode")
    {
        addInputPort("A");
        addInputPort("B");
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        if (inputPorts.size() >= 2) {
            const float* buffer1 = inputPorts[0].getAudioBuffer();
            const float* buffer2 = inputPorts[1].getAudioBuffer();
            auto output = outputPort->getAudioBuffer();

            for (unsigned long i = 0; i < frameCount; i++) {
                output[i] += buffer1[i] * buffer2[i];  // Multiply the two input signals
            }
        }
    }
};

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOutNode : public AudioNode {
public:
    AudioOutNode(NodeContext* context) : AudioNode(context, "AudioOutNode")
    {
        addInputPort("Signal");
        setHasNoOutputPort();
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        // The input port audio is directly sent to the PortAudio stream
        std::copy(inputPorts[0].getAudioBuffer(), inputPorts[0].getAudioBuffer() + frameCount, buffer);
    }
};

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
        nodes.at(oNode)->linkOutputPort(nodes.at(iNode)->getInputPort(iPort));
    }

    void topologicalSort(std::vector<AudioNode*>& sortedNodes) {
        std::stack<AudioNode*> stack;
        std::unordered_map<AudioNode*, bool> visited;
        std::unordered_map<AudioNode*, bool> inStack; // Track nodes in current DFS stack

        // Helper function to perform DFS
        std::function<void(AudioNode*)> dfs = [&](AudioNode* node) {
            if (inStack[node]) {
                std::cerr << "Cycle detected at node: " << node->getName() << std::endl;
                return; // If we encounter a cycle, we return immediately
            }

            if (visited[node]) return;

            // Mark the node as visited and part of the current DFS stack
            visited[node] = true;
            inStack[node] = true;

            // Traverse input ports
            for (auto& inputPort : node->getInputPorts()) {
                AudioNode* inputAudioNode = inputPort.getParentNode();
                if (inputAudioNode && !visited[inputAudioNode]) {
                    dfs(inputAudioNode);
                }
            }

            inStack[node] = false; // Remove from current stack after processing

            // After processing all dependencies, push the node to the stack
            stack.push(node);
        };

        // Perform DFS on all nodes to ensure proper order
        for (auto& node : nodes) {
            if (!visited[node.get()]) {
                dfs(node.get());
            }
        }

        // Pop nodes from stack and push them to sortedNodes
        while (!stack.empty()) {
            sortedNodes.push_back(stack.top());
            stack.pop();
        }

        // If you want to reverse the order so that it is in the correct "sorted" order:
        std::reverse(sortedNodes.begin(), sortedNodes.end());
    }


    void sortNodes()
    {
        topologicalSort(sortedNodes);
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
        for (auto& node : sortedNodes) {
            node->clearBuffers(frameCount);
        }

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
