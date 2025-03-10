/*
 // Copyright (c) 2024-2025 Alex Mitchell
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>
#include <vector>
#include <tuple>
#include <algorithm>

class Pack : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Pack", "pack");

    int packValues = 0;
    // Stores the last known value for each input port.
    std::vector<float> values;

public:
    Pack(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        packValues = objParams.value("values", 0);
        // Create one input port per expected value.
        for (int i = 0; i < packValues; i++)
        {
            addInputPort("data_" + std::to_string(i), AudioPort::PortType::Data);
        }
        values.assign(packValues, 0.0f);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        // Accumulate events from all ports as (timestamp, portIndex, value)
        std::vector<std::tuple<unsigned long, int, float>> combinedEvents;

        for (int portIndex = 0; portIndex < packValues; portIndex++)
        {
            const auto& events = inputPortBuffers[portIndex]->getEvents();
            for (const auto* ev : events)
            {
                unsigned long ts = ev->getTimeStamp();
                if (ts < frameCount)
                {
                    float value = ev->getAtomValue(0);
                    combinedEvents.emplace_back(ts, portIndex, value);
                }
            }
        }

        // If no events were received, nothing to output.
        if (combinedEvents.empty())
            return;

        // Sort the events by timestamp.
        std::sort(combinedEvents.begin(), combinedEvents.end(),
            [](const auto& a, const auto& b)
            {
                return std::get<0>(a) < std::get<0>(b);
            });

        // Start with the current state (last known values)
        std::vector<float> currentValues = values;

        // Iterate over the sorted events, grouping by timestamp.
        size_t i = 0;
        while (i < combinedEvents.size())
        {
            unsigned long ts = std::get<0>(combinedEvents[i]);
            // Process all events with the same timestamp.
            while (i < combinedEvents.size() && std::get<0>(combinedEvents[i]) == ts)
            {
                int portIndex = std::get<1>(combinedEvents[i]);
                float value = std::get<2>(combinedEvents[i]);
                currentValues[portIndex] = value;
                ++i;
            }
            // Output a merged event with the updated currentValues.
            if (auto* e = context->eventPool.getFreeEvent())
            {
                for (float val : currentValues)
                    e->addAtom(val);

                e->setTimeStamp(ts);
                outputPort.addEvent(e);
            }
        }

        // Update stored state for the next cycle.
        values = currentValues;
    }
};
