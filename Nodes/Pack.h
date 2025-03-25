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
    std::vector<DataAtom*> outputAtoms;

public:
    Pack(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        packNum = objParams.value("values", 0);

        persistentAtoms.resize(packNum, nullptr);
        outputAtoms.resize(packNum, nullptr);

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

            // Make disposable atoms to hold data / list heads
            for (int i = 0; i < packNum; i++)
            {
                if (!persistentAtoms[i])
                {
                    // Allocate an empty atom if missing.
                    DataAtom* emptyAtom = context->eventPool.allocateDataAtom();
                    if (emptyAtom)
                    {
                        emptyAtom->next = nullptr;
                        outputAtoms[i] = emptyAtom;
                    }
                    continue;
                }
                DataAtom* outAtom = context->eventPool.allocateDataAtom();
                if (outAtom)
                {
                    if (persistentAtoms[i]->next != nullptr)
                    {
                        outAtom->type = DataAtom::DataType::List;
                        outAtom->data.list = persistentAtoms[i];
                    }
                    else
                    {
                        outAtom->type = DataAtom::DataType::Float;
                        outAtom->data.atom = persistentAtoms[i]->data.atom;
                    }
                    outAtom->next = nullptr;
                    outputAtoms[i] = outAtom;
                }
            }

            if (!outputAtoms.empty())
            {
                for (size_t i = 0; i < outputAtoms.size() - 1; i++)
                {
                    outputAtoms[i]->next = outputAtoms[i + 1];
                }
                // Create and output a new event using the first atom in the chain.
                Event* newEvent = context->eventPool.getFreeEvent();
                if (newEvent)
                {
                    newEvent->setTimeStamp(t);
                    newEvent->data = outputAtoms.front();
                    newEvent->numAtoms = static_cast<int>(outputAtoms.size());
                    outputPortBuffers[0]->addEvent(newEvent);
                }

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
