#include <vector>
#include "../AudioPort.h"

#pragma once

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
    std::vector<AudioInputPort> inputPorts;
    AudioPort outputPort;
    NodeContext* context;
    std::string name;

public:
    AudioNode(NodeContext* context, std::string nodeName)
        : context(context)
        , name(nodeName)
        , outputPort(this, "output")
    {}

    virtual ~AudioNode()
    {
    }

    std::string getName() { return name; }

    // Add an input port (for dependency)
    void addInputPort(std::string portName)
    {
        inputPorts.emplace_back(portName);
    }

    void linkInputPort(AudioPort* portToLink, int inputPortIndex)
    {
        inputPorts[inputPortIndex].connectedPorts.push_back(portToLink);
    }

    void process(float* buffer, unsigned long frameCount)
    {
        outputPort.clear(frameCount);
        processAudio(buffer, frameCount);
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) = 0;

    // Method to get input ports for sorting
    AudioPort* getOutputPort()
    {
        return &outputPort;
    }

    std::vector<AudioInputPort>& getInputPorts() { return inputPorts; };

};