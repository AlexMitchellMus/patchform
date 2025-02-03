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
    Count(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port

        countValue = minCount = objParams.value("min", 1);
        maxCount = objParams.value("max", std::numeric_limits<int>::max());
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        const auto aEvents = inputPortBuffers[0]->getEvents();

        for (auto event : aEvents)
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->setTimeStamp(event->getTimeStamp());
                if (countValue > maxCount)
                    countValue = minCount;

                e->data = countValue++;

                // Now add it to the output port’s event list
                outputPort.addEvent(e);
            }
        }
    }
};
