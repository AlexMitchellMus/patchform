#pragma once

#include "AudioNodeBase.h"

class Value : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Value", "val", true);

    std::atomic<float> value = 0.0f;           // Smoothed output
    std::atomic<float> targetValue = 0.0f;     // From parameter only
    std::atomic<float> smoothing = 0.0f;

    float eventTarget = 0.0f;                  // Ephemeral override
    FloatParameter* valueParam;
    FloatParameter* smoothingTimeParam;

public:
    Value(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        float initial = objParams.value("value", 0.0f);
        value.store(initial);
        targetValue.store(initial);
        eventTarget = initial;

        valueParam = addParameter<FloatParameter>("value", initial, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        valueParam->informNodeOfChange = [this]() {
            targetValue.store(valueParam->getValue());
            eventTarget = valueParam->getValue(); // Reset to param on change
        };

        float smoothMs = objParams.value("smooth ms", 10.0f);
        smoothing.store(smoothMs);
        smoothingTimeParam = addParameter<FloatParameter>("smooth ms", smoothMs, 0.0f, 1000.0f);
        smoothingTimeParam->informNodeOfChange = [this]() {
            smoothing.store(smoothingTimeParam->getValue());
        };

        addInputPort("data", AudioPort::Data);
    }

    void processAudio(const float*, float* out, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto& dataIn = inputPortBuffers[0]->getEvents();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        float current = value.load(std::memory_order_relaxed);

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
                        eventTarget = atom->data.atom;
                }
                ++nextEventIndex;
            }

            current += (eventTarget - current) * alpha;
            output[i] = current;
        }

        value.store(current, std::memory_order_relaxed);
    }

    json getSerializedNode() override {
        nodeCreationData["value"] = valueParam->getValue();
        nodeCreationData["smooth ms"] = smoothingTimeParam->getValue();
        return nodeCreationData;
    }
};
