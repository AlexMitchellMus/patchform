/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class If : public AudioNode {
    DEFINE_AND_REGISTER_NODE("If", "if", NullParams);

    int coldValueIf;
    float coldValueReturn;

public:
    If(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port
        addInputPort("B", AudioPort::PortType::Data); // cold port
        addInputPort("C", AudioPort::PortType::Data); // cold port

        coldValueIf = objParams.value("if", 0.0f);
        coldValueReturn = objParams.value("return", 0.0f);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPortBuffers[0]->getEvents();
        if (auto bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
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
                    //Logger::getInstance().logEvent(this, e->getTimeStamp(), e->data);
                }
            }
            //context->eventPool.releaseEvent(event);
        }
    }
};
