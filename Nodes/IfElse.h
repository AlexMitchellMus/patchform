#pragma once

#include "AudioNodeBase.h"
#include <functional>

class IfElse : public AudioNode {
    DEFINE_AND_REGISTER_NODE("IfElse", "ifelse", false);
    DEFINE_NODE_ALIASES("ifelse");

    FloatParameter* ifParam;
    ListParameter* modeParam;

    std::atomic<float> coldValueIf;
    std::atomic<int> modeHash;

    enum Mode {
        EQ = hash("=="),
        NEQ = hash("!="),
        GT = hash(">"),
        GTE = hash(">="),
        LT = hash("<"),
        LTE = hash("<="),
    };

    Mode stringToMode(const std::string& s) {
        switch (hash(s)) {
            case hash("!="): return NEQ;
            case hash(">"):  return GT;
            case hash(">="): return GTE;
            case hash("<"):  return LT;
            case hash("<="): return LTE;
            case hash("=="):
            default:         return EQ;
        }
    }

public:
    IfElse(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("input", AudioPort::PortType::Data);
        addOutputPort("else", AudioPort::PortType::Data);

        coldValueIf = objParams.value("if", 0.0f);
        std::string initialMode = objParams.value("mode", "==");

        modeParam = addParameter<ListParameter>("mode", std::vector<std::string>{ "==", "!=", ">", ">=", "<", "<=" }, initialMode);
        ifParam = addParameter<FloatParameter>("if", coldValueIf, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());

        modeHash.store(hash(initialMode));
        context->stringMap.intern("==", "!=", ">", ">=", "<", "<=");

        modeParam->informNodeOfChange = [this]() {
            modeHash.store(hash(modeParam->getValue()));
        };

        ifParam->informNodeOfChange = [this]() {
            coldValueIf.store(ifParam->getValue());
        };
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        Mode mode = static_cast<Mode>(modeHash.load());

        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (auto event : aEvents)
        {
            float value = event->getAtomValue(0);
            bool condition = false;

            float epsilon = std::numeric_limits<float>::epsilon();

            switch (mode)
            {
            case EQ:
                condition = fabs(value - coldValueIf) < epsilon;
                break;
            case NEQ:
                condition = fabs(value - coldValueIf) >= epsilon;
                break;
            case GT:
                condition = (value - coldValueIf) > epsilon;
                break;
            case GTE:
                condition = (value - coldValueIf) > -epsilon;
                break;
            case LT:
                condition = (coldValueIf - value) > epsilon;
                break;
            case LTE:
                condition = (coldValueIf - value) > -epsilon;
                break;
            }

            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->data = event->data;
                e->numAtoms = event->numAtoms;
                e->setTagHashcode(event->getTagHash());
                e->setTimeStamp(event->getTimeStamp());

                if (condition)
                    addEvent(1, e);
                else
                    addEvent(0, e);
            }
        }
    }

    json getSerializedNode() override
    {
        if (const auto* s = context->stringMap.find(modeHash.load()))
            nodeCreationData["mode"] = *s;

        nodeCreationData["if"] = coldValueIf.load();
        return nodeCreationData;
    }
};

REGISTER(IfElse);
