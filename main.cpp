#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stack>
#include <functional>
#include <fstream>
#include <windows.h>

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

    NodeContext(float sampleRate) : sampleRate(sampleRate) {}
};

// Abstract AudioNode class
class AudioNode {
protected:
    std::vector<AudioPort> inputPorts;
    AudioPort* outputPort;
    NodeContext* context;
    std::string name;
    bool hasOutputPort = false;

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
        hasOutputPort = true;
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
        if (outputPort || hasOutputPort)
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

            for (unsigned long i = 0; i < frameCount; i++) {
                outputPort->getAudioBuffer()[i] += buffer1[i] + buffer2[i];  // Directly write to output buffer
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

// LFONode that modulates a value (e.g., frequency modulation)
class LFONode : public AudioNode {
    float frequency;
    float phase = 0.0f;

public:
    LFONode(NodeContext* context, float frequency) : AudioNode(context, "LFONode"), frequency(frequency) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort->getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = 0.5f * std::sin(phase);
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

            for (unsigned long i = 0; i < frameCount; i++) {
                buffer[i] = buffer1[i] * buffer2[i];  // Multiply the two input signals
            }
            std::copy(buffer, buffer + frameCount, outputPort->getAudioBuffer());
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
            case hash("ValueNode"):
                {
                    auto const value = node["value"].get<float>();
                    nodes.push_back(std::make_unique<ValueNode>(context, value));
                }
                break;
            case hash("LFONode"):
                {
                    auto const rate = node["rate"].get<float>();
                    nodes.push_back(std::make_unique<LFONode>(context, rate));
                }
                break;
            case hash("VolumeNode"):
                {
                    nodes.push_back(std::make_unique<VolumeNode>(context));
                }
                break;
            case hash("SineNode"):
                {
                    nodes.push_back(std::make_unique<SineWaveNode>(context));
                }
                break;
            case hash("AudioOutNode"):
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

// PortAudio Callback
static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graph = static_cast<AudioGraph*>(userData);
    float* out = (float*)output;

    graph->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cout << "issue" << std::endl;
    }

    return paContinue;
}

int main() {
    PaError err;
    unsigned long frameCount = 64;
    float sampleRate = 44100.0f;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto context = std::make_unique<NodeContext>(sampleRate);

    char buffer[MAX_PATH];
    DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
    if (length == 0) {
        std::cerr << "Error getting current directory." << std::endl;
    } else {
        std::cout << "Current working directory: " << buffer << std::endl;
    }

    std::ifstream file("graph.json");
    if (!file.is_open()) {
        std::cerr << "Could not open the file!" << std::endl;
        return 1;
    }

    json patch;
    file >> patch;
    file.close();

    AudioGraph graph(context.get());
    graph.loadPatch(patch);

    // Set up PortAudio stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, sampleRate, frameCount, audioCallback, &graph);
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

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

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
