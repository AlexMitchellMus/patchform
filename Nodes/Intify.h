// Intify.h

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class Intify : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Intify", "intify");

    enum Mode { ROUND = 0, FLOOR = 1, CEIL = 2, TRUNC = 3 };
    std::atomic<int> mode;

    IntParameter* modeParam;

public:
    Intify(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("in", AudioPort::PortType::Data);

        int initialMode = objParams.value("mode", 0);
        mode.store(initialMode);

        modeParam = addParameter<IntParameter>("mode", mode, 0, 3);

        modeParam->informNodeOfChange = [this]() {
            mode.store(modeParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["mode"] = modeParam->getValue();
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
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
                case FLOOR: result = static_cast<int>(std::floor(val)); break;
                case CEIL:  result = static_cast<int>(std::ceil(val)); break;
                case TRUNC: result = static_cast<int>(std::trunc(val)); break;
                case ROUND: default: result = static_cast<int>(std::round(val)); break;
            }

            if (Event* e = context->eventPool.getFreeEvent())
            {
                DataAtom* outData = context->eventPool.allocateDataAtom();
                outData->data.atom = static_cast<float>(result); // still float
                e->data = outData;
                e->numAtoms = 1;
                e->setTimeStamp(event->getTimeStamp());
                outputPort.addEvent(e);
            }
        }
    }
};
