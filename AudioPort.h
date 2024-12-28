//
// Created by alexw on 28/12/2024.
//
#include <vector>


#pragma once

class AudioNode;

class AudioPort
{
protected:
    std::vector<float> audioBuffer;
    AudioNode* node;
public:

    AudioPort(AudioNode* parent, size_t size = 0);

    void resize(size_t size) {
        audioBuffer.resize(size);
    }

    float* getAudioBuffer() {
        return audioBuffer.data();
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
