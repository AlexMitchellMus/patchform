/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <string>
#include <iostream>

#include "Graph/Event.h"

class AudioNode;

class AudioPort
{
protected:
    std::vector<float> audioBuffer;
    AudioNode* node;

    std::vector<Event*> events;

    std::string name;
public:

    AudioPort(AudioNode* parent, std::string portName) : node(parent), name(portName)
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

    AudioNode* getParentNode() const {return node; }

    // Define the equality operator for AudioPort
    bool operator==(const AudioPort& other) const {
        // Compare based on unique identifier or content
        return this == &other;  // For simplicity, compare addresses (can be adjusted based on your design)
    }
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

    std::vector<float>& sumPort()
    {
        // Get size from first port's data
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

    std::vector<Event*>& combineEvents()
    {
        summedEvents.clear();
        // 1) Gather all events from each connected port

        for (auto* port : connectedPorts) {
            if (!port) continue;

            auto events = port->getEvents();

            // 2) Insert them into 'combined'
            summedEvents.insert(summedEvents.end(), events.begin(), events.end());
        }

        // 3) Sort combined by timestamp
        std::sort(summedEvents.begin(), summedEvents.end(), [](const Event* a, const Event* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        });

        // Return all events in a single sorted vector
        return summedEvents;
    }
};
