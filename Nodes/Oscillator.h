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

#include "unordered_dense.h"

using WaveTables = ankerl::unordered_dense::map<std::string, std::vector<float>>;

// SineWaveNode that generates sine wave audio

class Oscillator : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Oscillator", "osc");

protected:
    static constexpr int TABLE_SIZE = 8192;
    static WaveTables waveformTables;
    static bool initialized;

    // TODO: move to state management
    float phase = 0.0f;
    std::string waveform;
    float freq = 0.0f;
    bool useTable = true;

    static void initializeWaveformTable(std::string& waveform, bool& useTable) {
        if (waveformTables.find(waveform) != waveformTables.end()) {
            return; // Table already initialized
        }

        std::vector<float> table(TABLE_SIZE);

        auto waveformHash = hash(waveform);

        switch (waveformHash) {
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
            case hash("tri"):
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
                if (waveformHash != hash("sine")) {
                    std::cout << "Error! Unknown waveform: " << waveform << ", using default sine" << std::endl;
                    waveform = "sine";
                }
                for (int i = 0; i < TABLE_SIZE; i++)
                {
                    table[i] = std::sin(2.0f * M_PI * i / TABLE_SIZE);
                }
                break;
            }
        }

        waveformTables[waveform] = std::move(table);
    }

public:
    Oscillator(NodeContext* context, const json& nodeData)
        : AudioNode(context, AudioPort::PortType::Signal, nodeData)
    {
        addInputPort("phase", AudioPort::PortType::Data);
        addInputPort("frequency", AudioPort::PortType::Signal);

        waveform = nodeData.value("waveform", "sine");
        freq = nodeData.value("freq", 440.0f);

        initializeWaveformTable(this->waveform, this->useTable);
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto events = inputPortBuffers[0]->getEvents();
        auto freqEvents = inputPortBuffers[1]->getEvents();
        // If a signal cable is connected, don't process events, and use the signal instead
        bool useSignalFreq = inputPortBuffers[1]->isAnyConnectedPortSignal;
        auto freqIn = inputPortBuffers[1]->getAudioBuffer();     // Frequency input
        auto output = outputPort.getAudioBuffer(); // Node's output buffer

        unsigned int nextEventIndex = 0;
        unsigned int nextFreqEventIndex = 0;

        if (useTable) {
            const auto& table = waveformTables[waveform];
            const float tableSizeF = static_cast<float>(TABLE_SIZE);

            for (unsigned long i = 0; i < frameCount; i++) {
                while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i) {
                    phase = 0.0f;
                    nextEventIndex++;
                }
                if (!useSignalFreq)
                {
                    while (nextFreqEventIndex < freqEvents.size() && freqEvents[nextFreqEventIndex]->getTimeStamp() == i) {
                        freq = freqEvents[nextFreqEventIndex]->data;
                        nextFreqEventIndex++;
                    }
                }

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
                phase += (tableSizeF * (useSignalFreq ? freqIn[i] : freq)) / context->sampleRate;
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
WaveTables Oscillator::waveformTables;
bool Oscillator::initialized = false;
