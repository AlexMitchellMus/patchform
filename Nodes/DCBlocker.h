/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Simple DC blocker using 1st-order high-pass filter
class DCBlock : public AudioNode {
    DEFINE_AND_REGISTER_NODE("DCBlock", "dcblock", true);
    DEFINE_NODE_ALIASES("dcblock");

    float prevInput = 0.0f;
    float prevOutput = 0.0f;
    const float R = 0.995f; // High-pass coefficient

public:
    DCBlock(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto* input = inputPortBuffers[0]->getAudioBuffer();
        auto* output = outputPortBuffers[0]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float x = input[i];
            float y = x - prevInput + R * prevOutput;
            prevInput = x;
            prevOutput = y;
            output[i] = y;
        }
    }
};

REGISTER(DCBlock);
