#include "AudioNodes.h"

#pragma once

// ValueNode that provides a constant value (e.g., for frequency modulation)
class ValueNode : public AudioNode
{
    float value;

public:
    ValueNode(NodeContext* context, float value) : AudioNode(context, "ValueNode"), value(value) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = value;
        }
    }
};
