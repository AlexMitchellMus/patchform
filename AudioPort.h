/*
// Copyright (c) 2024 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <vector>
#include <string>
#include <iostream>

#pragma once

class AudioNode;

class AudioPort
{
protected:
    std::vector<float> audioBuffer;
    AudioNode* node;

    struct Event
    {
        uint64_t timeStamp = 0;

        Event(uint64_t timeStamp) : timeStamp(timeStamp) {}
    };

    std::vector<Event> events;

    std::string name;
public:

    AudioPort(AudioNode* parent, std::string& portName) : node(parent), name(portName)
    {
    }

    float* getAudioBuffer() {
        return audioBuffer.data();
    }

    std::vector<Event>* getEvents()
    {
        return &events;
    }

    void addEvent(uint64_t timeStamp)
    {
        events.emplace_back(Event(timeStamp));
    }

    void clear(size_t size)
    {
        audioBuffer.resize(size, 0.0f);
        std::fill(audioBuffer.begin(), audioBuffer.end(), 0.0f);
    }

    size_t size() const {
        return audioBuffer.size();
    }

    AudioNode* getParentNode() const {return node; }

    // Define the equality operator for AudioPort
    bool operator==(const AudioPort& other) const {
        // Compare based on unique identifier or content
        return this == &other;  // For simplicity, compare addresses (can be adjusted based on your design)
    }
};
