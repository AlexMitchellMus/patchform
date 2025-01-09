/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <random>

// SineWaveNode that generates sine wave audio

class Oscillator : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Oscillator");

protected:
    static constexpr int TABLE_SIZE = 8192;
    static std::unordered_map<std::string, std::vector<float>> waveformTables;
    static bool initialized;

    float phase = 0.0f;
    std::string waveform;

    bool useTable = true;

    static void initializeWaveformTable(const std::string& waveform, bool& useTable) {
        if (waveformTables.find(waveform) != waveformTables.end()) {
            return; // Table already initialized
        }

        std::vector<float> table(TABLE_SIZE);
        switch (hash(waveform)) {
            case hash("sine"): {
                for (int i = 0; i < TABLE_SIZE; i++) {
                    table[i] = std::sin(2.0f * M_PI * i / TABLE_SIZE);
                }
                break;
            }
            case hash("saw"): {
                for (int i = 0; i < TABLE_SIZE; i++) {
                    float fraction = static_cast<float>(i) / TABLE_SIZE;
                    table[i] = 2.0f * fraction - 1.0f;
                }
                break;
            }
            case hash("square"): {
                for (int i = 0; i < TABLE_SIZE; i++) {
                    table[i] = (i < TABLE_SIZE / 2) ? 1.0f : -1.0f;
                }
                break;
            }
            case hash("triangle"): {
                for (int i = 0; i < TABLE_SIZE; i++) {
                    float fraction = static_cast<float>(i) / TABLE_SIZE;
                    if (fraction < 0.5f) {
                        table[i] = 4.0f * fraction - 1.0f;
                    } else {
                        table[i] = 3.0f - 4.0f * fraction;
                    }
                }
                break;
            }
            case hash("noise"): {
                    useTable = false;
                return;
            }
            default: {
                throw std::runtime_error("Unsupported waveform: " + waveform);
            }
        }

        waveformTables[waveform] = std::move(table);
    }

public:
    Oscillator(NodeContext* context, std::string waveform) : AudioNode(context, "Oscillator"), waveform(std::move(waveform)) {
        addInputPort("frequency");
        initializeWaveformTable(this->waveform, this->useTable);
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto freqIn = inputPorts[0].sumPort();     // Frequency input
        auto output = outputPort.getAudioBuffer(); // Node's output buffer

        if (useTable) {
            const auto& table = waveformTables[waveform];
            const float tableSizeF = static_cast<float>(TABLE_SIZE);

            for (unsigned long i = 0; i < frameCount; i++) {
                // Convert current phase to an integer index and calculate next index
                int idx = static_cast<int>(phase);
                int nextIdx = idx + 1;

                // Wrap the next index within TABLE_SIZE without conditionals
                if (nextIdx >= TABLE_SIZE) nextIdx -= TABLE_SIZE;

                // Calculate fractional part for interpolation
                float fraction = phase - static_cast<float>(idx);

                // Linearly interpolate between current and next table values
                float value = table[idx] + fraction * (table[nextIdx] - table[idx]);

                // Write the waveform value
                output[i] = 0.5f * value;

                // Increment and wrap phase efficiently
                phase += (tableSizeF * freqIn[i]) / context->sampleRate;
                if (phase >= tableSizeF) phase -= tableSizeF;
                else if (phase < 0.0f) phase += tableSizeF;
            }
        } else {
            // Generate noise on-the-fly
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

            for (unsigned long i = 0; i < frameCount; i++) {
                output[i] = dist(gen); // Random value in range [-1.0, 1.0]
            }
        }
    }
};

// Static member definitions
std::unordered_map<std::string, std::vector<float>> Oscillator::waveformTables;
bool Oscillator::initialized = false;
;