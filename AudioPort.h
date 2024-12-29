//
// Created by alexw on 28/12/2024.
//
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

    bool event = false;

    std::string name;
public:

    AudioPort(AudioNode* parent, std::string& portName) : node(parent), name(portName)
    {
    }

    float* getAudioBuffer() {
        return audioBuffer.data();
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
