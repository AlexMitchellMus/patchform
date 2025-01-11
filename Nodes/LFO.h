/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

// LFONode that modulates a value (e.g., frequency modulation)
class LFO : public AudioNode {
    DEFINE_AND_REGISTER_NODE("LFO");

    float frequency;
    float phase = 0.0f;

public:
    LFO(NodeContext* context, float frequency) : AudioNode(context, "LFONode", AudioPort::PortType::Signal), frequency(frequency) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();
        for (unsigned int i = 0; i < frameCount; i++) {
            output[i] = 0.5f * std::sin(phase);
            phase += 2.0f * M_PI * frequency / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};