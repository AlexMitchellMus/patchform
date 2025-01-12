/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Add : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Add");

    float coldValue;

public:
    Add(NodeContext* context, float initValue) : AudioNode(std::make_unique<NullState>(), context, AudioPort::PortType::Data), coldValue(initValue)
    {
        addInputPort("A"); // hot port
        addInputPort("B"); // cold port
                            
        coldValue = initValue;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].sumEvents();
        if (auto bEvent = inputPorts[1].sumEvents(); bEvent.size())
            coldValue = bEvent.back()->data;

        for (auto event : aEvents)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->setTimeStamp(event->getTimeStamp());
                e->data = event->data + coldValue;

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
            }
        }
    }
};
