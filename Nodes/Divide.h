#pragma once

#include "AudioNodeBase.h"


// DivideNode that divides A by B
class Divide : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Divide", "div", false);
    DEFINE_NODE_ALIASES("divide", "div");

    float coldValue;

public:
    explicit Divide(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot
        addInputPort("B", AudioPort::PortType::Data); // cold
        coldValue = objParams.value("value", 1.0f);
    }

    void processAudio(const float* in, float*, unsigned long, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        if (auto bEvent = inputPortBuffers[1]->getEvents(); bEvent.size())
            coldValue = bEvent.back()->getAtomValue(0);

        // Avoid division by zero
        float safeDivisor = (coldValue == 0.0f) ? 1.0f : coldValue;

        for (auto event : aEvents) {
            if (Event* e = context->eventPool.getFreeEvent()) {
                e->setTimeStamp(event->getTimeStamp());
                context->eventPool.addDataAtomTo(e, event->getAtomValue(0) / safeDivisor);
                addEvent(0, e);
            }
        }
    }
};

REGISTER(Divide);