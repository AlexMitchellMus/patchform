/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

// AddNode that sums two signals
class If : public AudioNode {
    DEFINE_AND_REGISTER_NODE("If");

    int coldValueIf;
    float coldValueReturn;

public:
    If(NodeContext* context, int ifValue, float rtnValue) : AudioNode(context, "If", AudioPort::PortType::Data)
    {
        addInputPort("A"); // hot port
        addInputPort("B"); // cold port
        addInputPort("C"); // cold port

        coldValueIf = ifValue;
        coldValueReturn = rtnValue;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].sumEvents();
        if (auto bEvent = inputPorts[1].sumEvents(); bEvent.size())
            coldValueIf = bEvent.back()->data;

        for (auto event : aEvents)
        {
            if (event->data == coldValueIf)
            {
                Event* e = context->eventPool.getFreeEvent();

                if (e)
                {
                    e->setTimeStamp(event->getTimeStamp());
                    e->data = coldValueReturn;

                    // Now add it to the output port’s event list
                    outputPort.addEvent(e);
                }
            }
            context->eventPool.returnFreeEvent(event);
        }
    }
};
