/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "../Graph/Event.h"

class AudioPort;
class AudioNode;

struct PortGroup
{
    uint8_t inputPortNumber;
    std::vector<AudioPort*> connectedPorts;
};

struct DownstreamPortGroup {
    uint8_t outputPortNumber;  // The source output port number on the current node.
    // Each pair holds a pointer to the downstream node and its corresponding input port.
    std::vector<std::pair<AudioNode*, int>> downstreamConnections;
};

using OutputPortMap = std::vector<std::vector<PortGroup>>;
using DownStreamPortMap = std::vector<std::vector<DownstreamPortGroup>>;

class AudioNode;

class AudioPort
{
public:
    enum PortType : uint8_t
    {
        None =   0,
        Signal = 1 << 0,
        Data   = 1 << 1
    };

    AudioPort(AudioNode* parent, const std::string& portName, PortType type)
        : node(parent)
        , name(portName)
        , portType(type)
    {
        events.reserve(1024);
    }

    float* getAudioBuffer() {
        return audioBuffer.data();
    }

    size_t getAudioBufferSize()
    {
        return audioBuffer.size();
    }

    // Only used if this port used for input summing
    bool isAnyConnectedPortSignal = false;

    // todo: this should be getEventBuffer
    std::vector<Event*>& getEvents()
    {
        return events;
    }

    void clearEvents()
    {
        events.clear();
    }

    void addEvent(Event* event)
    {
        events.push_back(event);
    }

    void clear(size_t size)
    {
        audioBuffer.assign(size, 0.0f);
    }

    void setSize(size_t size)
    {
        //audioBuffer.resize(size);
        audioBuffer.assign(size, 0.0f);
    }

    inline bool isSignal() const
    {
        return  (portType & PortType::Signal) != 0;
    }

    AudioNode* getParentNode() const { return node; }

    // Define the equality operator for AudioPort
    bool operator==(const AudioPort& other) const {
        // Compare based on unique identifier or content
        return this == &other;  // For simplicity, compare addresses (can be adjusted based on your design)
    }

    PortType getPortType() const { return portType; }

protected:
    std::vector<float> audioBuffer;
    AudioNode* node;

    std::vector<Event*> events;

    std::string name;

    PortType portType;
};