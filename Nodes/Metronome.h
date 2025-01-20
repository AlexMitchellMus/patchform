/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// Sample accurate metronome implementation

class Metronome : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Metronome", "metro");

    float sampleCounter = 0.0f;
    float tickInterval;

//#define TEST_TIMING
#ifdef TEST_TIMING
    unsigned long accumulatedFrames = 0;
#endif

public:
    Metronome(NodeContext* context, const json& nodeData) : AudioNode(context, AudioPort::PortType::Data, nodeData)
    {
        auto const hz = nodeData.value("hz", 1.0f);
        tickInterval = context->sampleRate / hz;

        addInputPort("ControlInput", AudioPort::PortType::Data);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        float samplesProcessed = 0.0f;

        auto events = inputPortBuffers[0]->getEvents();

        // Handle first event in metronome
        if (sampleCounter == 0.0)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e) {
                e->setTimeStamp(0); // Set event at time 0
                outputPort.addEvent(e);

#ifdef TEST_TIMING
                std::cout << accumulatedFrames << std::endl;
#endif
            }
        }

        while (samplesProcessed < frameCount)
        {
            double samplesUntilNextTick = tickInterval - sampleCounter;

            if (samplesUntilNextTick >= (frameCount - samplesProcessed))
            {
                sampleCounter += frameCount - samplesProcessed;
                break;
            }

            float tickPosition = samplesProcessed + samplesUntilNextTick;

            Event* e = context->eventPool.getFreeEvent();
            if (e)
            {
                e->setTimeStamp(tickPosition);
                outputPort.addEvent(e);
#ifdef TEST_TIMING
                std::cout << (accumulatedFrames + static_cast<unsigned long>(tickPosition)) << std::endl;
#endif
            }

            sampleCounter = (sampleCounter + samplesUntilNextTick) - tickInterval;
            samplesProcessed = tickPosition;
        }
#ifdef TEST_TIMING
        accumulatedFrames += frameCount;
#endif
    }
};