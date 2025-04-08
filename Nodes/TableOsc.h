#pragma once

#include "AudioNodeBase.h"

class TableOsc : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableOsc", "tblosc", true);

    float phase = 0.0f;
    float freq = 440.0f;
    SampleHandle currentWaveform;

public:
    TableOsc(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("waveform", AudioPort::PortType::Data);
        addInputPort("frequency", AudioPort::PortType::Data);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        // Pull waveform from event port
        const auto& waveformEvents = inputPortBuffers[0]->getEvents();
        if (!waveformEvents.empty() && waveformEvents[0]->data->type == DataAtom::DataType::Sample)
            currentWaveform = waveformEvents[0]->data->data.sample;

        if (!currentWaveform.isValid())
            return;

        auto* output = outputPortBuffers[0]->getAudioBuffer();

        const auto* waveform = currentWaveform.get()->samples.data();
        const size_t tableSize = currentWaveform.get()->samples.size();

        // Pull frequency from event port
        const auto& freqEvents = inputPortBuffers[1]->getEvents();
        for (auto e : freqEvents) {
            if (e->data && e->data->type == DataAtom::DataType::Float)
                freq = e->data->data.atom;
        }

        if (tableSize < 2)
            return;

        const float invSampleRate = 1.0f / context->sampleRate;

        // Check if tableSize is power of two
        const bool isPowerOfTwo = (tableSize & (tableSize - 1)) == 0;
        const size_t mask = tableSize - 1;

        for (unsigned long i = 0; i < frameCount; ++i) {
            phase += freq * invSampleRate;
            if (phase >= 1.0f) phase -= 1.0f;
            if (phase < 0.0f) phase += 1.0f;

            float idx = phase * static_cast<float>(tableSize);
            int i0 = static_cast<int>(idx);
            float frac = idx - i0;

            int i1 = isPowerOfTwo ? ((i0 + 1) & mask) : ((i0 + 1) % tableSize);

            // Safe bounds
            i0 = std::clamp(i0, 0, static_cast<int>(tableSize - 1));
            i1 = std::clamp(i1, 0, static_cast<int>(tableSize - 1));

            float s0 = waveform[i0];
            float s1 = waveform[i1];

            output[i] = s0 + frac * (s1 - s0);
        }
    }
};
