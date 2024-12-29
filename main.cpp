#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stack>
#include <PortAudio.h>
#include <functional>

#include "AudioPort.h"

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

public:
    AudioNode(NodeContext* context, std::string nodeName)
        : context(context)
        , name(nodeName)
    {}

    virtual ~AudioNode() {}

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
        if (outputPort)
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
            output[i] += value;  // Directly assign the value to outputBuffer
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
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        // The input port audio is directly sent to the PortAudio stream
        std::copy(inputPorts[0].getAudioBuffer(), inputPorts[0].getAudioBuffer() + frameCount, buffer);
    }
};

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
private:
    std::vector<AudioNode*> nodes;
    std::vector<AudioNode*> sortedNodes;

public:
    void addNode(AudioNode* node) {
        nodes.push_back(node);
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
            if (!visited[node]) {
                dfs(node);
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

        std::cout << "======== presort =======" << std::endl;
        for (auto& node : nodes)
        {
            std::cout << "node: " << node->getName() << std::endl;
        }

        std::cout << "======== sorted =======" << std::endl;
        for (auto node : sortedNodes)
            std::cout << "node graph: " << node->getName() << std::endl;
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

    std::fill(out, out + frameCount, 0.0f);

    graph->process(out, frameCount);  // Process the audio graph
    return paContinue;
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

    auto context = std::make_unique<NodeContext>(sampleRate);

    // Create nodes independently
    auto valueNode = std::make_unique<ValueNode>(context.get(), 440.0f);
    auto sineNode = std::make_unique<SineWaveNode>(context.get());

    auto valueNode2 = std::make_unique<ValueNode>(context.get(), 666.0f);
    auto sineNode2 = std::make_unique<SineWaveNode>(context.get());

    auto valueNode3 = std::make_unique<ValueNode>(context.get(), 320.0f);
    auto sineNode3 = std::make_unique<SineWaveNode>(context.get());

    auto valueNode4 = std::make_unique<ValueNode>(context.get(), 720.0f);
    auto sineNode4 = std::make_unique<SineWaveNode>(context.get());

    auto lfoNode = std::make_unique<LFONode>(context.get(), 1.0f);
    auto volumeNode = std::make_unique<VolumeNode>(context.get());
    auto audioOutNode = std::make_unique<AudioOutNode>(context.get());
    auto sumNode = std::make_unique<AddNode>(context.get());
    auto sumNode2 = std::make_unique<AddNode>(context.get());

    // Create the graph and add nodes
    AudioGraph graph;
    graph.addNode(valueNode.get());     // 0
    graph.addNode(lfoNode.get());       // 1
    graph.addNode(volumeNode.get());    // 2
    graph.addNode(sineNode.get());      // 3
    graph.addNode(audioOutNode.get());  // 4

    // Connect nodes
    graph.connect(0, 0, 2, 0);
    graph.connect(1, 0, 2, 1);
    graph.connect(2, 0, 3, 0);
    graph.connect(3, 0, 4, 0);

    graph.sortNodes();

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

    // Run for 5 seconds
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
