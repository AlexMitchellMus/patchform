#pragma once

#include "pffft.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include "AudioNodeBase.h"

class TableXSpectral : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXSpectral", "tableXspectral", true);

public:
    TableXSpectral(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Samples, objParams)
    {
        addInputPort("a", AudioPort::Samples);
        addInputPort("b", AudioPort::Samples);
        addInputPort("x", AudioPort::Signal); // blend 0–1

        setup = pffft_new_setup(2048, PFFFT_REAL);
        fftA = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        fftB = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        fftOut = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        tempTime = (float*)pffft_aligned_malloc(2048 * sizeof(float));

        outputSamples.assign(2048, 0.0f);
        smoothedOutput.assign(2048, 0.0f);
    }

    ~TableXSpectral() override {
        pffft_aligned_free(fftA);
        pffft_aligned_free(fftB);
        pffft_aligned_free(fftOut);
        pffft_aligned_free(tempTime);
        pffft_destroy_setup(setup);
    }

    struct Peak {
        float bin;
        float mag;
        float phase;
    };

    static void extractPeaks(const float* fft, Peak* peaks, int& count, int maxPeaks) {
        count = 0;
        for (int i = 1; i < 1023; ++i) {
            float re = fft[2 * i];
            float im = fft[2 * i + 1];
            float mag = std::sqrt(re * re + im * im);
            if (mag > 1e-6f && count < maxPeaks) {
                float phase = std::atan2(im, re);
                peaks[count++] = { (float)i, mag, phase };
            }
        }
        std::sort(peaks, peaks + count, [](const Peak& a, const Peak& b) {
            return a.mag > b.mag;
        });
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        const auto inputA = inputPortBuffers[0]->sampleBuffer;
        const auto inputB = inputPortBuffers[1]->sampleBuffer;
        if (inputA.size != 2048 || inputB.size != 2048) {
            std::fill(outputSamples.begin(), outputSamples.end(), 0.0f);
            outputPortBuffers[0]->sampleBuffer.reset();
            return;
        }

        const float* a = inputA.samples;
        const float* b = inputB.samples;
        const float* x = inputPortBuffers[2]->getAudioBuffer();

        float blend = std::clamp(x[context->frameCount - 1], 0.0f, 1.0f);
        float blendCurve = blend * blend;                 // subtle nonlinearity for mag
        float phaseBlend = std::sqrt(blend);              // different curve for phase
        float binBlend = 0.5f * (1.0f - SpectralHelpers::constexprCos(blend * M_PI)); // smoother bin blend

        pffft_transform_ordered(setup, a, fftA, nullptr, PFFFT_FORWARD);
        pffft_transform_ordered(setup, b, fftB, nullptr, PFFFT_FORWARD);

        Peak peaksA[128];
        Peak peaksB[128];
        int countA = 0, countB = 0;
        extractPeaks(fftA, peaksA, countA, 128);
        extractPeaks(fftB, peaksB, countB, 128);
        size_t count = std::min(countA, countB);

        std::fill(fftOut, fftOut + 2048, 0.0f);

        for (size_t i = 0; i < count; ++i) {
            float bin = (1.0f - binBlend) * peaksA[i].bin + binBlend * peaksB[i].bin;
            float mag = std::sqrt((1.0f - blendCurve) * peaksA[i].mag * peaksA[i].mag +
                                  blendCurve * peaksB[i].mag * peaksB[i].mag);
            float phase = (1.0f - phaseBlend) * peaksA[i].phase + phaseBlend * peaksB[i].phase;

            int binLo = (int)std::floor(bin);
            float frac = bin - binLo;
            float re = mag * std::cos(phase);
            float im = mag * std::sin(phase);

            if (binLo >= 1 && binLo < 2047) {
                fftOut[2 * binLo    ] += (1.0f - frac) * re;
                fftOut[2 * binLo + 1] += (1.0f - frac) * im;
                fftOut[2 * (binLo + 1)    ] += frac * re;
                fftOut[2 * (binLo + 1) + 1] += frac * im;
            }
        }

        fftOut[0] = (1.0f - blend) * fftA[0] + blend * fftB[0];
        fftOut[1] = (1.0f - blend) * fftA[1] + blend * fftB[1];

        pffft_transform_ordered(setup, fftOut, tempTime, nullptr, PFFFT_BACKWARD);

        constexpr float alpha = 0.05f;
        for (int i = 0; i < 2048; ++i) {
            float sample = tempTime[i] * (1.0f / 2048.0f);
            smoothedOutput[i] = (1.0f - alpha) * smoothedOutput[i] + alpha * sample;
            outputSamples[i] = smoothedOutput[i];
        }

        outputPortBuffers[0]->sampleBuffer.set(outputSamples);
    }

private:
    PFFFT_Setup* setup = nullptr;
    float* fftA = nullptr;
    float* fftB = nullptr;
    float* fftOut = nullptr;
    float* tempTime = nullptr;

    std::vector<float> outputSamples;
    std::vector<float> smoothedOutput;
};
