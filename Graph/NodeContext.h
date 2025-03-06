/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <numeric>

class EventPool {
public:
    EventPool(std::size_t initialSize = 1024)
    {
        growAtomPool(initialSize * 100);
        growPool(initialSize);
    }

    int eventPoolSize()
    {
        return events.size();
    }

    // Acquire a free Event (returns nullptr if none available)
    Event* getFreeEvent() {
        if (freeStack.empty()) {
            std::cout << "No free events left!" << std::endl;
            return nullptr;
        }
        // Pop index from stack
        //std::cout << "stack index: " << freeStack.back() << std::endl;
        auto eventIndex = freeStack.back();
        freeStack.pop_back();

        Event* evt = &events[eventIndex];
        evt->resetAtoms();
        return evt;
    }

    void releaseAllEvents()
    {
        freeStack = templateStack;
        nextAtomIndex = 0;
    }

    // Grow the pool by adding N new events (call this OUTSIDE audio callback)
    void growPool(std::size_t count) {
        // Current size
        auto oldSize = events.size();

        auto newSize = oldSize + count;

        events.resize(newSize);

        for (size_t i = oldSize; i < newSize; ++i)
        {
            events[i].addAtom = [this, i](float value) {
                if (nextAtomIndex >= sharedAtomPool.size()) {
                    throw std::runtime_error("Atom pool exhausted.");
                }
                // Get the next available atom.
                DataAtom* newAtom = &sharedAtomPool[nextAtomIndex++];
                newAtom->atom = value;
                newAtom->next = nullptr;

                // Retrieve the event using its index.
                auto& evnt = events[i];

                if (evnt.data == nullptr) {
                    // First atom being added.
                    evnt.data = newAtom;
                    evnt.tail = newAtom;
                    evnt.numAtoms = 1;
                }
                else {
                    evnt.tail->next = newAtom;
                    evnt.tail = newAtom;
                    ++(evnt.numAtoms);
                }
            };
        }

        freeStack.resize(newSize);
        std::iota(freeStack.begin(), freeStack.end(), 0);

        templateStack.resize(newSize);
        std::iota(templateStack.begin(), templateStack.end(), 0);
    }

    void growAtomPool(std::size_t count)
    {
        sharedAtomPool.resize(count);
    }

private:
    std::vector<Event> events;                    // Actual storage
    std::vector<std::size_t> freeStack;           // Indices of free events
    std::vector<std::size_t> templateStack;       // Template of a full freestack

    std::vector<DataAtom> sharedAtomPool;         // Shared data pool for all atoms
    size_t nextAtomIndex = 0;
};

class NodeContext {

public:
    float sampleRate;
    int frameCount;

    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {};
};