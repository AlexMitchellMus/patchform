/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// LFONode that modulates a value (e.g., frequency modulation)
class LFO : public AudioNode {
    DEFINE_AND_REGISTER_NODE("LFO", "lfo");

    FloatParameter* freqParam;

    float frequency;
    float phase = 0.0f;

public:
    LFO(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        auto const freq = objParams.value("rate", 1.0f);

        freqParam = addParameter<FloatParameter>("Hz", freq, 0.00001f, std::numeric_limits<float>::max());

        frequency = freq / context->sampleRate;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto output = outputPort.getAudioBuffer();

        frequency = freqParam->getValue() / context->sampleRate;

        for (unsigned int i = 0; i < frameCount; i++) {
            output[i] = 0.5f * std::sin(phase);
            phase += 2.0f * M_PI * frequency;;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};