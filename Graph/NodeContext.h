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
        persistentList.reserve(count);

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
        atom->data.list = nullptr;
        atom->next = nullptr;
        return atom;
    }

    int getFreeListSize()
    {
        return freeList.size();
    }

    // Fast allocation: Pull from freeList and move to allocatedList
    DataAtom* allocateListAtom() {
        if (freeList.empty()) {
            std::cout << "No free atoms left!" << std::endl;
            return nullptr; // No available datatoms
        }

        size_t index = freeList.back();
        freeList.pop_back();
        allocatedList.push_back(index);

        // **Ensure list pointer is initialized**
        DataAtom* listAtom = &sharedAtomPool[index];
        listAtom->type = DataAtom::DataType::List;
        listAtom->data.list = nullptr;  // **Ensure it starts empty**
        listAtom->next = nullptr;  // **Reset next pointer**

        return listAtom;
    }

    // Recycling: Move atom back to freeList unless persistent
    void recycleDataAtom(DataAtom* atom) {
        size_t index = atom - &sharedAtomPool[0];

        // If it's persistent, don't recycle yet
        auto it = std::find(persistentList.begin(), persistentList.end(), index);
        if (it == persistentList.end()) {
            freeList.push_back(index); // Only recycle if not persistent
        }
    }

    // Reset free list at the end of each cycle
    void resetFreeList() {
        // Move allocated atoms back to free list
        freeList.insert(freeList.end(), allocatedList.begin(), allocatedList.end());
        allocatedList.clear();
    }

    // Make an atom and its linked atoms persistent
    void makePersistent(DataAtom* atom) {
        std::vector<DataAtom*> stack;
        stack.push_back(atom);

        while (!stack.empty()) {
            DataAtom* current = stack.back();
            stack.pop_back();

            if (!current) continue;

            size_t index = current - &sharedAtomPool[0];

            // If it's already persistent, skip
            if (std::find(persistentList.begin(), persistentList.end(), index) != persistentList.end())
                continue;

            persistentList.push_back(index); // Mark as persistent

            // Remove from freeList if it's there
            auto freeIt = std::find(freeList.begin(), freeList.end(), index);
            if (freeIt != freeList.end()) {
                freeList.erase(freeIt);
            }

            // Remove from allocatedList if it's there
            auto allocIt = std::find(allocatedList.begin(), allocatedList.end(), index);
            if (allocIt != allocatedList.end()) {
                allocatedList.erase(allocIt);
            }

            // **Handle linked atoms (next pointer)**
            if (current->next) {
                stack.push_back(current->next);
            }

            // **Handle sublists if it's a List atom**
            if (current->type == DataAtom::DataType::List && current->data.list) {
                stack.push_back(current->data.list);
            }
        }
    }


    // Clear persistent datatoms (Deferred Freeing) (`O(N)`)
    void clearPersistentDataAtom(DataAtom* atom) {
        std::vector<DataAtom*> stack;
        stack.push_back(atom);

        while (!stack.empty()) {
            DataAtom* current = stack.back();
            stack.pop_back();

            if (!current) continue;

            size_t index = current - &sharedAtomPool[0];

            // Remove from persistentList
            auto it = std::find(persistentList.begin(), persistentList.end(), index);
            if (it != persistentList.end()) {
                persistentList.erase(it);
                // Move the atom to regular allocated list
                // This list is cleared each cycle
                allocatedList.push_back(index);
            }

            // **Handle linked atoms (next pointer)**
            if (current->next) {
                stack.push_back(current->next);
            }

            // **Handle sublists if it's a List atom**
            if (current->type == DataAtom::DataType::List && current->data.list) {
                stack.push_back(current->data.list);
            }
        }
    }


private:
    std::vector<Event> events;
    std::vector<std::size_t> freeStack;
    std::vector<std::size_t> templateStack;

    std::vector<DataAtom> sharedAtomPool;

    std::vector<size_t> freeList;
    std::vector<size_t> allocatedList;
    std::vector<size_t> persistentList;
};

class NodeContext {
public:
    float sampleRate;
    int frameCount;
    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {};
};
