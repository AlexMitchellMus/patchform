#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>
#include "pffft.h"
#include <cmath>
#include <algorithm>

class FreqResynth final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("FreqResynth", "freqresynth", true);

public:
    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;
    static constexpr size_t HOP_SIZE = FFT_SIZE / 64;

    FreqResynth(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("bins", AudioPort::PortType::Data);
        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);
    }

    ~FreqResynth() override {
        pffft_destroy_setup(fftSetup);
    }

    void processAudio(const float* in, float* out, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const auto& events = inputPortBuffers[0]->getEvents();
        auto* output = outputPortBuffers[0]->getAudioBuffer();

        bool hasNewBins = false;

        for (const auto* ev : events) {
            if (ev->getTagHash() != hash("complexbins") || ev->numAtoms < FREQ_BINS)
                continue;

            for (size_t i = 0; i < FREQ_BINS; ++i) {
                const DataAtom* outer = ev->getAtom(i);
                if (!outer || outer->type != DataAtom::DataType::List || !outer->data.list)
                    continue;

                const DataAtom* realAtom = outer->data.list;
                const DataAtom* imagAtom = realAtom ? realAtom->next : nullptr;
                if (!imagAtom) continue;

                binReal[i] = realAtom->data.atom;
                binImag[i] = imagAtom->data.atom;
            }

            hasNewBins = true;
            break; // Use first valid event only
        }

        if (++hopCounter >= HOP_SIZE || hasNewBins) {
            hopCounter = 0;
            doIFFT();
        }

        for (size_t i = 0; i < frameCount; ++i) {
            output[i] = overlapBuffer[outputReadPos];
            overlapBuffer[outputReadPos] = 0.0f;
            outputReadPos = (outputReadPos + 1) % overlapBuffer.size();
            outputWritePos = (outputWritePos + 1) % overlapBuffer.size();
        }
    }

private:
    void doIFFT() {
        std::array<float, FFT_SIZE> freq{};
        std::array<float, FFT_SIZE> time{};

        for (size_t i = 0; i < FREQ_BINS; ++i) {
            freq[i * 2]     = binReal[i];
            freq[i * 2 + 1] = binImag[i];
        }

        freq[1] = 0.0f;
        freq[FREQ_BINS * 2 - 1] = 0.0f;

        pffft_transform_ordered(fftSetup, freq.data(), time.data(), nullptr, PFFFT_BACKWARD);

        for (size_t i = 0; i < FFT_SIZE; ++i) {
            float hann = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
            time[i] *= hann / (FFT_SIZE * 0.5f);
            size_t idx = (outputWritePos + i) % overlapBuffer.size();
            overlapBuffer[idx] += time[i];
        }
    }

    PFFFT_Setup* fftSetup = nullptr;
    std::array<float, FFT_SIZE * 4> overlapBuffer{};
    size_t outputReadPos = 0;
    size_t outputWritePos = 0;
    size_t hopCounter = 0;
    std::array<float, FREQ_BINS> binReal { 0.0f };
    std::array<float, FREQ_BINS> binImag { 0.0f };
};
