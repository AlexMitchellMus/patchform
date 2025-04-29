/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class TagEvent : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("TagEvent", "tag", false);
    DEFINE_NODE_ALIASES("tag");

    std::string tag;
    StringParameter* tagParameter;

public:
    TagEvent(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        tag = objParams.value("symbol", "");
        tagParameter = addParameter<StringParameter>("symbol", tag);

        addInputPort("In", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["symbol"] = tagParameter->getValue();
        return nodeCreationData;
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& events = inputPortBuffers[0]->getEvents();
        tag = tagParameter->getValue();

        for (const auto* ev : events)
        {
            if (auto* e = context->eventPool.getFreeEvent())
            {
                e->shallowCopyFrom(ev);
                e->setTag(tag);

                addEvent(0, e);
            }
        }
    }
};

REGISTER(TagEvent);