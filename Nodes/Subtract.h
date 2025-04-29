#pragma once

class Subtract : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Subtract", "sub", false);
    DEFINE_NODE_ALIASES("sub", "subtract");

    std::atomic<float> latestA = 0.0f;
    std::atomic<float> latestB = 0.0f;
    std::atomic<int> mode = 0;

    ListParameter* modeParam = nullptr;
    FloatParameter* defaultAParam = nullptr;
    FloatParameter* defaultBParam = nullptr;

public:
    explicit Subtract(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data);
        addInputPort("B", AudioPort::PortType::Data);

        // FIXME we want to store the hot mode as a string in the json!
        mode.store(objParams.value("hot", 1));
        latestA.store(objParams.value("defaultA", 0.0f));
        latestB.store(objParams.value("defaultB", 0.0f));

        auto modeToInt = [](const std::string& mode)
        {
            switch (hash(mode))
            {
            default:
            case hash("A"):
                return 0;
            case hash("B"):
                return 1;
            case hash("A+B"):
                return 2;
            }
        };

        auto intToMode = [](int mode) -> std::string
        {
            switch (mode)
            {
            case 0: return "A";
            case 1: return "B";
            case 2: return "A+B";
            default: return "A";
            }
        };

        modeParam = addParameter<ListParameter>("hot", std::vector<std::string>{ "A", "B", "A+B" }, intToMode(mode));
        defaultAParam = addParameter<FloatParameter>("defaultA", 0.0f, -1000.0f, 1000.0f);
        defaultBParam = addParameter<FloatParameter>("defaultB", 0.0f, -1000.0f, 1000.0f);

        defaultAParam->informNodeOfChange = [this]() {
            latestA.store(defaultAParam->getValue());
        };
        defaultBParam->informNodeOfChange = [this]() {
            latestB.store(defaultBParam->getValue());
        };
        modeParam->informNodeOfChange = [this, modeToInt]() {
            mode.store(modeToInt(modeParam->getValue()));
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["hot"] = mode.load();
        nodeCreationData["defaultA"] = latestA.load();
        nodeCreationData["defaultB"] = latestB.load();
        return nodeCreationData;
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();
        const auto& bEvents = inputPortBuffers[1]->getEvents();

        switch (mode.load())
        {
            case 0: // A hot
            {
                size_t bIndex = 0;
                float bVal = latestB.load();

                for (auto* e : aEvents) {
                    float aVal = e->getAtomValue(0);
                    latestA.store(aVal);
                    uint32_t t = e->getTimeStamp();

                    while (bIndex < bEvents.size() && bEvents[bIndex]->getTimeStamp() <= t)
                        bVal = bEvents[bIndex++]->getAtomValue(0);

                    latestB.store(bVal);

                    if (auto out = context->eventPool.getFreeEvent()) {
                        out->setTimeStamp(t);
                        context->eventPool.addDataAtomTo(out, aVal - bVal);
                        addEvent(0, out);
                    }
                }

                // Store the cold B event, even if there are no events from A
                for (auto* e : bEvents)
                    latestB.store(e->getAtomValue(0));

                break;
            }

            case 1: // B hot
            {
                size_t aIndex = 0;
                float aVal = latestA.load();

                for (auto* e : bEvents) {
                    float bVal = e->getAtomValue(0);
                    latestB.store(bVal);
                    uint32_t t = e->getTimeStamp();

                    while (aIndex < aEvents.size() && aEvents[aIndex]->getTimeStamp() <= t)
                        aVal = aEvents[aIndex++]->getAtomValue(0);

                    latestA.store(aVal);

                    if (auto out = context->eventPool.getFreeEvent()) {
                        out->setTimeStamp(t);
                        context->eventPool.addDataAtomTo(out, aVal - bVal);
                        addEvent(0, out);
                    }
                }

                // Store the cold A event, even if there are no events from B
                for (auto* e : aEvents)
                    latestA.store(e->getAtomValue(0));

                break;
            }

            case 2: // A+B hot
            {
                // A triggers
                size_t bIndex = 0;
                float bVal = latestB.load();

                for (auto* e : aEvents) {
                    float aVal = e->getAtomValue(0);
                    latestA.store(aVal);
                    uint32_t t = e->getTimeStamp();

                    while (bIndex < bEvents.size() && bEvents[bIndex]->getTimeStamp() <= t)
                        bVal = bEvents[bIndex++]->getAtomValue(0);

                    latestB.store(bVal);

                    if (auto out = context->eventPool.getFreeEvent()) {
                        out->setTimeStamp(t);
                        context->eventPool.addDataAtomTo(out, aVal - bVal);
                        addEvent(0, out);
                    }
                }

                // B triggers
                size_t aIndex = 0;
                float aVal = latestA.load();

                for (auto* e : bEvents) {
                    float bVal = e->getAtomValue(0);
                    latestB.store(bVal);
                    uint32_t t = e->getTimeStamp();

                    while (aIndex < aEvents.size() && aEvents[aIndex]->getTimeStamp() <= t)
                        aVal = aEvents[aIndex++]->getAtomValue(0);

                    latestA.store(aVal);

                    if (auto out = context->eventPool.getFreeEvent()) {
                        out->setTimeStamp(t);
                        context->eventPool.addDataAtomTo(out, aVal - bVal);
                        addEvent(0, out);
                    }
                }
                break;
            }
        }
    }
};

REGISTER(Subtract);
