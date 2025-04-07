#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>         // Debugging output
#include "pffft.h"          // PFFFT for fast FFT
#include <algorithm>
#include <cmath>            // log2f()
#include "SpectralHelpers.h"

class SpecFFT final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("SpecFFT", "specFFT", true);

public:
    static constexpr size_t FFT_SIZE = 512;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    SpecFFT(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Spectral, objParams)
    {
        addInputPort("audioIn", AudioPort::Signal);
        addOutputPort("imaginary", AudioPort::Spectral);

        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);
    }

    ~SpecFFT()
    {
        if (fftSetup) pffft_destroy_setup(fftSetup);
    }

    void processAudio(const float*, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        float* input = inputPortBuffers[0]->getAudioBuffer();
        float* realOut = outputPortBuffers[0]->getAudioBuffer();
        float* imagOut = outputPortBuffers[1]->getAudioBuffer();

        for (size_t i = 0; i < frameCount; ++i)
        {
            dspBuffer[dspBufferIndex++] = input[i];
        }

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

        std::memmove(dspBuffer.data(), dspBuffer.data() + frameCount, (FFT_SIZE - frameCount) * sizeof(float));
        dspBufferIndex = FFT_SIZE - frameCount;
    }


private:
    PFFFT_Setup* fftSetup = nullptr;
    std::array<float, FFT_SIZE> dspBuffer{};

    static constexpr auto hannWindow = SpectralHelpers::makeHannWindow<FFT_SIZE>();

    size_t dspBufferIndex = 0;
};