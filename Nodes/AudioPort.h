/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "../Graph/Event.h"

class AudioNode;

class AudioPort
{
public:
    enum class PortType
    {
        None =   1 << 0,
        Signal = 1 << 1,
        Data   = 1 << 2
    };

    AudioPort(AudioNode* parent, std::string portName, PortType type) : node(parent), name(portName), portType(type)
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
        audioBuffer.resize(size, 0.0f);
        std::fill(audioBuffer.begin(), audioBuffer.end(), 0.0f);
    }

    void setSize(size_t size)
    {
        audioBuffer.assign(size, 0.0f);
    }

    bool isSignal()
    {
        return portType == PortType::Signal;
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