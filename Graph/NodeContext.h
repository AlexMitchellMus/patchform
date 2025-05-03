// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include "readerwriterqueue.h"

#include <iostream>
#include <vector>
#include <numeric>

#include "Event.h"
#include "../Utility/LockFreeHashMap.h"
#include "PersistentAtomFlags.h"
#include "DataAtom.h"
#include "unordered_dense.h"

class EventPool {
public:
    EventPool() {
        growAtomPool(1 << 17);  // 131072 atoms
        growPool(1 << 14);      // 16384 events
    }

    int eventPoolSize() {
        return events.size();
    }

    // Acquire a free Event (returns nullptr if none available)
    Event* getFreeEvent() {
        if (freestackPos == 0) {
            std::cout << "No free events left!" << std::endl;
            return nullptr;
        }

        auto eventIndex = freeStack[--freestackPos];
        Event* evt = &events[eventIndex];
        reset(evt);
        return evt;
    }

    // Resets the linked list pointers and the used count.
    // The atomPool remains allocated, so the DataAtoms can be reused.
    void reset(Event* event)
    {
        event->data = nullptr;
        event->tail = nullptr;
        event->setTag("");
        event->setTimeStamp(0);
        event->numAtoms = 0;
    }

    void releaseAllEvents() {
        freestackPos = freestackSize;
        resetFreeList();
    }

    void addDataAtomTo(Event* evnt, const float value)
    {
        DataAtom* newAtom = allocateDataAtom();
        if (!newAtom) {
            throw std::runtime_error("Atom pool exhausted.");
        }

        newAtom->type = DataAtom::DataType::Float;
        newAtom->data.atom = value;
        newAtom->next = nullptr;

        if (evnt->data == nullptr) {
            evnt->data = newAtom;
            evnt->tail = newAtom;
            evnt->numAtoms = 1;
        } else {
            evnt->tail->next = newAtom;
            evnt->tail = newAtom;
            ++(evnt->numAtoms);
        }
    }

    // Grow the event pool
    void growPool(std::size_t count) {
        auto oldSize = events.size();
        auto newSize = oldSize + count;

        events.resize(newSize);

        freeStack.resize(newSize);
        std::iota(freeStack.begin(), freeStack.end(), 0);

        freestackPos = newSize;
        freestackSize = newSize;
    }

    // Grow the atom pool
    void growAtomPool(std::size_t count) {
        sharedAtomPool.resize(count);

        freeList.reserve(count);
        allocatedList.reserve(count);

        initializePersistentAtomFlags();

        for (size_t i = 0; i < count; ++i) {
            freeList.push_back(i);
        }
    }

    void initializePersistentAtomFlags()
    {
        persistentAtoms.assign((sharedAtomPool.size() + 63) / 64, 0);

        // Bind bitfield to persistentFlags wrapper
        persistentAtomsFlags.bind(persistentAtoms);
    }

    // Fast allocation: Pull from freeList and move to allocatedList
    DataAtom* allocateDataAtom() {
        if (freeList.empty()) {
            std::cout << "No free atoms left!" << std::endl;
            return nullptr; // No available datatoms
        }

        size_t index = freeList.back();
        freeList.pop_back();
        allocatedList.push_back(index);
        const auto atom = &sharedAtomPool[index];

        assert(!persistentAtomsFlags.isSet(index) && "Allocating atom that's still marked persistent!");

        atom->type = DataAtom::DataType::Float;
        atom->data.atom = 0.0f;
        atom->next = nullptr;
        return atom;
    }

    int getFreeListSize()
    {
        return freeList.size();
    }

    void resetFreeList()
    {
        size_t writePos = 0;

        for (unsigned long long index : allocatedList)
        {
            if (!persistentAtomsFlags.isSet(index))
            {
                // Atom is no longer persistent — reclaim it
                freeList.push_back(index);
            }
            else
            {
                // Keep in allocatedList
                allocatedList[writePos++] = index;
            }
        }

        // Shrink the logical size — no actual memory freed
        allocatedList.resize(writePos);
    }

    PersistentAtomFlags& getPersistentFlags()
    {
        persistentAtomsFlags.bind(persistentAtoms);
        return persistentAtomsFlags;
    }

    std::vector<DataAtom> sharedAtomPool;

private:
    std::vector<Event> events;
    std::vector<std::size_t> freeStack;
    size_t freestackPos = 0;
    size_t freestackSize = 0;

    std::vector<uint64_t> persistentAtoms;
    PersistentAtomFlags persistentAtomsFlags;

    std::vector<size_t> freeList;
    std::vector<size_t> allocatedList;
};

class Graph;

class NodeContext {
public:
    float sampleRate;
    int frameCount;
    EventPool eventPool;

    OwnershipBlockPool ownershipBlockPool;

    moodycamel::ReaderWriterQueue<std::function<void(Graph& runningGraph)>> messageQueue;

    LockFreeHashMap stringMap;

    PersistentAtomFlags& persistentFlags;

    NodeContext(float sampleRate, int frameCount)
        : sampleRate(sampleRate)
        , frameCount(frameCount)
        , ownershipBlockPool(100000)
        , persistentFlags(eventPool.getPersistentFlags())
    {};

    void makeDataPersistent(DataAtom* atom, const bool toBePersistent, int nodeID)
    {
        if (atom)
        {
            atom->makePersistent(toBePersistent, nodeID, ownershipBlockPool, persistentFlags, eventPool.sharedAtomPool);
        }
    }
};
