/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

class Changed : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Changed", "chg");

    float lastValue = std::numeric_limits<float>::quiet_NaN();

public:
    Changed(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Input", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();

        for (Event* e : events)
        {
            float value = e->data;

            if (std::isnan(lastValue) || value != lastValue)
            {
                lastValue = value;

                Event* outEvent = context->eventPool.getFreeEvent();
                if (outEvent)
                {
                    outEvent->setTimeStamp(e->getTimeStamp());
                    outEvent->data = value;
                    outputPort.addEvent(outEvent);
                }
            }
        }
    }
};