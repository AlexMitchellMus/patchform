#pragma once

#include "AudioNodeBase.h"


// DivideNode that divides A by B
class Zerox final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Zerox", "zerox", true);

public:
    explicit Zerox(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("audioin", AudioPort::PortType::Signal);
    }

    void processAudio(const float*, float*, unsigned long frames, std::vector<MidiMessage>&) override
    {
        const float* buf = inputPortBuffers[0]->getAudioBuffer();

        int count = 0;
        unsigned long lastIndex = 0;
        bool found = false;

        float prev = buf[0];
        for (unsigned long i = 1; i < frames; ++i) {
            float curr = buf[i];
            if ((prev < 0.f && curr >= 0.f) || (prev > 0.f && curr <= 0.f)) {
                lastIndex = i;
                count++;
                found = true;
            }
            prev = curr;
        }

        if (found) {
            if (Event* outEvent = context->eventPool.getFreeEvent()) {
                outEvent->setTimeStamp(lastIndex);
                context->eventPool.addDataAtomTo(outEvent, count);
                outputPortBuffers[0]->addEvent(outEvent);
            }
        }
    }
};