// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include "AudioNodeBase.h"
#include <vector>
#include <iostream>
#include <algorithm> // For std::sort and std::unique

class Pack : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Pack", "pack");

    int packNum = 0;
    // One persistent DataAtom pointer per input port.
    std::vector<DataAtom*> persistentAtoms;

public:
    Pack(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        packNum = objParams.value("values", 0);

        persistentAtoms.clear();
        persistentAtoms.resize(packNum, nullptr);

        for (int i = 0; i < packNum; i++)
        {
            addInputPort("data_" + std::to_string(i), AudioPort::PortType::Data);
        }
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long /*frameCount*/) override
    {
//#define DEBUG_ATOM_POOL
#ifdef DEBUG_ATOM_POOL
        int listSize = context->eventPool.getFreeListSize();
        if (freelistSize != listSize)
        {
            freelistSize = listSize;
            std::cout << "atoms available: " << freelistSize << std::endl;
        }
#endif

        // 1. Collect all unique timestamps from all input ports.
        std::vector<unsigned long> timestamps;
        for (int i = 0; i < packNum; i++)
        {
            const auto& events = inputPortBuffers[i]->getEvents();
            for (const auto* ev : events)
            {
                timestamps.push_back(ev->getTimeStamp());
            }
        }
        if (timestamps.empty())
            return;
        std::sort(timestamps.begin(), timestamps.end());
        timestamps.erase(std::unique(timestamps.begin(), timestamps.end()), timestamps.end());

        // 2. For each unique timestamp, update persistent atoms and output an event.
        for (unsigned long t : timestamps)
        {
            // For each port, if an event at timestamp t is found, update that port's persistent atom.
            for (int i = 0; i < packNum; i++)
            {
                const auto& events = inputPortBuffers[i]->getEvents();
                bool eventFound = false;
                for (const auto* ev : events)
                {
                    if (ev->getTimeStamp() == t)
                    {
                        DataAtom* inAtom = ev->getAtom(0);
                        if (inAtom)
                        {
                            // Mark the old persistent atom as no longer persistent.
                            if (persistentAtoms[i])
                                persistentAtoms[i]->makePersistent(false);

                            inAtom->makePersistent(true);
                            persistentAtoms[i] = inAtom;
                        }
                        break; // Use only the first event at this timestamp.
                    }
                }
            }

            // 3. Remake fresh output atoms from each persistent atom.
            std::vector<DataAtom*> outputAtoms;
            for (int i = 0; i < packNum; i++)
            {
                if (!persistentAtoms[i])
                {
                    // Allocate an empty atom if missing.
                    DataAtom* emptyAtom = context->eventPool.allocateDataAtom();
                    if (emptyAtom)
                    {
                        outputAtoms.push_back(emptyAtom);
                    }
                    continue;
                }
                DataAtom* outAtom = context->eventPool.allocateDataAtom();
                if (outAtom)
                {
                    if (persistentAtoms[i]->type == DataAtom::DataType::List)
                    {
                        outAtom->type = DataAtom::DataType::List;
                        outAtom->data.list = persistentAtoms[i]->data.list;
                    }
                    else
                    {
                        outAtom->type = DataAtom::DataType::Float;
                        outAtom->data.atom = persistentAtoms[i]->data.atom;
                    }
                    outAtom->next = nullptr;
                    outputAtoms.push_back(outAtom);
                }
            }

            // 4. Chain the output atoms into a parent list.
            DataAtom* parentList = context->eventPool.allocateDataAtom();
            if (!parentList)
                continue;
            parentList->type = DataAtom::DataType::List;
            parentList->data.list = nullptr;
            DataAtom* last = nullptr;
            for (DataAtom* atom : outputAtoms)
            {
                if (!parentList->data.list)
                {
                    parentList->data.list = atom;
                    last = atom;
                }
                else
                {
                    last->next = atom;
                    last = atom;
                }
            }
            if (!parentList->data.list)
            {
                DataAtom* fallback = context->eventPool.allocateDataAtom();
                if (fallback)
                {
                    parentList->data.list = fallback;
                }
            }

            // 5. Create and output a new event with the freshly remade data.
            Event* newEvent = context->eventPool.getFreeEvent();
            if (!newEvent)
                continue;
            newEvent->setTimeStamp(t);
            newEvent->data = parentList;
            newEvent->numAtoms = packNum;
            outputPort.addEvent(newEvent);

//#define DEBUG_PACK
#ifdef DEBUG_PACK
            if (newEvent->data)
            {
                std::cout << "Packed data: " << newEvent->data->toString()
                          << " (Atoms: " << newEvent->numAtoms << ")" << std::endl;
            }
            else
            {
                std::cout << "Packed data: (empty event)" << std::endl;
            }
#endif
        }
    }

    ~Pack() override
    {
        for (auto* atom : persistentAtoms)
        {
            // return the saved atoms from the pack object to the atom pool
            if (atom)
                atom->makePersistent(false);
        }
    }

private:
    int freelistSize = 0;
};
