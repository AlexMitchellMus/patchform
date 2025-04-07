/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class If : public AudioNode {
    DEFINE_AND_REGISTER_NODE("If", "if", false);

    IntParameter* ifParam;
    FloatParameter* retParam;

    int coldValueIf;
    float coldValueReturn;

public:
    If(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port
        addInputPort("B", AudioPort::PortType::Data); // cold port
        addInputPort("C", AudioPort::PortType::Data); // cold port

        coldValueIf = objParams.value("if", 0.0f);
        coldValueReturn = objParams.value("return", 0.0f);

        ifParam = addParameter<IntParameter>("if", coldValueIf, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
        retParam = addParameter<FloatParameter>("return", coldValueReturn, std::numeric_limits<float>::min(), std::numeric_limits<float>::max());

        ifParam->informNodeOfChange = [this]()
        {
            coldValueIf = ifParam->getValue();
        };

        retParam->informNodeOfChange = [this]()
        {
            coldValueReturn = retParam->getValue();
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["if"] = coldValueIf;
        nodeCreationData["return"] = coldValueReturn;
        return nodeCreationData;
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();
        if (auto& bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
            coldValueIf = bEvent.back()->getAtomValue(0);

        for (auto event : aEvents)
        {
            if (event->getAtomValue(0) == coldValueIf)
            {
                if (Event* e = context->eventPool.getFreeEvent())
                {
                    e->setTimeStamp(event->getTimeStamp());
                    context->eventPool.addDataAtomTo(e, coldValueReturn);

                    // Now add it to the output port’s event list
                    addEvent(0, e);
                }
            }
        }
    }
};
