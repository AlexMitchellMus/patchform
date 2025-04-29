#pragma once

#include "AudioNodeBase.h"

class TableXFade : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXfade", "tableXfade", true);
    DEFINE_NODE_ALIASES("tablexfade");

    SampleHandle sampleA;
    SampleHandle sampleB;
    SampleHandle waveformData;

public:
    TableXFade(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("a", AudioPort::Data);
        addInputPort("b", AudioPort::Data);
        addInputPort("x", AudioPort::Signal); // Blend 0–1

        waveformData = SampleHandle::makeSampleHandle(defaultTableSize);
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        const auto bufferA = inputPortBuffers[0]->getEvents();
        const auto bufferB = inputPortBuffers[1]->getEvents();

        // Pull in sample handles from events
        if (!bufferA.empty() && bufferA[0]->data->type == DataAtom::DataType::Sample)
            sampleA = bufferA[0]->data->data.sample;

        if (!bufferB.empty() && bufferB[0]->data->type == DataAtom::DataType::Sample)
            sampleB = bufferB[0]->data->data.sample;

        if (!sampleA.isValid() || !sampleB.isValid())
            return;

        const float* a = sampleA.get()->samples.data();
        const float* b = sampleB.get()->samples.data();
        const float* x = inputPortBuffers[2]->getAudioBuffer();

        float mix = std::clamp(x[0], 0.0f, 1.0f);

        auto& output = waveformData.sample->samples;
        for (size_t i = 0; i < defaultTableSize; ++i)
            output[i] = (1.0f - mix) * a[i] + mix * b[i];

        // Send output as Sample event
        if (auto e = context->eventPool.getFreeEvent())
        {
            auto dataAtom = context->eventPool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Sample;
            new (&dataAtom->data.sample) SampleHandle(waveformData);
            e->data = dataAtom;
            e->numAtoms = 1;
            addEvent(0, e);
        }
    }
};

REGISTER(TableXFade);