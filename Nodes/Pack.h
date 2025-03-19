// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include "AudioNodeBase.h"
#include <vector>
#include <iostream>

class Pack : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Pack", "pack");

    int packNum = 0;
    std::vector<DataAtom*> persistentAtoms;

public:
    Pack(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        packNum = objParams.value("values", 0);

        for (int i = 0; i < packNum; i++)
        {
            addInputPort("data_" + std::to_string(i), AudioPort::PortType::Data);
        }

        persistentAtoms.resize(packNum, nullptr);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    DataAtom* cloneAtomChain(DataAtom* original, NodeContext* context, int& count)
    {
        if (!original) return nullptr;

        DataAtom* headClone = nullptr;
        DataAtom* lastClone = nullptr;

        std::vector<std::pair<DataAtom*, DataAtom**>> workQueue;
        workQueue.emplace_back(original, &headClone);

        while (!workQueue.empty())
        {
            auto [src, destPtr] = workQueue.back();
            workQueue.pop_back();

            if (!src)
            {
                *destPtr = nullptr;
                continue;
            }

            // Allocate new atom
            DataAtom* newAtom = context->eventPool.allocateDataAtom();
            if (!newAtom) return nullptr;

            newAtom->type = src->type;

            if (src->type == DataAtom::DataType::Float)
            {
                newAtom->data.atom = src->data.atom;
            }
            else if (src->type == DataAtom::DataType::List)
            {
                newAtom->data.list = nullptr;
                workQueue.emplace_back(src->data.list, &newAtom->data.list);
            }

            *destPtr = newAtom;
            lastClone = newAtom;
            count++;

            if (src->next)
            {
                workQueue.emplace_back(src->next, &newAtom->next);
            }
        }

        return headClone;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto freesize = context->eventPool.getFreeListSize();
        if (freelistSize != freesize)
        {
            std::cout << context->eventPool.getFreeListSize() << std::endl;
            freelistSize = freesize;
        }
        for (int portIndex = 0; portIndex < packNum; portIndex++)
        {
            const auto& events = inputPortBuffers[portIndex]->getEvents();

            for (const auto* ev : events)
            {
                unsigned long ts = ev->getTimeStamp();
                if (ts >= frameCount) continue;

                DataAtom* atom = ev->getAtom(0);
                if (!atom) continue;

                // **Replace old persistent atom**
                if (persistentAtoms[portIndex])
                {
                    context->eventPool.clearPersistentDataAtom(persistentAtoms[portIndex]);
                }

                persistentAtoms[portIndex] = atom;
                context->eventPool.makePersistent(atom);

                Event* newEvent = context->eventPool.getFreeEvent();
                if (!newEvent) return;

                // **Create a parent list atom**
                DataAtom* listAtom = context->eventPool.allocateDataAtom();
                if (!listAtom) return;

                listAtom->type = DataAtom::DataType::List;
                listAtom->data.list = nullptr;

                DataAtom* lastListAtom = nullptr;
                int atomCount = 0;

                // **Clone all persistent atoms and attach to the list**
                for (DataAtom* storedAtom : persistentAtoms)
                {
                    if (storedAtom)
                    {
                        DataAtom* clonedChain = cloneAtomChain(storedAtom, context, atomCount);
                        if (!clonedChain) return;

                        if (!listAtom->data.list)
                        {
                            listAtom->data.list = clonedChain;
                            lastListAtom = clonedChain;
                        }
                        else
                        {
                            lastListAtom->next = clonedChain;
                        }

                        while (lastListAtom->next)
                        {
                            lastListAtom = lastListAtom->next;
                        }
                    }
                }

                // **Ensure the event always contains a valid list atom**
                if (!listAtom->data.list)
                {
                    listAtom->data.list = context->eventPool.allocateDataAtom();
                    if (listAtom->data.list) listAtom->data.list->data.atom = 0.0f;
                }

                newEvent->setTimeStamp(ts);
                newEvent->data = listAtom;
                newEvent->numAtoms = atomCount;

//#define DEBUG_PACK
#ifdef DEBUG_PACK
                if (newEvent->data)
                {
                    std::cout << "Packed data: " << newEvent->data->toString() << " (Atoms: " << atomCount << ")" <<
                        std::endl;
                }
                else
                {
                    std::cout << "Packed data: (empty event)" << std::endl;
                }
#endif

                outputPort.addEvent(newEvent);
            }
        }
    }

    ~Pack() override
    {
        for (DataAtom* atom : persistentAtoms)
        {
            if (atom) context->eventPool.clearPersistentDataAtom(atom);
        }
    }

    int freelistSize = 0;
};
