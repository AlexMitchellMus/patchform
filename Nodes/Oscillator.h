/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <random>
#include <cstdlib>
#include <array>

// Use TABLE_SIZE as the number of intervals; we add one extra sample to close the cycle.
constexpr size_t TABLE_SIZE = 8192;
//constexpr size_t FULL_TABLE_SIZE = TABLE_SIZE + 1;

constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 6.28318530717958647692f;

// A constexpr sine approximation.
constexpr float constexpr_sin(float x) {
    // Normalize x to [-pi, pi]
    while (x > pi)  x -= twoPi;
    while (x < -pi) x += twoPi;

    float x2 = x * x;
    float term1 = x;                           // x
    float term2 = (x * x2) / 6.0f;               // x^3/3!
    float term3 = (x * x2 * x2) / 120.0f;          // x^5/5!
    float term4 = (x * x2 * x2 * x2) / 5040.0f;      // x^7/7!
    float term5 = (x * x2 * x2 * x2 * x2) / 362880.0f; // x^9/9!
    float term6 = (x * x2 * x2 * x2 * x2 * x2) / 39916800.0f; // x^11/11!

    return term1 - term2 + term3 - term4 + term5 - term6;
}

//-----------------------------------------------------------
// Waveform Table Generators (all produce FULL_TABLE_SIZE samples)
//-----------------------------------------------------------

constexpr std::array<float, TABLE_SIZE> generateSineWave() {
    std::array<float, TABLE_SIZE> table = {};
    // When i == TABLE_SIZE, the angle is exactly 2*pi.
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        table[i] = constexpr_sin(2.0f * pi * static_cast<float>(i) / TABLE_SIZE);
    }
    return table;
}

constexpr std::array<float, TABLE_SIZE> generateSawWave() {
    std::array<float, TABLE_SIZE> table = {};
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        float fraction = static_cast<float>(i) / TABLE_SIZE;
        table[i] = 2.0f * fraction - 1.0f;
    }
    return table;
}

constexpr std::array<float, TABLE_SIZE> generateSquareWave() {
    std::array<float, TABLE_SIZE> table = {};
    // Square wave: first half is 1.0, second half is -1.0.
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        table[i] = (i < (TABLE_SIZE / 2)) ? 1.0f : -1.0f;
    }
    return table;
}

constexpr std::array<float, TABLE_SIZE> generateTriangleWave() {
    std::array<float, TABLE_SIZE> table = {};
    for (size_t i = 0; i < TABLE_SIZE; ++i) {
        float fraction = static_cast<float>(i) / TABLE_SIZE;
        if (fraction < 0.5f) {
            table[i] = 4.0f * fraction - 1.0f;
        } else {
            table[i] = 3.0f - 4.0f * fraction;
        }
    }
    return table;
}

// Precomputed tables.
constexpr auto sineWaveTable     = generateSineWave();
constexpr auto sawWaveTable      = generateSawWave();
constexpr auto squareWaveTable   = generateSquareWave();
constexpr auto triangleWaveTable = generateTriangleWave();

//-----------------------------------------------------------
// Oscillator Class Using the Tables
//-----------------------------------------------------------

class Oscillator : public AudioNode {

    DEFINE_AND_REGISTER_NODE("Oscillator", "osc");

protected:
    inline static bool initialized;

    // Phase (in table index units)
    float phase = 0.0f;
    std::string waveform;
    float freq = 0.0f;

    StringParameter* waveformParameter;

    // All tables now have FULL_TABLE_SIZE samples.
    // We use a pointer to float and an effective cycle length.
    const float* waveformTable = nullptr;
    // Effective cycle length remains TABLE_SIZE (the extra sample is for interpolation only)
    size_t tableLength = TABLE_SIZE;

    // Choose waveform table based on the string.
    void updateWaveform()
    {
        switch (hash(waveform)) {
        case hash("saw"):
            waveformTable = sawWaveTable.data();
            break;
        case hash("square"):
            waveformTable = squareWaveTable.data();
            break;
        case hash("tri"):
        case hash("triangle"):
            waveformTable = triangleWaveTable.data();
            break;
        case hash("noise"):
            waveformTable = nullptr;
            break;
        default:
            if (hash(waveform) != hash("sine")) {
                //std::cout << "Error! Unknown waveform: " << waveform << ", using default sine" << std::endl;
                waveform = "sine";
            }
            waveformTable = sineWaveTable.data();
            break;
        }
    }

public:
    Oscillator(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("phase", AudioPort::PortType::Data);
        addInputPort("frequency", AudioPort::PortType::Signal);

        waveform = objParams.value("waveform", "sine");
        freq = objParams.value("freq", 440.0f);

        waveformParameter = addParameter<StringParameter>("Waveform", waveform);

        updateWaveform();
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();
        auto freqEvents = inputPortBuffers[1]->getEvents();
        bool useSignalFreq = inputPortBuffers[1]->isAnyConnectedPortSignal;
        auto freqIn = inputPortBuffers[1]->getAudioBuffer();
        auto output = outputPort.getAudioBuffer();

        waveform = waveformParameter->getValue();
        updateWaveform();

        // We use tableLength (TABLE_SIZE) as the effective period.
        const float tableSizeF = static_cast<float>(tableLength);

        if (waveformTable) {
            unsigned int nextEventIndex = 0;
            unsigned int nextFreqEventIndex = 0;

            for (unsigned long i = 0; i < frameCount; i++) {
                // Handle phase-reset events.
                while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i) {
                    phase = 0.0f;
                    nextEventIndex++;
                }
                if (!useSignalFreq) {
                    while (nextFreqEventIndex < freqEvents.size() && freqEvents[nextFreqEventIndex]->getTimeStamp() == i) {
                        freq = freqEvents[nextFreqEventIndex]->data;
                        nextFreqEventIndex++;
                    }
                }

                int idx = static_cast<int>(phase);
                // We may want to extend all wavetables by 1 sample, instead of using % here
                int nextIdx = (idx + 1) % TABLE_SIZE;
                float fraction = phase - static_cast<float>(idx);
                float value = waveformTable[idx] + fraction * (waveformTable[nextIdx] - waveformTable[idx]);

                // Write the output sample.
                output[i] = 0.5f * value;

                // Determine current frequency.
                float currentFreq = useSignalFreq ? freqIn[i] : freq;

                // Increment phase.
                phase += (tableSizeF * currentFreq) / context->sampleRate;
                // Wrap phase if necessary.
                if (phase >= tableSizeF)
                    phase -= tableSizeF;
                else if (phase < 0.0f)
                    phase += tableSizeF;
            }
        } else {
            // Generate noise on the fly.
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            for (unsigned long i = 0; i < frameCount; i++) {
                output[i] = dist(gen);
            }
        }
    }
};
