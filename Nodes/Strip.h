/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Strip removes data atoms from events, return's anly an event
class Strip : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Strip", "strip");

    IntParameter *atomNumberParam;
    int atomNumber;

public:
    Strip(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data);
    }

    bool shouldProcess(unsigned int frameCount) override
    {
        return hasInputEvents.load(std::memory_order_relaxed);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (const auto event : aEvents)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());

                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};