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

    void processAudio(const float*, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const float* in = inputPortBuffers[0]->getAudioBuffer();
        if (!in) return;

        for (unsigned long i = 0; i < frameCount; ++i) {
            if (dspBufferIndex < FFT_SIZE) {  // Add bounds check
                dspBuffer[dspBufferIndex++] = in[i];
            }
            sampleCounter++;

            // If we've filled the buffer, process FFT
            if (dspBufferIndex >= FFT_SIZE) {
                processFFT(sampleCounter - FFT_SIZE);
                dspBufferIndex = 0;

                // To implement proper hop size (overlapping FFTs):
                /*
                // Shift buffer by HOP_SIZE
                for (size_t j = 0; j < FFT_SIZE - HOP_SIZE; ++j) {
                    dspBuffer[j] = dspBuffer[j + HOP_SIZE];
                }
                dspBufferIndex = FFT_SIZE - HOP_SIZE;
                */
            }
        }
    }

    void processFFT(size_t timestamp)
    {
        if (!fftSetup) return;  // Safety check

        std::array<float, FFT_SIZE> time{};
        std::array<float, FFT_SIZE> freq{};

        // Apply window function to input
        for (size_t i = 0; i < FFT_SIZE; ++i) {
            time[i] = dspBuffer[i] * hannWindow[i];
        }

        pffft_transform_ordered(fftSetup, time.data(), freq.data(), nullptr, PFFFT_FORWARD);

        auto* ev = context->eventPool.getFreeEvent();
        if (!ev) return;  // Handle allocation failure

        ev->setTimeStamp(timestamp);
        DataAtom* head = nullptr;
        DataAtom* tail = nullptr;

        for (size_t i = 0; i < FREQ_BINS; ++i) {
            float re = freq[i * 2];
            float im = freq[i * 2 + 1];

            DataAtom* outer = context->eventPool.allocateDataAtom();
            if (!outer) {
                return;
            }

            outer->type = DataAtom::DataType::List;

            DataAtom* realAtom = context->eventPool.allocateDataAtom();
            if (!realAtom) {
                return;
            }

            DataAtom* imagAtom = context->eventPool.allocateDataAtom();
            if (!imagAtom) {
                return;
            }

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
    std::array<float, FFT_SIZE> hannWindow{};  // Pre-calculated window
    size_t dspBufferIndex = 0;
    size_t sampleCounter = 0;
};