/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Add : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Add", "add");

    float coldValue;

public:
    explicit Add(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port
        addInputPort("B", AudioPort::PortType::Data); // cold port

        coldValue = objParams.value("value", 0.0f);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPortBuffers[0]->getEvents();
        if (auto bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
        {
            coldValue= bEvent.back()->getAtomValue(0);
        }

        for (auto event : aEvents)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());
                e->addAtom(event->getAtomValue(0) + coldValue);

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
            }
        }
    }
};
