#pragma once

#include "AudioNodeBase.h"

class TableIndexOsc : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableIndexOsc", "tblidxosc", true);

    float phase = 0.0f;
    float freq = 440.0f;
    std::vector<float> internalWaveform;
    std::vector<float> indexWaveform;
    bool hasWaveform = false;
    bool hasIndex = false;

public:
    TableIndexOsc(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("waveform", AudioPort::PortType::Data);   // SampleHandle
        addInputPort("index", AudioPort::PortType::Data);      // optional SampleHandle
        addInputPort("frequency", AudioPort::PortType::Data);  // float

        internalWaveform.assign(2048, 0.0f);
        indexWaveform.assign(2048, 0.0f);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        // Load waveform
        const auto& wfEvents = inputPortBuffers[0]->getEvents();
        if (!wfEvents.empty() && wfEvents[0]->data->type == DataAtom::DataType::Sample) {
            auto& sample = wfEvents[0]->data->data.sample;
            if (sample.isValid()) {
                internalWaveform = sample.get()->samples;
                hasWaveform = !internalWaveform.empty();
            }
        }

        if (!hasWaveform || internalWaveform.size() < 2)
            return;

        // Frequency
        const auto& freqEvents = inputPortBuffers[2]->getEvents();
        for (auto e : freqEvents) {
            if (e->data && e->data->type == DataAtom::DataType::Float)
                freq = e->data->data.atom;
        }

        // Try load index wavetable
        const auto& idxEvents = inputPortBuffers[1]->getEvents();
        if (!idxEvents.empty() && idxEvents[0]->data->type == DataAtom::DataType::Sample) {
            auto& indexSample = idxEvents[0]->data->data.sample;
            if (indexSample.isValid()) {
                indexWaveform = indexSample.get()->samples;
                hasIndex = true;
            }
        }

        // Fallback to identity index table
        if (!hasIndex || indexWaveform.size() < 2) {
            indexWaveform.resize(2048);
            for (size_t i = 0; i < indexWaveform.size(); ++i)
                indexWaveform[i] = static_cast<float>(i) / static_cast<float>(indexWaveform.size() - 1);
        }

        auto* output = outputPortBuffers[0]->getAudioBuffer();
        const float* waveform = internalWaveform.data();
        const size_t tableSize = internalWaveform.size();

        const float invSampleRate = 1.0f / context->sampleRate;

        for (unsigned long i = 0; i < frameCount; ++i) {
            // Advance phase normally
            phase += freq * invSampleRate;
            if (phase >= 1.0f) phase -= 1.0f;

            // Get warped phase from index waveform (treating it as 0..1)
            float indexPhase = phase * (indexWaveform.size() - 1);
            int i0 = static_cast<int>(indexPhase);
            float frac = indexPhase - i0;
            int i1 = std::min(i0 + 1, (int)indexWaveform.size() - 1);

            float warped = hasIndex
                ? std::clamp(indexWaveform[i0] + frac * (indexWaveform[i1] - indexWaveform[i0]), -1.0f, 1.0f) * 0.5f + 0.5f
                : phase;

            // Lookup final waveform using warped phase
            float wavePhase = warped * (tableSize - 1);
            int w0 = static_cast<int>(wavePhase);
            float waveFrac = wavePhase - w0;
            int w1 = std::min(w0 + 1, (int)tableSize - 1);

            output[i] = waveform[w0] + waveFrac * (waveform[w1] - waveform[w0]);
        }
    }
};
