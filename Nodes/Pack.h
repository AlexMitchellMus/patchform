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
    // Stores **direct** references to persistent data atoms.
    std::vector<DataAtom*> persistentAtoms;

public:
    Pack(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        packNum = objParams.value("values", 0);

        // Create one input port per expected value.
        for (int i = 0; i < packNum; i++)
        {
            addInputPort("data_" + std::to_string(i), AudioPort::PortType::Data);
        }

        // Allocate persistent data atom pointers for each input
        persistentAtoms.resize(packNum, nullptr);
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    // Clone a full DataAtom chain and count the number of atoms
    DataAtom* cloneAtomChain(DataAtom* original, NodeContext* context, int& count)
    {
        if (!original) return nullptr;

        DataAtom* firstClone = context->eventPool.allocateDataAtom();
        if (!firstClone) return nullptr; // Out of memory

        firstClone->atom = original->atom;
        firstClone->next = cloneAtomChain(original->next, context, count); // Recursively clone next atom
        count++; // Count atoms in the new event

        return firstClone;
    }

    // Pack node: Process events **without cloning atoms on input**
    void processAudio(float* out, unsigned long frameCount) override
    {
        for (int portIndex = 0; portIndex < packNum; portIndex++)
        {
            const auto& events = inputPortBuffers[portIndex]->getEvents();

            for (const auto* ev : events)
            {
                unsigned long ts = ev->getTimeStamp();
                if (ts >= frameCount) continue; // Ignore future events

                DataAtom* atom = ev->getAtom(0);

                // **Directly store the original atom reference (no cloning yet)**
                persistentAtoms[portIndex] = atom;
                context->eventPool.makePersistent(atom);

                // Create new event
                Event* newEvent = context->eventPool.getFreeEvent();
                if (!newEvent) return; // No available events, exit early

                // **Clone all persistent atoms when creating the final packed event**
                DataAtom* firstAtom = nullptr;
                DataAtom* lastAtom = nullptr;
                int atomCount = 0;

                for (DataAtom* storedAtom : persistentAtoms)
                {
                    if (storedAtom)
                    {
                        DataAtom* clonedChain = cloneAtomChain(storedAtom, context, atomCount);
                        if (!clonedChain) return; // Out of memory

                        if (!firstAtom)
                        {
                            firstAtom = clonedChain;
                            lastAtom = clonedChain;
                        }
                        else
                        {
                            lastAtom->next = clonedChain;
                        }

                        // Move lastAtom to the end of the cloned chain
                        while (lastAtom->next)
                        {
                            lastAtom = lastAtom->next;
                        }
                    }
                }

                // Assign cloned atoms to new event
                newEvent->setTimeStamp(ts);
                newEvent->data = firstAtom;
                newEvent->numAtoms = atomCount;

//#define DEBUG_PACK
#ifdef DEBUG_PACK
                if (newEvent->data) {
                    std::cout << "Packed data: " << newEvent->data->toString() << " (Atoms: " << atomCount << ")" << std::endl;
                } else {
                    std::cout << "Packed data: (empty event)" << std::endl;
                }
#endif

                outputPort.addEvent(newEvent);
            }
        }
    }

    ~Pack() override
    {
        // Ensure persistent atoms are cleared when the node is destroyed
        for (DataAtom* atom : persistentAtoms)
        {
            if (atom) context->eventPool.clearPersistentDataAtom(atom);
        }
    }
};
