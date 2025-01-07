/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include "AudioPort.h"
#include "Graph/NodeContext.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

    enum class State { Unvisited, Visiting, Visited };
    State state = State::Unvisited;

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