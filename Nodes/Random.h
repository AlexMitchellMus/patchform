#pragma once

#include "AudioNodeBase.h"
#include <cstdlib>

class Random : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Random", "rnd", false);

    FloatParameter* minParam;
    FloatParameter* maxParam;

public:
    Random(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        float minVal = objParams.value("min", 0.0f);
        float maxVal = objParams.value("max", 1.0f);

        minParam = addParameter<FloatParameter>("Min", minVal, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        maxParam = addParameter<FloatParameter>("Max", maxVal, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

        addInputPort("In", AudioPort::PortType::Data);
    }

    void processAudio(float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& events = inputPortBuffers[0]->getEvents();

        const float min = minParam->getValue();
        const float max = maxParam->getValue();

        for (const auto* ev : events)
        {
            if (auto* e = context->eventPool.getFreeEvent())
            {
                const float r = min + ((float)rand() / RAND_MAX) * (max - min);
                context->eventPool.addDataAtomTo(e, r);
                e->setTimeStamp(ev->getTimeStamp());
                outputPortBuffers[0]->addEvent(e);
            }
        }
    }

    json getSerializedNode() override
    {
        nodeCreationData["min"] = minParam->getValue();
        nodeCreationData["max"] = maxParam->getValue();
        return nodeCreationData;
    }
};
