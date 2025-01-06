#include "AudioNodes.h"

#pragma once

class Metronome : public AudioNode
{
    uint64_t sampleCounter = 0;
    uint64_t tickInterval;

public:
    Metronome(NodeContext* context, float hz) : AudioNode(context, "Metro")
    {
        tickInterval = static_cast<uint64_t>(context->sampleRate / hz);

        addInputPort("ControlInput");
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        unsigned long samplesProcessed = 0;

        auto events = inputPorts[0].combineEvents();

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
            }
            else {
                // If you get nullptr, you ran out of free events.
                // handle it (grow pool outside RT or skip event, etc.)
            }

            sampleCounter = 0;
            samplesProcessed = tickPosition + 1;
        }
    }
};