/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <vector>
#include <string>
#include <iostream>

#pragma once

class Event
{
    uint64_t timeStamp = 0;
public:
    Event(){};

    Event(uint64_t timeStamp) : timeStamp(timeStamp) {}

    uint64_t getTimeStamp() const
    {
        return timeStamp;
    }

    Event& setTimeStamp(const uint64_t timestamp)
    {
        timeStamp = timestamp;
        return *this;
    }
};

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
    }

    float* getAudioBuffer() {
        return audioBuffer.data();
    }

    size_t getAudioBufferSize()
    {
        return audioBuffer.size();
    }

    std::vector<Event*> getEvents()
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
    std::vector<Event> summedEvents;


    explicit AudioInputPort(const std::string& portName)
        : name(portName)
    {
    }

    std::vector<float>& sumPort()
    {
        // Get size from first port's data
        std::size_t dataSize = connectedPorts[0]->getAudioBufferSize();

        if (dataSize != summed.size())
            summed.resize(dataSize, 0.0f);

        std::fill(summed.begin(), summed.end(), 0.0f);

        // Sum each port’s data
        for (auto* port : connectedPorts) {
            const auto& data = port->getAudioBuffer();

            // (Optional) confirm data.size() == dataSize. If not, handle mismatch.
            for (std::size_t i = 0; i < dataSize; i++) {
                summed[i] += data[i];
            }
        }

        return summed;
    }

    std::vector<Event*> combineEvents()
    {
        // 1) Gather events from each connected port
        std::vector<Event*> combined;

        // 1) Gather all events from each connected port
        for (auto* port : connectedPorts) {
            if (!port) continue;

            // Assume each port can supply its events (or we have a function to retrieve them)
            auto events = port->getEvents();
            // or something like: auto events = port->takeEvents();

            // 2) Insert them into 'combined'
            combined.insert(combined.end(), events.begin(), events.end());
        }

        // 3) Sort combined by timestamp
        std::sort(combined.begin(), combined.end(), [](const Event* a, const Event* b) {
            return a->getTimeStamp() < b->getTimeStamp();
        });

        // Return all events in a single sorted vector
        return combined;
    }
};
