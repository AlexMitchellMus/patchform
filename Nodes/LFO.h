/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

class LFOState : public AudioNode::StateBase
{
public:
    LFOState(float val) : frequency(val){};
    float frequency;
    float phase = 0.0f;

    ENABLE_COPY(LFOState);
};

// LFONode that modulates a value (e.g., frequency modulation)
class LFO : public AudioNode {
    DEFINE_AND_REGISTER_NODE("LFO");

public:
    LFO(NodeContext* context, float frequency) : AudioNode(std::make_unique<LFOState>(frequency), context, AudioPort::PortType::Signal) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();

        auto* currentState = dynamic_cast<LFOState*>(activeState);

        for (unsigned int i = 0; i < frameCount; i++) {
            output[i] = 0.5f * std::sin(currentState->phase);
            currentState->phase += 2.0f * M_PI * currentState->frequency / context->sampleRate;
            if (currentState->phase >= 2.0f * M_PI) currentState->phase -= 2.0f * M_PI;
        }
    }
};