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

    FreqBins(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("audioIn", AudioPort::Signal);
        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);
    }

    ~FreqBins() override {
        pffft_destroy_setup(fftSetup);
    }

    void processAudio(const float*, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const float* in = inputPortBuffers[0]->getAudioBuffer();
        if (!in) return;

        for (unsigned long i = 0; i < frameCount; ++i) {
            dspBuffer[dspBufferIndex++] = in[i];
            sampleCounter++;

            if (dspBufferIndex >= FFT_SIZE) {
                processFFT(sampleCounter - FFT_SIZE);
                dspBufferIndex = 0;
            }
        }
    }

    void processFFT(size_t timestamp)
    {
        std::array<float, FFT_SIZE> time{};
        std::array<float, FFT_SIZE> freq{};

        std::fill(freq.begin(), freq.end(), 0.0f);

        for (size_t i = 0; i < FFT_SIZE; ++i)
            time[i] = dspBuffer[i] * 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));

        pffft_transform_ordered(fftSetup, time.data(), freq.data(), nullptr, PFFFT_FORWARD);

        auto* ev = context->eventPool.getFreeEvent();
        ev->setTimeStamp(timestamp);

        DataAtom* head = nullptr;
        DataAtom* tail = nullptr;

        for (size_t i = 0; i < FREQ_BINS; ++i) {
            float re = freq[i * 2];
            float im = freq[i * 2 + 1];

            DataAtom* outer = context->eventPool.allocateDataAtom();
            outer->type = DataAtom::DataType::List;

            DataAtom* realAtom = context->eventPool.allocateDataAtom();
            DataAtom* imagAtom = context->eventPool.allocateDataAtom();

            realAtom->type = DataAtom::DataType::Float;
            realAtom->data.atom = re;
            realAtom->next = imagAtom;

            imagAtom->type = DataAtom::DataType::Float;
            imagAtom->data.atom = im;
            imagAtom->next = nullptr;

            outer->data.list = realAtom;
            outer->next = nullptr;

            if (!head) {
                head = tail = outer;
            } else {
                tail->next = outer;
                tail = outer;
            }
        }

        ev->data = head;
        ev->numAtoms = FREQ_BINS;
        ev->setTagHashcode(hash("complexbins"));
        outputPortBuffers[0]->addEvent(ev);
    }

private:
    PFFFT_Setup* fftSetup = nullptr;
    std::array<float, FFT_SIZE> dspBuffer{};
    size_t dspBufferIndex = 0;
    size_t sampleCounter = 0;
};
