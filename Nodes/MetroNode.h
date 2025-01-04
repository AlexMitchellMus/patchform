#include "AudioNodes.h"

#pragma once

class Metro : public AudioNode
{
    uint64_t sampleCounter = 0;
    uint64_t tickInterval;

public:
    Metro(NodeContext* context, float hz) : AudioNode(context, "Metro")
    {
        tickInterval = static_cast<uint64_t>(context->sampleRate / hz);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        unsigned long samplesProcessed = 0;

        while (samplesProcessed < frameCount)
        {
            unsigned long samplesUntilNextTick = static_cast<unsigned long>(tickInterval - sampleCounter);

            if (samplesUntilNextTick >= (frameCount - samplesProcessed))
            {
                sampleCounter += frameCount - samplesProcessed;
                break;
            }

            unsigned long tickPosition = samplesProcessed + samplesUntilNextTick;
            outputPort.addEvent(tickPosition);

            sampleCounter = 0;
            samplesProcessed = tickPosition + 1;
        }
    }
};