// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include <iostream>
#include <vector>
#include <numeric>

class EventPool {
public:
    EventPool(std::size_t initialSize = 1024) {
        growAtomPool(initialSize * 100);
        growPool(initialSize);
    }

    int eventPoolSize() {
        return events.size();
    }

    // Acquire a free Event (returns nullptr if none available)
    Event* getFreeEvent() {
        if (freeStack.empty()) {
            std::cout << "No free events left!" << std::endl;
            return nullptr;
        }

        auto eventIndex = freeStack.back();
        freeStack.pop_back();

        Event* evt = &events[eventIndex];
        evt->resetAtoms();
        return evt;
    }

    void releaseAllEvents() {
        freeStack = templateStack;
        resetFreeList();
    }

    // Grow the event pool
    void growPool(std::size_t count) {
        auto oldSize = events.size();
        auto newSize = oldSize + count;

        events.resize(newSize);

        for (size_t i = oldSize; i < newSize; ++i) {
            events[i].addAtom = [this, i](float value) {
                DataAtom* newAtom = allocateDataAtom();
                if (!newAtom) {
                    throw std::runtime_error("Atom pool exhausted.");
                }

                newAtom->type = DataAtom::DataType::Float;
                newAtom->data.atom = value;
                newAtom->next = nullptr;

                auto& evnt = events[i];

                if (evnt.data == nullptr) {
                    evnt.data = newAtom;
                    evnt.tail = newAtom;
                    evnt.numAtoms = 1;
                } else {
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

    // Grow the atom pool
    void growAtomPool(std::size_t count) {
        sharedAtomPool.resize(count);

        freeList.reserve(count);
        allocatedList.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            freeList.push_back(i);
        }
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
        auto atom = &sharedAtomPool[index];
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
        std::erase_if(allocatedList,
                      [&](size_t index) {
                          if (const DataAtom* atom = &sharedAtomPool[index]; !atom->isPersistent) {
                              freeList.push_back(index);
                              return true;  // Remove this index.
                          }
                          return false;
                      }
        );
    }

private:
    std::vector<Event> events;
    std::vector<std::size_t> freeStack;
    std::vector<std::size_t> templateStack;

    std::vector<DataAtom> sharedAtomPool;

    std::vector<size_t> freeList;
    std::vector<size_t> allocatedList;
};

class NodeContext {
public:
    float sampleRate;
    int frameCount;
    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {};
};
