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

    IntParameter* atomNumberParam;
    std::atomic<size_t> atomNumber;

    IntParameter* routeModeParam;
    std::atomic<int> routeMode;

    DataAtom* savedData = nullptr;

public:
    Get(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port

        atomNumber.store(objParams.value("get", 0));
        routeMode.store(objParams.value("routeMode", 0));

        atomNumberParam = addParameter<IntParameter>("get", atomNumber, 0, 1024);
        routeModeParam = addParameter<IntParameter>("route", routeMode, 0, 1);

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

    void processAudio(float* out, unsigned long frameCount) override
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
                        newData->data = atomData->data;
                        newData->type = atomData->type;
                        e->numAtoms = 1;
                        e->data = newData;
                    }
                }
                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};
