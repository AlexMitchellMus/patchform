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
    DEFINE_AND_REGISTER_NODE("GetValue", "get");

    IntParameter* atomNumberParam;
    size_t atomNumber;

    DataAtom* savedData = nullptr;

public:
    Get(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port

        atomNumber = objParams.value("get", 0);

        atomNumberParam = addParameter<IntParameter>("Get atom: ", atomNumber, 0, 1024);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPortBuffers[0]->getEvents();
        atomNumber = atomNumberParam->getValue();

        for (const auto event : aEvents)
        {
            switch (event->getTagHash())
            {
                case hash("get"):
                    {
                        atomNumber = event->getAtomValue(0);
                    }
                break;
                default:
                    if (event->data)
                    {
                        if (savedData)
                            savedData->makePersistent(false);

                        savedData = event->data;
                        savedData->makePersistent(true);
                        continue;
                    }
                break;
            }

            if (!savedData)
                return;

            int getAtomNumber = std::min(atomNumber, savedData->getAtomCount() - 1);

            if (Event* e = context->eventPool.getFreeEvent())
            {
                e->setTimeStamp(event->getTimeStamp());
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
                    newData->data.atom = atomData->data.atom;
                    e->numAtoms = 1;
                    e->data = newData;
                }
                outputPort.addEvent(e);
            }
        }
    }
};
