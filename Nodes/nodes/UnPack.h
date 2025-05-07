#pragma once

class UnPack final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Unpack", "unpack", false);
    DEFINE_NODE_ALIASES("unpack");

    int unpackNum = 0;
    std::vector<DataAtom*> outputAtoms;

public:
    UnPack(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        unpackNum = objParams.value("values", 0);
        outputAtoms.resize(unpackNum, nullptr);

        addInputPort("input", AudioPort::PortType::Data);
        for (int i = 0; i < unpackNum; i++) {
            addOutputPort("out_" + std::to_string(i), AudioPort::PortType::Data);
        }
    }

    json getSerializedNode() override {
        return nodeCreationData;
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override {
        const auto& events = inputPortBuffers[0]->getEvents();
        for (const auto* ev : events) {
            DataAtom* atom = ev->data;
            for (int i = 0; i < unpackNum && atom; i++) {
                outputAtoms[i] = context->eventPool.allocateDataAtom();
                if (outputAtoms[i]) {
                    outputAtoms[i]->copyFrom(atom);
                    outputAtoms[i]->next = nullptr;
                }
                atom = atom->next;
            }

            for (int i = 0; i < unpackNum; i++) {
                if (!outputAtoms[i]) continue;

                Event* outEv = context->eventPool.getFreeEvent();
                if (outEv) {
                    outEv->setTimeStamp(ev->getTimeStamp());
                    outEv->data = outputAtoms[i];
                    outEv->numAtoms = 1;
                    addEvent(i, outEv);
                }
            }
        }
    }
};

REGISTER(UnPack);
