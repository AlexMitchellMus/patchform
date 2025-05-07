#pragma once

#include "../AudioNodeBase.h"
#include <array>
#include "pffft.h"
#include <cmath>
#include <algorithm>
#include "../SpectralHelpers.h"

class SpecIFFT final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("SpecIFFT", "specIFFT", true);
    DEFINE_NODE_ALIASES("specifft");

public:
    static constexpr size_t FFT_SIZE = 512;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    SpecIFFT(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("real", AudioPort::Spectral);
        addInputPort("imaginary", AudioPort::Spectral);

        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);
    }

    ~SpecIFFT() override {
        if (fftSetup) pffft_destroy_setup(fftSetup);
    }

    void processAudio(const float*, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        float* realIn = inputPortBuffers[0]->getAudioBuffer();
        float* imagIn = inputPortBuffers[1]->getAudioBuffer();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        std::array<float, FFT_SIZE> freq{};
        std::array<float, FFT_SIZE> time{};

        for (size_t i = 0; i < FREQ_BINS; ++i)
        {
            freq[i * 2] = realIn[i];
            freq[i * 2 + 1] = imagIn[i];
        }

        freq[1] = 0.0f;
        freq[FREQ_BINS * 2 - 1] = 0.0f;

        pffft_transform_ordered(fftSetup, freq.data(), time.data(), nullptr, PFFFT_BACKWARD);

        for (size_t i = 0; i < FFT_SIZE; ++i)
        {
            time[i] *= hannWindow[i] / FFT_SIZE;
            size_t idx = (outputWritePos + i) % overlapBuffer.size();
            overlapBuffer[idx] += time[i];
        }

        for (size_t i = 0; i < frameCount; ++i) {
            output[i] = overlapBuffer[outputReadPos];
            overlapBuffer[outputReadPos] = 0.0f;
            outputReadPos = (outputReadPos + 1) % overlapBuffer.size();
            outputWritePos = (outputWritePos + 1) % overlapBuffer.size();
        }
    }

private:
    PFFFT_Setup* fftSetup = nullptr;

    static constexpr auto hannWindow = SpectralHelpers::makeHannWindow<FFT_SIZE>();

    std::array<float, FFT_SIZE * 4> overlapBuffer{};
    size_t outputReadPos = 0;
    size_t outputWritePos = 0;
    size_t hopCounter = 0;
};

REGISTER(SpecIFFT);
