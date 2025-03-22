// EventDelay.h

#pragma once

#include "AudioNodeBase.h"

class EventDelay : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("EventDelay", "eventdelay");

    FloatParameter* delayParam;
    std::atomic<float> delayTimeMs;

    struct DelayedEvent {
        float remainingTime;
        uint64_t timestamp;
        DataAtom* data;
        hash32 eventHash;
        int numAtoms;
    };

    std::vector<DelayedEvent> queue;

public:
    EventDelay(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("in", AudioPort::PortType::Data);

        delayTimeMs.store(objParams.value("ms", 100.0f));

        // Max delay time 1 min for now
        delayParam = addParameter<FloatParameter>("delay", delayTimeMs, 0.0f, 60000.0f);

        delayParam->informNodeOfChange = [this]() {
            delayTimeMs.store(delayParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["ms"] = delayParam->getValue();
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        float sampleRate = context->sampleRate;
        float dt = 1000.0f * frameCount / sampleRate;

        // Update queue
        for (auto it = queue.begin(); it != queue.end();)
        {
            it->remainingTime -= dt;
            if (it->remainingTime <= 0.0f)
            {
                if (auto ev = context->eventPool.getFreeEvent())
                {
                    it->data->makePersistent(false);
                    ev->data = it->data;
                    ev->setTagHashcode(it->eventHash);
                    uint64_t delaySamples = delayTimeMs.load() * sampleRate * 0.001f;
                    int delayedTime = static_cast<int>(it->timestamp + delaySamples);
                    if (delayedTime >= static_cast<int>(frameCount))
                        delayedTime = frameCount - 1;

                    ev->setTimeStamp(delayedTime);
                    ev->numAtoms = it->numAtoms;
                    outputPort.addEvent(ev);
                }
                it = queue.erase(it);
            }
            else ++it;
        }

        // Enqueue new events
        auto events = inputPortBuffers[0]->getEvents();
        for (const auto event : events)
        {
            event->data->makePersistent(true);
            queue.push_back({ delayTimeMs.load(), event->getTimeStamp(), event->data, event->getTagHash(), event->numAtoms });
        }
    }
};
