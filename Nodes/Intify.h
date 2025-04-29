/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class Intify : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Intify", "intify", false);
    DEFINE_NODE_ALIASES("intify");

    enum Mode
    {
        ROUND = hash("round"),
        FLOOR = hash("floor"),
        CEIL  = hash("ceil"),
        TRUNC = hash("trunc")  // Removes fractional part, moves towards zero (-2.9 = -2)
    };
    std::atomic<Mode> mode;

    ListParameter* modeParam;

    Mode stringToMode(const std::string& modeString)
    {
        switch (hash(modeString))
        {
        case FLOOR:             return FLOOR;
        case CEIL:              return CEIL;
        case TRUNC:             return TRUNC;
        case ROUND: default:    return ROUND;
        }
    }

public:
    Intify(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("in", AudioPort::PortType::Data);

        std::string initialMode = "round";
        if (objParams.contains("mode") && objParams["mode"].is_string()) {
            initialMode = objParams["mode"];
        }

        mode.store(stringToMode(initialMode));

        modeParam = addParameter<ListParameter>("mode", std::vector<std::string>{ "round", "floor", "ceil", "trunc"}, initialMode);

        modeParam->informNodeOfChange = [this]() {
            mode.store(stringToMode(modeParam->getValue()));
        };

        context->stringMap.intern("round", "floor", "ceil", "trunc");
    }

    json getSerializedNode() override
    {
        if (const auto* s = context->stringMap.find(mode.load()))
            nodeCreationData["mode"] = *s;
        return nodeCreationData;
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        auto inEvents = inputPortBuffers[0]->getEvents();

        for (const auto event : inEvents)
        {
            if (!event->data)
                continue;

            DataAtom* atom = event->data;
            if (atom->type != DataAtom::DataType::Float)
                continue;

            float val = atom->data.atom;
            int result = 0;

            switch (mode.load())
            {
                case FLOOR:             result = static_cast<int>(std::floor(val)); break;
                case CEIL:              result = static_cast<int>(std::ceil(val)); break;
                case TRUNC:             result = static_cast<int>(std::trunc(val)); break;
                case ROUND: default:    result = static_cast<int>(std::round(val)); break;
            }

            if (Event* e = context->eventPool.getFreeEvent())
            {
                DataAtom* outData = context->eventPool.allocateDataAtom();
                outData->data.atom = static_cast<float>(result); // still float
                e->data = outData;
                e->numAtoms = 1;
                e->setTimeStamp(event->getTimeStamp());
                addEvent(0, e);
            }
        }
    }
};

REGISTER(Intify);
