/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Count : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Count", "cnt");

    float countValue;
    int minCount;
    int maxCount;

public:
    Count(NodeContext* context, int min, int max) : AudioNode(std::make_unique<NullState>(), context, AudioPort::PortType::Data)
    {
        addInputPort("A"); // hot port
        countValue = minCount = min;
        maxCount = max;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].sumEvents();

        for (auto event : aEvents)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->setTimeStamp(event->getTimeStamp());
                if (countValue > maxCount)
                    countValue = minCount;
                e->data = countValue;

                countValue++;

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
            }
        }
    }
};
