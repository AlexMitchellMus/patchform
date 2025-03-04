/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class MidiToFreq : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("MIDI2Freq", "mtof");

    float midiNote = 60.0f; // Default to middle C

public:
    MidiToFreq(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("MIDI Note", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();

        for (Event* e : events)
        {
            int noteValue = e->getAtomValue(0);
            noteValue = std::clamp(noteValue, 0, 128);

            // 12 tone equal tempered for now. Tuning 440hz
            // TODO: make tuning a parameter / value
            // TODO: use scala format for tuning table
            float frequency = 440.0f * std::pow(2.0f, (noteValue - 69.0f) / 12.0f);

            Event* outEvent = context->eventPool.getFreeEvent();
            if (outEvent)
            {
                outEvent->setTimeStamp(e->getTimeStamp());
                outEvent->addAtom(frequency);
                outputPort.addEvent(outEvent);
            }
        }
    }
};
