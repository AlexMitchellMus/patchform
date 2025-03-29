#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class FilterTag : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("FilterTag", "filtag");

    std::string tag;
    StringParameter* tagParameter;

public:
    FilterTag(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        tag = objParams.value("tag", "");
        tagParameter = addParameter<StringParameter>("Tag", tag);

        addInputPort("In", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["tag"] = tagParameter->getValue();
        return nodeCreationData;
    }

    bool shouldProcess(unsigned int frameCount) override
    {
        return hasInputEvents.load(std::memory_order_relaxed);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        const auto& events = inputPortBuffers[0]->getEvents();
        tag = tagParameter->getValue();

        for (auto* ev : events)
        {
            // Only pass the event through if its tag equals the tag parameter.
            if (ev->getTagHash() == hash(tag))
            {
                outputPortBuffers[0]->addEvent(ev);
            }
        }
    }
};
