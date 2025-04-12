#pragma once

#include "AudioNodeBase.h"

class Multiply : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Multiply", "mul", false);
    DEFINE_NODE_ALIASES("mul");

    float coldValue;

public:
    explicit Multiply(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot
        addInputPort("B", AudioPort::PortType::Data); // cold
        coldValue = objParams.value("value", 1.0f);
    }

    void processAudio(const float* in, float*, unsigned long, std::vector<MidiMessage>& midiMessage) override {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        if (auto bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
            coldValue = bEvent.back()->getAtomValue(0);

        for (auto event : aEvents) {
            if (Event* e = context->eventPool.getFreeEvent()) {
                e->setTimeStamp(event->getTimeStamp());
                context->eventPool.addDataAtomTo(e, event->getAtomValue(0) * coldValue);
                addEvent(0, e);
            }
        }
    }
};

REGISTER(Multiply);
