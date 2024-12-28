#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <PortAudio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Abstract AudioNode class
class AudioNode {
protected:
    std::vector<AudioNode*> dependencies;  // Store dependencies (input nodes)
    float* processedBuffer;  // Store processed output
public:
    virtual ~AudioNode() {
        delete[] processedBuffer;
    }

    // Virtual method for processing the audio buffer
    virtual void process(float* buffer, unsigned long frameCount) = 0;

    // Method to get the processed buffer after processing
    float* getProcessedBuffer() { return processedBuffer; }

    // Method to store the processed data in the buffer
    void setProcessedBuffer(float* buffer, unsigned long frameCount) {
        processedBuffer = new float[frameCount];
        std::copy(buffer, buffer + frameCount, processedBuffer);
    }

    // Add a dependency (input node)
    void addDependency(AudioNode* node) {
        dependencies.push_back(node);
    }

    // Method to get node dependencies
    virtual std::vector<AudioNode*> getDependencies() {
        return dependencies;
    }

    // Method to check if a node has been processed
    virtual bool isProcessed() = 0;
};

// SineWaveNode that generates sine wave audio
class SineWaveNode : public AudioNode {
    float phase = 0.0f;
    AudioNode* frequencyNode;
    float sampleRate;

public:
    SineWaveNode(float sampleRate) : frequencyNode(nullptr), sampleRate(sampleRate) {}

    void setFrequencyNode(AudioNode* freqNode) {
        frequencyNode = freqNode;
    }

    void process(float* buffer, unsigned long frameCount) override {
        if (frequencyNode) {
            float* freqBuffer = frequencyNode->getProcessedBuffer();
            for (unsigned long i = 0; i < frameCount; i++) {
                float frequency = freqBuffer[i];
                buffer[i] = 0.5f * std::sin(phase);
                phase += 2.0f * M_PI * frequency / sampleRate;
                if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
            }
            setProcessedBuffer(buffer, frameCount);
        }
    }

    bool isProcessed() override { return processedBuffer != nullptr; }
};

// ValueNode that provides a constant value (e.g., for frequency modulation)
class ValueNode : public AudioNode {
    float value;

public:
    ValueNode(float value) : value(value) {}

    void process(float* buffer, unsigned long frameCount) override {
        std::fill(buffer, buffer + frameCount, value);
        setProcessedBuffer(buffer, frameCount);
    }

    bool isProcessed() override { return processedBuffer != nullptr; }
};

// LFO Node that modulates a value (e.g., frequency modulation)
class LFONode : public AudioNode {
    float frequency;
    float sampleRate;
    float phase = 0.0f;

public:
    LFONode(float frequency, float sampleRate) : frequency(frequency), sampleRate(sampleRate) {}

    void process(float* buffer, unsigned long frameCount) override {
        for (unsigned long i = 0; i < frameCount; i++) {
            buffer[i] = 0.5f * std::sin(phase);
            phase += 2.0f * M_PI * frequency / sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
        setProcessedBuffer(buffer, frameCount);
    }

    bool isProcessed() override { return processedBuffer != nullptr; }
};

// VolumeNode that multiplies the outputs of two input nodes
class VolumeNode : public AudioNode {
public:
    void connectInputs(AudioNode* input1, AudioNode* input2) {
        addDependency(input1);
        addDependency(input2);
    }

    void process(float* buffer, unsigned long frameCount) override {
        if (dependencies.size() >= 2) {
            float* buffer1 = dependencies[0]->getProcessedBuffer();
            float* buffer2 = dependencies[1]->getProcessedBuffer();

            for (unsigned long i = 0; i < frameCount; i++) {
                buffer[i] = buffer1[i] * buffer2[i];
            }
            setProcessedBuffer(buffer, frameCount);
        }
    }

    bool isProcessed() override { return processedBuffer != nullptr; }
};

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOutNode : public AudioNode {
public:
    void process(float* buffer, unsigned long frameCount) override {
        // The buffer is directly sent to the PortAudio stream
        setProcessedBuffer(buffer, frameCount);
    }

    bool isProcessed() override { return true; }  // Always "processed" since it just outputs the audio
};

// AudioGraph to manage nodes and process them in the correct order
class AudioGraph {
    std::vector<AudioNode*> nodes;

    // Topological sort
    void topologicalSort(std::vector<AudioNode*>& sortedNodes) {
        std::vector<AudioNode*> toProcess = nodes;
        std::vector<AudioNode*> processed;

        while (!toProcess.empty()) {
            bool progress = false;

            for (auto it = toProcess.begin(); it != toProcess.end(); ) {
                AudioNode* node = *it;
                bool ready = true;

                // Check if all dependencies are processed
                for (auto dep : node->getDependencies()) {
                    if (!dep->isProcessed()) {
                        ready = false;
                        break;
                    }
                }

                if (ready) {
                    sortedNodes.push_back(node);
                    processed.push_back(node);
                    it = toProcess.erase(it);  // Remove the node from processing list
                    progress = true;
                } else {
                    ++it;
                }
            }

            if (!progress) {
                std::cerr << "Circular dependency detected!" << std::endl;
                break;
            }
        }
    }

public:
    void addNode(AudioNode* node) {
        nodes.push_back(node);
    }

    void process(float* buffer, unsigned long frameCount) {
        std::vector<AudioNode*> sortedNodes;
        topologicalSort(sortedNodes);

        // Process nodes in topological order
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

    // Create nodes independently (without connecting them immediately)
    auto* valueNode = new ValueNode(440.0f);  // Set a constant frequency of 440 Hz
    auto* lfoNode = new LFONode(0.2f, sampleRate);  // LFO node (for modulation)
    auto* sineNode = new SineWaveNode(sampleRate);  // Sine wave node
    auto* volumeNode = new VolumeNode();  // Volume node to multiply two inputs
    auto* audioOutNode = new AudioOutNode();  // AudioOutNode to send output to PortAudio

    // Create the graph and add nodes
    AudioGraph graph;
    graph.addNode(valueNode);
    graph.addNode(lfoNode);
    graph.addNode(sineNode);
    graph.addNode(volumeNode);
    graph.addNode(audioOutNode);

    // Connect the nodes later (after creation)
    sineNode->setFrequencyNode(valueNode);  // Set the value node as frequency input for sine node
    volumeNode->connectInputs(sineNode, lfoNode);  // Connect sine and LFO to volume node

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

    // Clean up nodes
    delete valueNode;
    delete lfoNode;
    delete sineNode;
    delete volumeNode;
    delete audioOutNode;

    return 0;
}
