/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Get : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("GetValue", "get", false);
    DEFINE_NODE_ALIASES("get");

    IntParameter* atomNumberParam;
    std::atomic<size_t> atomNumber;

    BoolParameter* routeModeParam;
    std::atomic<bool> routeMode;

    DataAtom* savedData = nullptr;

public:
    Get(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port

        atomNumber.store(objParams.value("get", 0));
        routeMode.store(JsonHelpers::getBoolOrIntFallback(objParams, "routeMode", false));

        atomNumberParam = addParameter<IntParameter>("get", atomNumber, 0, 1024);
        routeModeParam = addParameter<BoolParameter>("route", routeMode);

        atomNumberParam->informNodeOfChange = [this]()
        {
            atomNumber.store(atomNumberParam->getValue());
        };

        routeModeParam->informNodeOfChange = [this]()
        {
            routeMode.store(routeModeParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["get"] = atomNumberParam->getValue();
        nodeCreationData["routeMode"] = routeModeParam->getValue();
        return nodeCreationData;
    }

    void cleanupAudio() override
    {

        context->makeDataPersistent(savedData, false, nodeID);
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (const auto event : aEvents)
        {
            switch (event->getTagHash())
            {
            case hash("get"):
                {
                    atomNumber.store(event->getAtomValue(0));
                    if (routeMode.load() != 0)
                        continue;
                    break;
                }
            default:
                {
                    if (savedData)
                        context->makeDataPersistent(savedData, false, nodeID);

                    savedData = event->data;
                    context->makeDataPersistent(savedData, true, nodeID);
                    if (routeMode.load() == 0)
                        continue;
                }
            }

            int getAtomNumber = std::min(atomNumber.load(), savedData ? savedData->getAtomCount() - 1 : 0);

            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());
                if (savedData)
                {
                    auto atomData = savedData->getAtom(getAtomNumber);
                    if (atomData->type == DataAtom::DataType::List)
                    {
                        e->data = atomData->data.list;
                        auto walk = atomData->data.list;
                        int numAtoms = 0;
                        while (walk)
                        {
                            numAtoms++;
                            walk = walk->next;
                        }
                        e->numAtoms = numAtoms;
                    }
                    else
                    {
                        // Make a new atom to hold the list or data
                        auto newData = context->eventPool.allocateDataAtom();
                        newData->copyFrom(atomData);
                        e->numAtoms = 1;
                        e->data = newData;
                    }
                }
                addEvent(0, e);
            }
        }
    }
};

REGISTER(Get);
