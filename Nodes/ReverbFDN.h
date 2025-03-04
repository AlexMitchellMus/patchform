/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <vector>
#include <cmath>

// Huge FDN Reverb Node
class ReverbFDN : public AudioNode {
    DEFINE_AND_REGISTER_NODE("ReverbFDN", "fdn");

    static constexpr int numDelays = 16; // Further increase delay lines for an even bigger sound
    std::array<std::vector<float>, numDelays> delayBuffers;
    std::array<unsigned int, numDelays> delayIndices{};
    std::array<unsigned int, numDelays> delayLengths{};
    std::array<float, numDelays> feedbackGains;

    float mix = 0.8f; // Higher wet signal for a massive effect

public:
    ReverbFDN(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Input", AudioPort::PortType::Signal);

        // Even longer prime-number delay lengths for maximum spaciousness
        const std::array<unsigned int, numDelays> baseLengths = {1493, 2137, 2719, 3323, 3881, 4457, 5023, 5651, 6311, 6967, 7621, 8293, 8963, 9649, 10331, 11003};
        for (int i = 0; i < numDelays; i++) {
            delayLengths[i] = baseLengths[i];
            delayBuffers[i].resize(delayLengths[i], 0.0f);
            feedbackGains[i] = 0.9f; // Higher feedback for longer sustain
        }
    }

    void processAudio(float* buffer, unsigned long frameCount) override
    {
        const auto* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        auto* outputBuffer = outputPort.getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            float inputSample = inputBuffer[i];
            float reverbSample = 0.0f;
            std::array<float, numDelays> tempOutputs{};

            for (int j = 0; j < numDelays; j++) {
                float delayedSample = delayBuffers[j][delayIndices[j]];
                tempOutputs[j] = delayedSample;
                delayBuffers[j][delayIndices[j]] = inputSample + delayedSample * feedbackGains[j];
                delayIndices[j] = (delayIndices[j] + 1) % delayLengths[j];
            }

            // More complex mixing matrix for ultra-lush and cavernous sound
            reverbSample = (tempOutputs[0] + tempOutputs[1] - tempOutputs[2] + tempOutputs[3] - tempOutputs[4] + tempOutputs[5] - tempOutputs[6] + tempOutputs[7] +
                            tempOutputs[8] - tempOutputs[9] + tempOutputs[10] - tempOutputs[11] + tempOutputs[12] - tempOutputs[13] + tempOutputs[14] - tempOutputs[15]) * 0.25f;

            // Apply wet/dry mix
            outputBuffer[i] = (1.0f - mix) * inputSample + mix * reverbSample;
        }
    }
};
