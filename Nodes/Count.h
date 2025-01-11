/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

// AddNode that sums two signals
class Count : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Count");

    float countValue;

public:
    Count(NodeContext* context) : AudioNode(context, "AddNode")
    {
        addInputPort("A"); // hot port
        countValue = 0.0f;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].combineEvents();

        for (auto event : aEvents)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->setTimeStamp(event->getTimeStamp());
                e->data = countValue++;

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
            }
        }
    }
};
