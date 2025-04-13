#pragma once

#include "AudioNodeBase.h"

class Select : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Select", "sel", false);
    DEFINE_NODE_ALIASES("sel", "select");

public:
    Select(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        addInputPort("input", AudioPort::PortType::Data);

        int outputs = objParams.value("outputs", 2);
        for (int i = 0; i < outputs; ++i) {
            addOutputPort(std::to_string(i), AudioPort::PortType::Data);
        }
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (auto event : aEvents)
        {
            int value = static_cast<int>(event->getAtomValue(0));

            if (value >= 0 && value < (int)outputPortBuffers.size()) {
                if (Event* e = context->eventPool.getFreeEvent())
                {
                    e->data = event->data;
                    e->numAtoms = event->numAtoms;
                    e->setTimeStamp(event->getTimeStamp());
                    addEvent(value, e);
                }
            }
        }
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }
};

REGISTER(Select);
