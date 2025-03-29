#pragma once

#include "AudioNodeBase.h"

class Multiply : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Multiply", "mul");

    float coldValue;

public:
    explicit Multiply(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot
        addInputPort("B", AudioPort::PortType::Data); // cold
        coldValue = objParams.value("value", 1.0f);
    }

    bool shouldProcess(unsigned int) override {
        return hasInputEvents.load(std::memory_order_relaxed);
    }

    void processAudio(float*, unsigned long) override {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        if (auto bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
            coldValue = bEvent.back()->getAtomValue(0);

        for (auto event : aEvents) {
            if (Event* e = context->eventPool.getFreeEvent()) {
                e->setTimeStamp(event->getTimeStamp());
                e->addAtom(event->getAtomValue(0) * coldValue);
                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};
