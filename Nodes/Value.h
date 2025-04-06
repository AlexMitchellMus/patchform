#pragma once

#include "AudioNodeBase.h"

class Value : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Value", "val", true);

    std::atomic<float> value = 0.0f;           // Current smoothed output
    std::atomic<float> targetValue = 0.0f;     // Latest target
    std::atomic<float> smoothing = 0.0f;

    FloatParameter* valueParam;
    FloatParameter* smoothingTimeParam;

public:
    Value(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        float initial = objParams.value("value", 0.0f);
        value.store(initial);
        targetValue.store(initial);

        valueParam = addParameter<FloatParameter>("value", initial,
            -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        valueParam->informNodeOfChange = [this]() {
            targetValue.store(valueParam->getValue());
        };

        smoothingTimeParam = addParameter<FloatParameter>("smooth ms", 10.0f, 0.0f, 1000.0f);
        smoothingTimeParam->informNodeOfChange = [this]() {
            smoothing.store(smoothingTimeParam->getValue());
        };

        addInputPort("data", AudioPort::Data);
    }

    void processAudio(const float*, float* out, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto& dataIn = inputPortBuffers[0]->getEvents();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        float current = value.load();
        float target = targetValue.load();

        float smoothingTimeSec = smoothing.load() * 0.001f;
        float alpha = (smoothingTimeSec > 0.0f)
            ? 1.0f - std::exp(-1.0f / (context->sampleRate * smoothingTimeSec))
            : 1.0f;

        unsigned int nextEventIndex = 0;
        for (unsigned long i = 0; i < frameCount; ++i)
        {
            while (nextEventIndex < dataIn.size() && dataIn[nextEventIndex]->getTimeStamp() == i)
            {
                if (auto* atom = dataIn[nextEventIndex]->getAtom(0)) {
                    if (atom->type == DataAtom::DataType::Float)
                        target = atom->data.atom;
                }
                ++nextEventIndex;
            }

            current += (target - current) * alpha;
            output[i] = current;
        }

        value.store(current);
        targetValue.store(target);
    }

    json getSerializedNode() override {
        nodeCreationData["value"] = targetValue.load();
        nodeCreationData["smooth ms"] = smoothingTimeParam->getValue();
        return nodeCreationData;
    }
};
