#pragma once

#include "AudioNodeBase.h"

class TableXPhase : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXPhase", "tableXPhase", false);
    DEFINE_NODE_ALIASES("tablexphase");

    SampleHandle sampleA;
    SampleHandle sampleB;
    SampleHandle waveformData;

public:
    TableXPhase(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("a", AudioPort::Data);
        addInputPort("b", AudioPort::Data);
        addInputPort("x", AudioPort::Signal); // Phase array: 0.0–1.0
        waveformData = SampleHandle::makeSampleHandle(defaultTableSize);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto bufferA = inputPortBuffers[0]->getEvents();
        const auto bufferB = inputPortBuffers[1]->getEvents();

        if (!bufferA.empty() && bufferA[0]->data->type == DataAtom::DataType::Sample)
            sampleA = bufferA[0]->data->data.sample;

        if (!bufferB.empty() && bufferB[0]->data->type == DataAtom::DataType::Sample)
            sampleB = bufferB[0]->data->data.sample;

        if (!sampleA.isValid() || !sampleB.isValid())
            return;

        const float* a = sampleA.get()->samples.data();
        const float* b = sampleB.get()->samples.data();
        const float* x = inputPortBuffers[2]->getAudioBuffer();

        auto& output = waveformData.get()->samples;

        for (size_t i = 0; i < defaultTableSize; ++i) {
            float mix = std::clamp(x[i], 0.0f, 1.0f); // use per-sample phase
            output[i] = (1.0f - mix) * a[i] + mix * b[i];
        }

        if (auto e = context->eventPool.getFreeEvent()) {
            auto dataAtom = context->eventPool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Sample;
            new (&dataAtom->data.sample) SampleHandle(waveformData);
            e->data = dataAtom;
            e->numAtoms = 1;
            addEvent(0, e);
        }
    }
};

REGISTER(TableXPhase);