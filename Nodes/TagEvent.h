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
    DEFINE_AND_REGISTER_NODE("TagEvent", "tag");

    std::string tag;
    StringParameter* tagParameter;

public:
    TagEvent(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
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

    void processAudio(float* out, unsigned long frameCount) override
    {
        const auto& events = inputPortBuffers[0]->getEvents();
        tag = tagParameter->getValue();

        for (const auto* ev : events)
        {
            if (auto* e = context->eventPool.getFreeEvent())
            {
                e->shallowCopyFrom(ev);
                e->setTag(tag);

                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};
