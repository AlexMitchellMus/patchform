#pragma once

#include "AudioNodeBase.h"
#include "../Utility/LinearSmoother.h"

class Value : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Value", "val", true);
    DEFINE_NODE_ALIASES("value", "val");

    LinearSmoother smoother;
    float eventTarget = 0.0f;

    FloatParameter* valueParam;
    FloatParameter* smoothingTimeParam;

public:
    Value(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        float initial = objParams.value("value", 0.0f);
        float smoothMs = objParams.value("smooth ms", 10.0f);

        smoother.setSampleRate(context->sampleRate);
        smoother.setSmoothTime(smoothMs * 0.001f); // convert ms to seconds
        smoother.clear(initial);
        eventTarget = initial;

        valueParam = addParameter<FloatParameter>("value", initial,
                        -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        valueParam->informNodeOfChange = [this]() {
            eventTarget = valueParam->getValue();
            smoother.clear(eventTarget); // reset to new param
        };

        smoothingTimeParam = addParameter<FloatParameter>("smooth ms", smoothMs, 0.0f, 1000.0f);
        smoothingTimeParam->informNodeOfChange = [this]() {
            smoother.setSmoothTime(smoothingTimeParam->getValue() * 0.001f);
        };

        addInputPort("data", AudioPort::Data);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto& dataIn = inputPortBuffers[0]->getEvents();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        unsigned int nextEventIndex = 0;
        for (unsigned long i = 0; i < frameCount; ++i)
        {
            while (nextEventIndex < dataIn.size() && dataIn[nextEventIndex]->getTimeStamp() == i)
            {
                if (auto* atom = dataIn[nextEventIndex]->getAtom(0)) {
                    if (atom->type == DataAtom::DataType::Float)
                        eventTarget = atom->data.atom;
                }
                ++nextEventIndex;
            }
            output[i] = eventTarget;
        }

        smoother.process(output, output, frameCount, false);
    }

    json getSerializedNode() override {
        nodeCreationData["value"] = valueParam->getValue();
        nodeCreationData["smooth ms"] = smoothingTimeParam->getValue();
        return nodeCreationData;
    }
};

REGISTER(Value);
