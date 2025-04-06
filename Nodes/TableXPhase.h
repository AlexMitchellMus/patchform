#pragma once

#include "AudioNodeBase.h"

class TableXPhase : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXPhase", "tableXphase", true);

    std::vector<float> outputSamples;

public:
    TableXPhase(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Samples, objParams)
    {
        addInputPort("a", AudioPort::Samples);   // source wavetable
        addInputPort("b", AudioPort::Samples);   // phase table

        outputSamples.assign(defaultTableSize, 0.0f);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto sourceBuffer = inputPortBuffers[0]->sampleBuffer;
        const auto phaseBuffer = inputPortBuffers[1]->sampleBuffer;
        if (sourceBuffer.size != 2048 || phaseBuffer.size != 2048)
        {
            std::fill(outputSamples.begin(), outputSamples.end(), 0.0f);
            outputPortBuffers[0]->sampleBuffer.reset();
            return;
        }

        const auto source = sourceBuffer.samples;
        const auto phase = phaseBuffer.samples;

        for (size_t i = 0; i < defaultTableSize; ++i)
        {
            float p = std::clamp(phase[i], 0.0f, 1.0f);  // phase offset
            float pos = p * defaultTableSize - 1;
            int index = static_cast<int>(pos);
            float frac = pos - index;

            float a0 = source[index % defaultTableSize];
            float a1 = source[(index + 1) % defaultTableSize];
            outputSamples[i] = a0 + frac * (a1 - a0); // linear interpolation
        }

        outputPortBuffers[0]->sampleBuffer.set(outputSamples);
    }
};
