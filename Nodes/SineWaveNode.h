#include "AudioNodes.h"

#pragma once

// SineWaveNode that generates sine wave audio
class SineWaveNode : public AudioNode {
    float phase = 0.0f;

public:
    SineWaveNode(NodeContext* context) : AudioNode(context, "SineWaveNode")
    {
        addInputPort("frequency");
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto input1 = inputPorts[0].sumPort();
        auto output = outputPort.getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = (0.5f * std::sin(phase));
            phase += 2.0f * M_PI * input1[i] / context->sampleRate;
            if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
        }
    }
};
