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

// Improved FDN Reverb Node
class ReverbFDN : public AudioNode {
    DEFINE_AND_REGISTER_NODE("ReverbFDN", "fdn", true);

    static constexpr int numDelays = 8;
    std::array<std::vector<float>, numDelays> delayBuffers;
    std::array<unsigned int, numDelays> delayIndices{};
    std::array<unsigned int, numDelays> delayLengths{};
    std::array<float, numDelays> feedbackGains;
    std::array<float, numDelays> prevOut{};

    float mix = 0.90f; // Less wet signal for better clarity

public:
    ReverbFDN(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Input", AudioPort::PortType::Signal);

        // Adjusted prime-number delay lengths for better diffusion
        const std::array<unsigned int, numDelays> baseLengths = {1493, 2137, 2719, 3323, 4127, 4871, 5519, 6197};
        for (int i = 0; i < numDelays; i++) {
            delayLengths[i] = baseLengths[i];
            delayBuffers[i].resize(delayLengths[i], 0.0f);
            feedbackGains[i] = 0.6f + 0.1f * (i % 2); // Slight variation in feedback
        }
    }

    void processAudio(float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        auto* outputBuffer = outputPortBuffers[0]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            float inputSample = inputBuffer[i];
            float reverbSample = 0.0f;
            std::array<float, numDelays> tempOutputs{};

            for (int j = 0; j < numDelays; j++) {
                float delayedSample = delayBuffers[j][delayIndices[j]];

                // Low-pass filter to reduce high-frequency buildup
                float filteredSample = 0.85f * prevOut[j] + 0.15f * delayedSample;
                prevOut[j] = filteredSample;

                tempOutputs[j] = filteredSample;
                delayBuffers[j][delayIndices[j]] = inputSample + filteredSample * feedbackGains[j];
                delayIndices[j] = (delayIndices[j] + 1) % delayLengths[j];
            }

            // Improved mixing matrix for better diffusion
            reverbSample = (tempOutputs[0] + tempOutputs[1] + tempOutputs[2] - tempOutputs[3] +
                            tempOutputs[4] - tempOutputs[5] - tempOutputs[6] + tempOutputs[7]) * 0.35f;

            // Apply wet/dry mix
            outputBuffer[i] = (1.0f - mix) * inputSample + mix * reverbSample;
        }
    }
};
