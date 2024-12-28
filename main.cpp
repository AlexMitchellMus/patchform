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
    std::vector<AudioPort*> inputPorts;
    AudioPort outputBuffer;
    NodeContext* context;
    std::string name;

public:
    AudioNode(NodeContext* context, std::string nodeName)
        : context(context)
        , name(nodeName)
        , outputBuffer(this)
    {}

    virtual ~AudioNode() {}

    std::string getName() { return name; }

    // Add an input port (for dependency)
    void addInputPort(AudioNode* node, int index) {
        inputPorts.push_back(&node->outputBuffer);
    }

    // Virtual method for processing the audio buffer
    virtual void process(float* buffer, unsigned long frameCount) = 0;

    // Get the output buffer
    AudioPort getProcessedBuffer() { return outputBuffer; }

    // Method to set the output buffer size
    void allocateOutputBuffer(unsigned long frameCount) {
        outputBuffer = AudioPort(this, frameCount);
    }

    // Method to get input ports for sorting
    const std::vector<AudioPort*>& getInputPorts() const {
        return inputPorts;
    }
};

// SineWaveNode that generates sine wave audio
class SineWaveNode : public AudioNode {
    float phase = 0.0f;

public:
    SineWaveNode(NodeContext* context) : AudioNode(context, "SineWaveNode") {}

    void process(float* out, unsigned long frameCount) override {
        allocateOutputBuffer(frameCount);  // Allocate memory for the output buffer

        for (unsigned long i = 0; i < frameCount; i++) {
            outputBuffer.getAudioBuffer()[i] = 0.5f * std::sin(phase);  // Directly assign to outputBuffer
            phase += 2.0f * M_PI * inputPorts[0]->getAudioBuffer()[i] / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};

// AddNode that sums two signals
class AddNode : public AudioNode {

public:
    AddNode(NodeContext* context) : AudioNode(context, "AddNode") {}

    void process(float* out, unsigned long frameCount) override {
        if (inputPorts.size() >= 2) {
            const float* buffer1 = inputPorts[0]->getAudioBuffer();
            const float* buffer2 = inputPorts[1]->getAudioBuffer();

            allocateOutputBuffer(frameCount);  // Allocate memory for the output buffer

            for (unsigned long i = 0; i < frameCount; i++) {
                outputBuffer.getAudioBuffer()[i] = buffer1[i] + buffer2[i];  // Directly write to output buffer
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

    void process(float* out, unsigned long frameCount) override {
        allocateOutputBuffer(frameCount);  // Allocate memory for the output buffer

        for (unsigned long i = 0; i < frameCount; i++) {
            outputBuffer.getAudioBuffer()[i] = value;  // Directly assign the value to outputBuffer
        }
    }
};

// LFONode that modulates a value (e.g., frequency modulation)
class LFONode : public AudioNode {
    float frequency;
    float phase = 0.0f;

public:
    LFONode(NodeContext* context, float frequency) : AudioNode(context, "LFONode"), frequency(frequency) {}

    void process(float* out, unsigned long frameCount) override {
        allocateOutputBuffer(frameCount);  // Allocate memory for the output buffer

        for (unsigned long i = 0; i < frameCount; i++) {
            outputBuffer.getAudioBuffer()[i] = 0.5f * std::sin(phase);
            phase += 2.0f * M_PI * frequency / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};

// VolumeNode that multiplies the outputs of two input nodes
class VolumeNode : public AudioNode {
public:
    VolumeNode(NodeContext* context) : AudioNode(context, "VolumeNode") {}

    void process(float* buffer, unsigned long frameCount) override {
        if (inputPorts.size() >= 2) {
            const float* buffer1 = inputPorts[0]->getAudioBuffer();
            const float* buffer2 = inputPorts[1]->getAudioBuffer();

            for (unsigned long i = 0; i < frameCount; i++) {
                buffer[i] = buffer1[i] * buffer2[i];  // Multiply the two input signals
            }
            allocateOutputBuffer(frameCount);
            std::copy(buffer, buffer + frameCount, outputBuffer.getAudioBuffer());
        }
    }
};

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOutNode : public AudioNode {
public:
    AudioOutNode(NodeContext* context) : AudioNode(context, "AudioOutNode") {}

    void process(float* buffer, unsigned long frameCount) override {
        // The buffer is directly sent to the PortAudio stream
        std::copy(inputPorts[0]->getAudioBuffer(), inputPorts[0]->getAudioBuffer() + frameCount, buffer);
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
        nodes.at(iNode)->addInputPort(nodes.at(oNode), oPort);
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
                AudioNode* inputAudioNode = inputPort->getParentNode();
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

    // Process nodes in sorted order
    void process(float* buffer, unsigned long frameCount) {

        // Process nodes in sorted order
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
    auto sineNode = std::make_unique<SineWaveNode>(context.get());
    auto sineNode2 = std::make_unique<SineWaveNode>(context.get());
    auto valueNode = std::make_unique<ValueNode>(context.get(), 440.0f);
    auto valueNode2 = std::make_unique<ValueNode>(context.get(), 666.0f);
    auto lfoNode = std::make_unique<LFONode>(context.get(), 20.0f);
    auto volumeNode = std::make_unique<VolumeNode>(context.get());
    auto audioOutNode = std::make_unique<AudioOutNode>(context.get());
    auto sumNode = std::make_unique<AddNode>(context.get());
    auto sumNode2 = std::make_unique<AddNode>(context.get());

    // Create the graph and add nodes
    AudioGraph graph;
    graph.addNode(valueNode.get());     // 0
    graph.addNode(sineNode.get());      // 1
    graph.addNode(valueNode2.get());    // 2
    graph.addNode(sineNode2.get());     // 3
    graph.addNode(sumNode.get());       // 4
    graph.addNode(audioOutNode.get());  // 5
    graph.addNode(volumeNode.get());    // 6
    graph.addNode(lfoNode.get());       // 7
    graph.addNode(sumNode2.get());      // 8

    // Connect nodes
    graph.connect(0, 0, 1, 0);
    graph.connect(2, 0, 3, 0);
    graph.connect(3, 0, 3, 0);
    graph.connect(1, 0, 4, 0);
    graph.connect(3, 0, 4, 1);
    graph.connect(3, 0, 6, 0);
    graph.connect(7, 0, 6, 1);
    graph.connect(6, 0, 8, 0);
    graph.connect(4, 0, 8, 1);
    graph.connect(8, 0, 5, 0);

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
