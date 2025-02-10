/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <atomic>

// Sample accurate metronome implementation

class Metronome : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Metronome", "metro");

    FloatParameter* tickParam;

    float sampleCounter = 0.0f;
    float tickInterval;
    std::atomic<float> hzValue;

//#define TEST_TIMING
#ifdef TEST_TIMING
    unsigned long accumulatedFrames = 0;
#endif

public:
    Metronome(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        hzValue.store(objParams.value("hz", 1.0f));

        tickParam = addParameter<FloatParameter>("Hz", hzValue.load(), 0.0f, std::numeric_limits<float>::max());

        tickInterval = hzValue.load() == 0.0f ? 0.0f : context->sampleRate / hzValue.load();

        addInputPort("ControlInput", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["hz"] = hzValue.load();
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        float samplesProcessed = 0.0f;

        hzValue.store(tickParam->getValue());

        if (hzValue.load() == 0.0f || tickInterval == 0.0f)
            return;

        tickInterval = context->sampleRate / hzValue.load();

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