/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

class Changed : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Changed", "chg", false);
    DEFINE_NODE_ALIASES("chg", "change", "changed");

    float lastValue = std::numeric_limits<float>::quiet_NaN();

public:
    Changed(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Input", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        auto events = inputPortBuffers[0]->getEvents();

        for (Event* e : events)
        {
            float value = e->getAtomValue(0);

            if (std::isnan(lastValue) || value != lastValue)
            {
                lastValue = value;

                if (Event* outEvent = context->eventPool.getFreeEvent())
                {
                    outEvent->setTimeStamp(e->getTimeStamp());
                    context->eventPool.addDataAtomTo(outEvent, value);
                    addEvent(0, outEvent);
                }
            }
        }
    }
};

REGISTER(Changed);