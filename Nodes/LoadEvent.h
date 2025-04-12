/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// Output a single event on object load (which is normally patch load)

#pragma once

#include "AudioNodeBase.h"

class LoadEvent final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("LoadEvent", "loadEvent", false);
    DEFINE_NODE_ALIASES("loadevent")

public:

    LoadEvent(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        eventOnLoad = true;
    }

    void processAudio(const float*, float* out, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        if (Event* e = context->eventPool.getFreeEvent())
        {
            addEvent(0, e);
        }
    }
};

REGISTER(LoadEvent);
