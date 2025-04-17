
#pragma once

#include "AudioNodeBase.h"

#define SAMPLERATE_STATIC
#include "samplerate.h"

class TableOsc : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableOsc", "tblosc", true);
    DEFINE_NODE_ALIASES("tableosc");

    float phase = 0.0f;
    float freq = 440.0f;
    std::vector<float> internalWaveform;
    bool hasWaveform = false;

    SRC_STATE* srcState = nullptr;
    size_t upLen;
    std::vector<float> oversampled;
    std::vector<float> filtered;
    int oversampleFactor = 8;

public:
    TableOsc(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("waveform", AudioPort::PortType::Data);
        addInputPort("frequency", AudioPort::PortType::Data);
        internalWaveform.assign(2048, 0.0f);
        srcState = src_new(SRC_SINC_FASTEST, 1, nullptr);

        upLen = context->frameCount * oversampleFactor;  // safety margin
        oversampled.resize(upLen, 0.0f);
        filtered.resize(context->frameCount, 0.0f);
    }

    ~TableOsc()
    {
        if (srcState)
            src_delete(srcState);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto& waveformEvents = inputPortBuffers[0]->getEvents();
        if (!waveformEvents.empty() && waveformEvents[0]->data->type == DataAtom::DataType::Sample) {
            auto& sample = waveformEvents[0]->data->data.sample;
            if (sample.isValid()) {
                internalWaveform = sample.get()->samples;
                hasWaveform = !internalWaveform.empty();
            }
        }

        if (!hasWaveform || internalWaveform.size() < 2)
            return;

        const float* waveform = internalWaveform.data();
        const size_t tableSize = internalWaveform.size();

        const auto& freqEvents = inputPortBuffers[1]->getEvents();
        for (auto e : freqEvents)
            if (e->data && e->data->type == DataAtom::DataType::Float)
                freq = e->data->data.atom;

        const float phaseIncrement = freq / context->sampleRate;

        for (size_t i = 0; i < upLen; ++i) {
            phase += phaseIncrement / oversampleFactor;
            if (phase >= 1.0f) phase -= 1.0f;
            if (phase < 0.0f) phase += 1.0f;

            float idx = phase * static_cast<float>(tableSize);
            int i0 = std::min(static_cast<int>(idx), static_cast<int>(tableSize - 2));
            int i1 = i0 + 1;
            float frac = idx - static_cast<float>(i0);

            float s0 = waveform[i0];
            float s1 = waveform[i1];

            oversampled[i] = s0 + frac * (s1 - s0);
        }

        auto* output = outputPortBuffers[0]->getAudioBuffer();

        SRC_DATA srcData;
        srcData.data_in = oversampled.data();
        srcData.input_frames = static_cast<long>(upLen);
        srcData.data_out = filtered.data();
        srcData.output_frames = static_cast<long>(frameCount);
        srcData.src_ratio = 1.0 / oversampleFactor;
        srcData.end_of_input = 0;

        if (!srcState) {
            std::fill_n(output, frameCount, 0.0f);
            return;
        }

        int err = src_process(srcState, &srcData);
        if (err != 0) {
            std::fill_n(output, frameCount, 0.0f);
            return;
        }

        // Fill output with filtered data
        long actual = std::min(srcData.output_frames_gen, static_cast<long>(frameCount));
        std::copy(filtered.begin(), filtered.begin() + actual, output);
        if (actual < static_cast<long>(frameCount))
            std::fill(output + actual, output + frameCount, 0.0f);
    }
};

REGISTER(TableOsc);
