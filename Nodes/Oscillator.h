#include "AudioNodes.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <random>

#pragma once

// SineWaveNode that generates sine wave audio

class Oscillator : public AudioNode {
protected:
    static constexpr int TABLE_SIZE = 4096;
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

            for (unsigned long i = 0; i < frameCount; i++) {
                // Convert current phase to an integer index
                int idx = static_cast<int>(phase);

                // Clamp index in case of any floating error
                if (idx < 0) idx = 0;
                if (idx >= TABLE_SIZE) idx = TABLE_SIZE - 1;

                // Write the waveform value
                output[i] = 0.5f * table[idx];

                // Increment phase based on frequency and sample rate
                float phaseInc = (TABLE_SIZE * freqIn[i]) / context->sampleRate;
                phase += phaseInc;

                // Wrap phase within [0, TABLE_SIZE)
                while (phase >= TABLE_SIZE) phase -= TABLE_SIZE;
                while (phase < 0.0f) phase += TABLE_SIZE;
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