/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

// AddNode that sums two signals
class Add : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Add");

    float coldValue;

public:
    Add(NodeContext* context, float initValue) : AudioNode(context, "AddNode"), coldValue(initValue)
    {
        addInputPort("A"); // hot port
        addInputPort("B"); // cold port
                            
        coldValue = initValue;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].combineEvents();
        if (auto bEvent = inputPorts[1].combineEvents(); bEvent.size())
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
            context->eventPool.returnFreeEvent(event);
        }
    }
};
