#pragma once

#include "AudioNodeBase.h"
#include <vector>
#include <cmath>
#include <algorithm>

class Limiter : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Limiter", "limiter", true);
    DEFINE_NODE_ALIASES("limiter");

    std::atomic<float> threshold;
    FloatParameter* thresholdParam;

    std::vector<float> delayBuffer;
    size_t delayWriteIndex = 0;
    size_t lookaheadSamples = 64;

    float envelope = 0.0f;
    float attackCoef = 0.01f;
    float releaseCoef = 0.001f;

public:
    Limiter(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);

        threshold.store(objParams.value("threshold", 1.0f));
        thresholdParam = addParameter<FloatParameter>("threshold", threshold, 0.0f, 1.0f);

        thresholdParam->informNodeOfChange = [this]() {
            threshold.store(thresholdParam->getValue());
        };

        delayBuffer.resize(lookaheadSamples, 0.0f);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* input = inputPortBuffers[0]->getAudioBuffer();
        float* output = outputPortBuffers[0]->getAudioBuffer();
        float t = threshold.load();

        for (unsigned long i = 0; i < frameCount; ++i) {
            float x = input[i];

            // Envelope follower
            float absx = std::fabs(x);
            if (absx > envelope)
                envelope = attackCoef * (absx - envelope) + envelope;
            else
                envelope = releaseCoef * (absx - envelope) + envelope;

            float gain = (envelope > t) ? t / envelope : 1.0f;

            // Lookahead delay
            float delayed = delayBuffer[delayWriteIndex];
            delayBuffer[delayWriteIndex] = x;
            delayWriteIndex = (delayWriteIndex + 1) % lookaheadSamples;

            output[i] = delayed * gain;
        }
    }
};

REGISTER(Limiter);