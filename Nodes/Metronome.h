/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

class Metronome : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Metronome");

    uint64_t sampleCounter = 0;
    uint64_t tickInterval;

public:
    Metronome(NodeContext* context, float hz) : AudioNode(context, "Metro", AudioPort::PortType::Data)
    {
        tickInterval = static_cast<uint64_t>(context->sampleRate / hz);

        addInputPort("ControlInput");
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        unsigned long samplesProcessed = 0;

        auto events = inputPorts[0].sumEvents();

        // Handle first event in metronome
        if (sampleCounter == 0)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e) {
                e->setTimeStamp(0); // Set event at time 0
                outputPort.addEvent(e);
                //Logger::getInstance().logEvent(this, e->getTimeStamp(), e->data);
            }
            else {
                // Handle out-of-event-pool condition
            }

            sampleCounter = 0;
        }

        while (samplesProcessed < frameCount)
        {
            unsigned long samplesUntilNextTick = static_cast<unsigned long>(tickInterval - sampleCounter);

            if (samplesUntilNextTick >= (frameCount - samplesProcessed))
            {
                sampleCounter += frameCount - samplesProcessed;
                break;
            }

            unsigned long tickPosition = samplesProcessed + samplesUntilNextTick;

            Event* e = context->eventPool.getFreeEvent();

            if (e) {
                e->setTimeStamp(tickPosition);

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
                //Logger::getInstance().logEvent(this, e->getTimeStamp(), e->data);
            }
            else {
                // If you get nullptr, you ran out of free events.
                // handle it (grow pool outside RT or skip event, etc.)
            }

            sampleCounter = 0;
            samplesProcessed = tickPosition;
        }
    }
};