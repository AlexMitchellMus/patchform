#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>         // Debugging output
#include "pffft.h"          // PFFFT for fast FFT
#include <algorithm>
#include <cmath>            // log2f()

class FreqBins final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("FreqBins", "freqbins", true);

public:
    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t HOP_SIZE = FFT_SIZE / 4;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    FreqBins(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Spectral, objParams)
    {
        addInputPort("audioIn", AudioPort::Signal);

        addOutputPort("imaginary", AudioPort::Spectral);

        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);

        // Precompute window function
        for (size_t i = 0; i < FFT_SIZE; ++i) {
            hannWindow[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
        }
    }

    void cleanupAudio() override
    {
        if (fftSetup) {
            pffft_destroy_setup(fftSetup);
            fftSetup = nullptr;
        }
    }

    void processAudio(const float* in, float* out, const unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        float* input = inputPortBuffers[0]->getAudioBuffer();
        float* realOut = outputPortBuffers[0]->getAudioBuffer();
        float* imagOut = outputPortBuffers[1]->getAudioBuffer();

        for (size_t i = 0; i < frameCount; ++i)
        {
            if (dspBufferIndex < FFT_SIZE)
                dspBuffer[dspBufferIndex++] = input[i];

            if (dspBufferIndex == FFT_SIZE)
            {
                std::array<float, FFT_SIZE> time{};
                std::array<float, FFT_SIZE> freq{};

                for (size_t j = 0; j < FFT_SIZE; ++j)
                    time[j] = dspBuffer[j] * hannWindow[j];

                pffft_transform_ordered(fftSetup, time.data(), freq.data(), nullptr, PFFFT_FORWARD);

                for (size_t j = 0; j < FREQ_BINS; ++j)
                {
                    realOut[j] = freq[2 * j];
                    imagOut[j] = freq[2 * j + 1];
                }

                dspBufferIndex = 0;
            }
        }
    }

private:
    PFFFT_Setup* fftSetup = nullptr;
    std::array<float, FFT_SIZE> dspBuffer{};
    std::array<float, FFT_SIZE> hannWindow{};  // Pre-calculated window
    size_t dspBufferIndex = 0;
    size_t sampleCounter = 0;
};