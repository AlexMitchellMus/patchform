#pragma once

#include "AudioNodeBase.h"

class TableXFade : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXfade", "tableXfade", true);

    std::vector<float> outputSamples;

public:
    TableXFade(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Samples, objParams)
    {
        addInputPort("a", AudioPort::Samples);
        addInputPort("b", AudioPort::Samples);
        addInputPort("x", AudioPort::Signal); // Blend 0–1

        outputSamples.assign(defaultTableSize, 0.0f);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto bufferA = inputPortBuffers[0]->sampleBuffer;
        const auto bufferB = inputPortBuffers[1]->sampleBuffer;

        if (bufferA.size != 2048 || bufferB.size != 2048)
        {
            std::fill(outputSamples.begin(), outputSamples.end(), 0.0f);
            outputPortBuffers[0]->sampleBuffer.reset();

            return;
        }

        const float* a = bufferA.samples;
        const float* b = bufferB.samples;

        const float* x = inputPortBuffers[2]->getAudioBuffer();

        // Assume all buffers are same size (i.e., table length)
        for (size_t i = 0; i < defaultTableSize; ++i)
        {
            float mix = std::clamp(x[0], 0.0f, 1.0f); // Use first sample of control signal
            outputSamples[i] = (1.0f - mix) * a[i] + mix * b[i];
        }

        outputPortBuffers[0]->sampleBuffer.set(outputSamples);
    }
};
