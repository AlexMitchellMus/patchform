/*
 // Copyright (c) 2024-2025 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>
#include <algorithm>

class MidiToFreq : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("MIDI2Freq", "mtof");

    // Storage for the Scala tuning table.
    // If set via a "scale" event, it is assumed to cover one octave.
    float scalaRatios[128] = { 0.0f };
    int tuningCount = 0; // Number of ratios provided

public:
    MidiToFreq(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("MIDI Note", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    bool shouldProcess(unsigned int frameCount) override
    {
        return hasInputEvents.load(std::memory_order_relaxed);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();

        for (Event* e : events)
        {
            // If the event is a scale event, update the tuning table and do NOT output a frequency.
            switch (e->getTagHash())
            {
            case hash("scale"):
                {
                    tuningCount = std::min(static_cast<int>(e->getNumAtoms()), 128);
                    for (int i = 0; i < tuningCount; i++)
                    {
                        scalaRatios[i] = e->getAtomValue(i);
                    }
                }
                break;
            default:

                // Otherwise, treat it as a note event
                float noteValue = e->getAtomValue(0);
                noteValue = std::clamp(noteValue, 0.0f, 127.0f);

                float frequency = 0.0f;
                if (tuningCount >= 2)
                {
                    // Use the stored tuning table to map the MIDI note to frequency.
                    // We assume the table covers one octave.
                    int steps = tuningCount;
                    int noteOctave = noteValue / steps;
                    int noteDegree = static_cast<int>(noteValue) % steps;

                    // Use A4 (MIDI 69) as a reference.
                    int refOctave = 69 / steps;
                    int refDegree = 69 % steps;

                    if (scalaRatios[refDegree] != 0.0f)
                    {
                        float ratio = scalaRatios[noteDegree] / scalaRatios[refDegree];
                        int octaveDiff = noteOctave - refOctave;
                        frequency = 440.0f * ratio * std::pow(2.0f, octaveDiff);
                    }
                    else
                    {
                        // Fall back if reference ratio is zero.
                        frequency = 440.0f * std::pow(2.0f, (noteValue - 69.0f) / 12.0f);
                    }
                }
                else
                {
                    // No valid tuning table; use standard 12-tone equal temperament.
                    frequency = 440.0f * std::pow(2.0f, (noteValue - 69.0f) / 12.0f);
                }

                // Output a frequency event for note events.
                if (Event* outEvent = context->eventPool.getFreeEvent())
                {
                    outEvent->setTimeStamp(e->getTimeStamp());
                    outEvent->addAtom(frequency);
                    outputPortBuffers[0]->addEvent(outEvent);
                }
            }
        }
    }
};
