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
        if (audioBuffer.size() != size)
            audioBuffer.resize(size, 0.0f);
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

struct AudioInputPort {
    std::string name;
    std::vector<AudioPort*> connectedPorts;
    std::vector<float> summed;
    std::vector<Event*> summedEvents;


    explicit AudioInputPort(const std::string& portName)
        : name(portName)
    {
        summedEvents.reserve(1024);
    }

    bool isAnyConnectedPortsSignal()
    {
        return std::any_of(connectedPorts.begin(), connectedPorts.end(), [](auto const& port) {
            return port->isSignal();
        });
    }

    std::vector<float>& sumAudio()
    {
        if (connectedPorts.size() == 0)
            return summed;

        if (!isAnyConnectedPortsSignal())
            return summed;

        std::size_t dataSize = connectedPorts[0]->getAudioBufferSize();

        if (dataSize != summed.size())
            summed.resize(dataSize, 0.0f);

        std::fill(summed.begin(), summed.end(), 0.0f);

        // Sum each port’s audio
        for (auto* port : connectedPorts) {
            const auto& audio = port->getAudioBuffer();

            // (Optional) confirm data.size() == dataSize. If not, handle mismatch.
            for (std::size_t i = 0; i < dataSize; i++) {
                summed[i] += audio[i];
            }
        }

        return summed;
    }

    std::vector<Event*>& sumEvents()
    {
        summedEvents.clear();

        for (auto* port : connectedPorts) {
            if (!port) continue;
            auto& events = port->getEvents();
            summedEvents.insert(summedEvents.end(), events.begin(), events.end());
        }

        // Sort combined by timestamp
        std::sort(summedEvents.begin(), summedEvents.end(), [](const Event* a, const Event* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        });

        return summedEvents;
    }

};
