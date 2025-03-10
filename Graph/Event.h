#pragma once

#include <vector>
#include <iostream>
#include <stdexcept>
#include "../Utility/Hash.h"

class DataAtom
{
public:
    float atom = 0.0f;
    DataAtom* next = nullptr;
};

class Tag
{
public:
    hash32 tagHash;

    Tag(const std::string& tag) : tagHash(hash(tag)) {}

    Tag(const hash32 hash) : tagHash(hash) {}
};

class Event
{
    uint64_t timeStamp = 0;
    Tag tag = Tag("trigger");

public:
    // Pointer to the head of the linked list of data atoms.
    DataAtom* data;

    std::function<void(float)> addAtom = [](float){};

    Event() : data(nullptr), tail(nullptr)
    {
    }

    explicit Event(uint64_t timeStamp)
        : timeStamp(timeStamp), data(nullptr), tail(nullptr)
    {
    }

    void setTag(const std::string& tagName)
    {
        tag = Tag(tagName);
    }

    hash32 getTagHash()
    {
        return tag.tagHash;
    }

    [[nodiscard]] float getAtomValue(const int index) const
    {
        if (DataAtom* atomPtr = getAtom(index))
            return atomPtr->atom;
        return 0.0f;
    }

    // Retrieve the atom at a given index by traversing the linked list.
    [[nodiscard]] DataAtom* getAtom(const int index) const
    {
        DataAtom* current = data;
        int count = 0;
        while (current != nullptr)
        {
            if (count == index)
                return current;
            current = current->next;
            ++count;
        }
        return nullptr;
    }

    [[nodiscard]] size_t getNumAtoms() const
    {
        return numAtoms;
    }

    void shallowCopyFrom(const Event& src)
    {
        data = src.data;
        numAtoms = src.numAtoms;
        tail = src.tail;
        timeStamp = src.getTimeStamp();
        tag = src.tag;
    }

    void deepCopyFrom(const Event& src)
    {
        timeStamp = src.getTimeStamp();
        // Deep copy the data list.
        data = cloneDataAtoms(src.data);
        numAtoms = src.numAtoms;
        // Recompute the tail pointer from the new data list.
        tail = data;
        if (tail) {
            while (tail->next)
                tail = tail->next;
        }
    }

    static DataAtom* cloneDataAtoms(const DataAtom* src)
    {
        if (!src)
            return nullptr;
        // Allocate a new DataAtom for the head.
        DataAtom* newHead = new DataAtom();
        newHead->atom = src->atom;
        newHead->next = nullptr;
        DataAtom* currentNew = newHead;
        const DataAtom* currentSrc = src->next;
        while (currentSrc)
        {
            DataAtom* newAtom = new DataAtom();
            newAtom->atom = currentSrc->atom;
            newAtom->next = nullptr;
            currentNew->next = newAtom;
            currentNew = newAtom;
            currentSrc = currentSrc->next;
        }
        return newHead;
    }

    uint64_t getTimeStamp() const
    {
        return timeStamp;
    }

    Event& setTimeStamp(const uint64_t timestamp)
    {
        timeStamp = timestamp;
        return *this;
    }

    // Resets the linked list pointers and the used count.
    // The atomPool remains allocated, so the DataAtoms can be reused.
    void resetAtoms()
    {
        data = nullptr;
        tail = nullptr;
        tag = Tag(hash("trigger"));
        timeStamp = 0;
        numAtoms = 0;
    }

    DataAtom* tail;        // Pointer to the last DataAtom in the linked list.

    int numAtoms = 0;

private:
};
