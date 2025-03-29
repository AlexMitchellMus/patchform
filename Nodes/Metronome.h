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

        tickParam->informNodeOfChange = [this]()
        {
            hzValue.store(tickParam->getValue());
        };

        addInputPort("ControlInput", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["hz"] = hzValue.load();
        return nodeCreationData;
    }

    bool shouldProcess(unsigned int frameCount) override
    {
        if (sampleCounter == 0.0f)
            return true;

        // Get the current frequency and recalc tickInterval.
        float currentHz = hzValue.load();
        if (currentHz == 0.0f)
            return false;
        tickInterval = context->sampleRate / currentHz;

        // Calculate how many samples remain until the next tick.
        float samplesUntilNextTick = tickInterval - sampleCounter;

        // If the current block isn't long enough to reach the next tick,
        // simply update sampleCounter and skip processing.
        if (frameCount < samplesUntilNextTick) {
            sampleCounter += frameCount;
            // Optionally wrap sampleCounter if desired:
            if (sampleCounter >= tickInterval)
                sampleCounter = fmod(sampleCounter, tickInterval);
            return false;
        }
        // Otherwise, we know a tick event will occur within this block.
        return true;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        float samplesProcessed = 0.0f;

        // Handle the first tick event if starting at 0.
        if (sampleCounter == 0.0f)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(0);
                outputPortBuffers[0]->addEvent(e);
#ifdef TEST_TIMING
                std::cout << accumulatedFrames << std::endl;
#endif
            }
        }

        // Process the frames, generating tick events as needed.
        while (samplesProcessed < frameCount)
        {
            // Calculate how many samples remain until the next tick.
            float samplesUntilNextTick = tickInterval - sampleCounter;
            float remainingFrames = frameCount - samplesProcessed;

            if (samplesUntilNextTick >= remainingFrames)
            {
                // Not enough frames to reach the next tick.
                sampleCounter += remainingFrames;
                break;
            }

            // Calculate the sample position at which the tick should occur.
            float tickPosition = samplesProcessed + samplesUntilNextTick;

            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(tickPosition);
                outputPortBuffers[0]->addEvent(e);
#ifdef TEST_TIMING
                std::cout << (accumulatedFrames + static_cast<unsigned long>(tickPosition)) << std::endl;
#endif
            }

            // Update sampleCounter and processed frames.
            sampleCounter = sampleCounter + samplesUntilNextTick - tickInterval;
            samplesProcessed = tickPosition;
        }
#ifdef TEST_TIMING
        accumulatedFrames += frameCount;
#endif
    }
};
