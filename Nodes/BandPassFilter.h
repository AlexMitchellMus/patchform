// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include "AudioNodeBase.h"
#include <algorithm>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
    #define M_PI_2 (M_PI / 2.0)
#endif


// Band-Pass Filter node (Pure Data-style)
class BandPassFilter : public AudioNode {
    DEFINE_AND_REGISTER_NODE("BandPass", "bpf", true);
    DEFINE_NODE_ALIASES("bpf", "bandpassfilter");

    // Internal filter state
    float x1 = 0.0f, x2 = 0.0f; // Previous inputs
    float y1 = 0.0f, y2 = 0.0f; // Previous outputs

    // Filter coefficients
    float coef1 = 0.0f, coef2 = 0.0f, gain = 1.0f;

    // Parameters
    float freq = 440.0f;
    float q = 1.0f;

public:
    BandPassFilter(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Audio", AudioPort::PortType::Signal);
        addInputPort("Freq", AudioPort::PortType::Data);
        addInputPort("Q", AudioPort::PortType::Data);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto* audioInput = inputPortBuffers[0]->getAudioBuffer();
        const auto freqEvents  = inputPortBuffers[1]->getEvents();
        const auto qEvents     = inputPortBuffers[2]->getEvents();

        auto* output = outputPortBuffers[0]->getAudioBuffer();

        unsigned int nextFreqEventIndex = 0;
        unsigned int nextQEventIndex = 0;

        for (unsigned long i = 0; i < frameCount; i++)
        {
            // Process frequency events at correct timestamp
            while (nextFreqEventIndex < freqEvents.size() && freqEvents[nextFreqEventIndex]->getTimeStamp() == i)
            {
                freq = std::clamp(freqEvents[nextFreqEventIndex]->getAtomValue(0), 10.0f, context->sampleRate * 0.45f);
                nextFreqEventIndex++;
                updateCoefficients();
            }

            // Process Q events at correct timestamp
            while (nextQEventIndex < qEvents.size() && qEvents[nextQEventIndex]->getTimeStamp() == i)
            {
                q = std::clamp(qEvents[nextQEventIndex]->getAtomValue(0), 0.1f, 300.0f);
                nextQEventIndex++;
                updateCoefficients();
            }

            // **PD-Style Direct Form I Band-Pass Filter**
            float inputSample = audioInput[i];
            float y = inputSample + coef1 * y1 + coef2 * y2;

            // Apply PD gain scaling
            output[i] = gain * y;

            // Shift state
            y2 = y1;
            y1 = y;
        }
    }

private:
    void updateCoefficients() {
        float sr = context->sampleRate;
        float omega = (freq * (2.0f * M_PI)) / sr;

        // Approximate cosine for stability
        float cosOmega = pd_qcos(omega);

        float oneminusr = (q < 0.001f) ? 1.0f : omega / q;
        oneminusr = std::min(oneminusr, 1.0f); // Limit for stability

        float r = 1.0f - oneminusr;

        coef1 = 2.0f * cosOmega * r;
        coef2 = -r * r;
        gain = 2.0f * oneminusr * (oneminusr + r * omega);
    }

    // **Pure Data's cosine approximation function**
    static float pd_qcos(float f) {
        if (f >= -M_PI_2 && f <= M_PI_2) {
            float g = f * f;
            return (((g * g * g * (-1.0f / 720.0f)) + (g * g * (1.0f / 24.0f))) - g * 0.5f) + 1.0f;
        }
        return 0.0f;
    }
};

REGISTER(BandPassFilter);
