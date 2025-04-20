#pragma once

#include "pffft.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include "AudioNodeBase.h"
#include "simde/x86/sse.h"

class TableXSpectral : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("TableXSpectral", "tableXspectral", true);
    DEFINE_NODE_ALIASES("tablexspectral");

    PFFFT_Setup* setup = nullptr;
    float* fftA = nullptr;
    float* fftB = nullptr;
    float* fftOut = nullptr;
    float* tempTime = nullptr;

    SampleHandle sampleA;
    SampleHandle sampleB;
    SampleHandle waveformData;
    std::vector<float> smoothedOutput;

    float blend = 0.0f;
    const float convergenceEpsilon = 1e-4f;
    const float smoothingAlpha = 0.05f;

    int remainingFrames = 0;
    float targetBlend = 0.0f;

public:
    TableXSpectral(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("a", AudioPort::Data);
        addInputPort("b", AudioPort::Data);
        addInputPort("x", AudioPort::Data); // Blend 0–1

        setup = pffft_new_setup(defaultTableSize, PFFFT_REAL);
        fftA = (float*)pffft_aligned_malloc(defaultTableSize * sizeof(float));
        fftB = (float*)pffft_aligned_malloc(defaultTableSize * sizeof(float));
        fftOut = (float*)pffft_aligned_malloc(defaultTableSize * sizeof(float));
        tempTime = (float*)pffft_aligned_malloc(defaultTableSize * sizeof(float));

        waveformData = SampleHandle::makeSampleHandle(defaultTableSize);
        smoothedOutput.assign(defaultTableSize, 0.0f);
    }

    ~TableXSpectral() override
    {
        pffft_aligned_free(fftA);
        pffft_aligned_free(fftB);
        pffft_aligned_free(fftOut);
        pffft_aligned_free(tempTime);
        pffft_destroy_setup(setup);
    }

    struct Peak
    {
        float bin;
        float mag;
        float phase;
    };

    void extractPeaks(const float* fft, Peak* peaks, int& count, int maxPeaks)
    {
        count = 0;
        for (int i = 1; i < (defaultTableSize / 2) - 1; ++i)
        {
            float re = fft[2 * i];
            float im = fft[2 * i + 1];
            float mag = std::sqrt(re * re + im * im);
            if (mag > 1e-6f && count < maxPeaks)
            {
                float phase = std::atan2(im, re);
                peaks[count++] = {(float)i, mag, phase};
            }
        }
        std::sort(peaks, peaks + count, [](const Peak& a, const Peak& b)
        {
            return a.mag > b.mag;
        });
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        const auto bufferA = inputPortBuffers[0]->getEvents();
        const auto bufferB = inputPortBuffers[1]->getEvents();
        const auto bufferX = inputPortBuffers[2]->getEvents();

        bool samplesUpdated = false;

        if (!bufferA.empty() && bufferA[0]->data->type == DataAtom::DataType::Sample)
        {
            sampleA = bufferA[0]->data->data.sample;
            samplesUpdated = true;
        }

        if (!bufferB.empty() && bufferB[0]->data->type == DataAtom::DataType::Sample)
        {
            sampleB = bufferB[0]->data->data.sample;
            samplesUpdated = true;
        }

        if (!sampleA.isValid() || !sampleB.isValid())
            return;

        for (auto it = bufferX.rbegin(); it != bufferX.rend(); ++it)
        {
            if ((*it)->data && (*it)->data->type == DataAtom::DataType::Float)
            {
                float newBlend = std::clamp((*it)->data->data.atom, 0.0f, 1.0f);
                onBlendChanged(newBlend);
                break;
            }
        }

        if (samplesUpdated)
        {
            const float* a = sampleA.get()->samples.data();
            const float* b = sampleB.get()->samples.data();

            pffft_transform_ordered(setup, a, fftA, nullptr, PFFFT_FORWARD);
            pffft_transform_ordered(setup, b, fftB, nullptr, PFFFT_FORWARD);

            constexpr float updateBoost = 2.0f;
            remainingFrames = static_cast<int>(
                updateBoost * std::ceil(std::log(convergenceEpsilon) / std::log(1.0f - smoothingAlpha))
            );
        }

        if (remainingFrames <= 0)
            return;

        --remainingFrames;

        float blendCurve = targetBlend * targetBlend;
        float phaseBlend = std::sqrt(targetBlend);
        float binBlend = 0.5f * (1.0f - SpectralHelpers::constexprCos(targetBlend * M_PI));

        Peak peaksA[128], peaksB[128];
        int countA = 0, countB = 0;
        extractPeaks(fftA, peaksA, countA, 128);
        extractPeaks(fftB, peaksB, countB, 128);
        const size_t count = std::min(countA, countB);

        std::fill_n(fftOut, defaultTableSize, 0.0f);

        for (size_t i = 0; i < count; ++i)
        {
            const float bin = (1.0f - binBlend) * peaksA[i].bin + binBlend * peaksB[i].bin;
            const float mag = std::sqrt(fma(1.0f - blendCurve, peaksA[i].mag * peaksA[i].mag, blendCurve * peaksB[i].mag * peaksB[i].mag));
            const float phase = (1.0f - phaseBlend) * peaksA[i].phase + phaseBlend * peaksB[i].phase;

            const int binLo = static_cast<int>(std::floor(bin));
            const float frac = bin - binLo;
            const float re = mag * std::cos(phase);
            const float im = mag * std::sin(phase);

            if (binLo >= 1 && binLo < (defaultTableSize / 2) - 1)
            {
                fftOut[2 * binLo] += (1.0f - frac) * re;
                fftOut[2 * binLo + 1] += (1.0f - frac) * im;
                fftOut[2 * (binLo + 1)] += frac * re;
                fftOut[2 * (binLo + 1) + 1] += frac * im;
            }
        }

        fftOut[0] = (1.0f - targetBlend) * fftA[0] + targetBlend * fftB[0];
        fftOut[1] = (1.0f - targetBlend) * fftA[1] + targetBlend * fftB[1];

        pffft_transform_ordered(setup, fftOut, tempTime, nullptr, PFFFT_BACKWARD);

        auto& output = waveformData.get()->samples;
        const float* rawA = sampleA.get()->samples.data();
        const float* rawB = sampleB.get()->samples.data();

        // Edge blend fades in raw table near blend=0 or blend=1
        // This is because the spectral content of A or B can influence how if the extremes will ever be reached.
        float edgeFade = 1.0f - std::clamp(targetBlend * (1.0f - targetBlend) * 16.0f, 0.0f, 1.0f);

        for (int i = 0; i < defaultTableSize; ++i)
        {
            float raw = (1.0f - targetBlend) * rawA[i] + targetBlend * rawB[i];
            float spec = tempTime[i] * (1.0f / defaultTableSize);
            float mixed = (1.0f - edgeFade) * spec + edgeFade * raw;

            smoothedOutput[i] = (1.0f - smoothingAlpha) * smoothedOutput[i] + smoothingAlpha * mixed;
            output[i] = smoothedOutput[i];
        }

        // Normalize wavetable to -1,1 range
        float maxAmp = 0.0f;
        for (float v : output)
            maxAmp = std::max(maxAmp, std::abs(v));

        if (maxAmp > 1.0f && maxAmp > 0.0f) {
            float scale = 1.0f / maxAmp;
            for (float& v : output)
                v *= scale;
        }

        // Emit event with blended sample
        if (auto e = context->eventPool.getFreeEvent())
        {
            auto dataAtom = context->eventPool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Sample;
            new(&dataAtom->data.sample) SampleHandle(waveformData);
            e->data = dataAtom;
            e->numAtoms = 1;
            addEvent(0, e);
        }
    }

    void onBlendChanged(float newBlend)
    {
        if (std::abs(newBlend - targetBlend) > 1e-4f)
        {
            targetBlend = newBlend;
            remainingFrames = static_cast<int>(
                std::ceil(std::log(convergenceEpsilon) / std::log(1.0f - smoothingAlpha))
            );
        }
    }
};

REGISTER(TableXSpectral);