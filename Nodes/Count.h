/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Count : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Count", "count");

    IntParameter* minCountParam;
    IntParameter* maxCountParam;

    float countValue;
    int minCount;
    int maxCount;

public:
    Count(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port

        countValue = minCount = objParams.value("min", 1);
        maxCount = objParams.value("max", 10);

        minCountParam = addParameter<IntParameter>("Min", minCount, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        maxCountParam = addParameter<IntParameter>("Max", maxCount, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    }

    json getSerializedNode() override
    {
        nodeCreationData["min"] = minCountParam->getValue();
        nodeCreationData["max"] = maxCountParam->getValue();
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        minCount = minCountParam->getValue();
        maxCount = maxCountParam->getValue();

        const auto aEvents = inputPortBuffers[0]->getEvents();

        for (const auto* event : aEvents)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());
                if (countValue > maxCount)
                    countValue = minCount;

                e->addAtom(countValue++);

                // Now add it to the output port’s event list
                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};
