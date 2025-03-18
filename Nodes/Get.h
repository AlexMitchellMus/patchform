/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Get : public AudioNode {
    DEFINE_AND_REGISTER_NODE("GetValue", "get");

    IntParameter *atomNumberParam;
    int atomNumber;

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
            if (event->getNumAtoms() > atomNumber)
            {
                if (Event* e = context->eventPool.getFreeEvent()){
                    e->setTimeStamp(event->getTimeStamp());
                    e->data = event->getAtom(atomNumber);
                    if (e->data->type == DataAtom::DataType::List)
                    {
                        auto walk = e->data;
                        int numAtoms = 0;
                        while (walk)
                        {
                            numAtoms++;
                            walk = walk->next;
                        }
                        e->numAtoms = numAtoms;
                    }
                    else
                        e->addAtom(event->getAtomValue(atomNumber));

                    outputPort.addEvent(e);
                }
            }
        }
    }
};