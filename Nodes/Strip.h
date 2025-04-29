/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Strip removes data atoms from events, return's anly an event
class Strip : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Strip", "strip", false);
    DEFINE_NODE_ALIASES("strip");

    IntParameter *atomNumberParam;
    int atomNumber;

public:
    Strip(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data);
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (const auto event : aEvents)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());
                addEvent(0, e);
            }
        }
    }
};

REGISTER(Strip);