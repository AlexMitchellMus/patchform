#pragma once

#include <vector>
#include <iostream>
#include <stdexcept>
#include "../Utility/Hash.h"
#include "bitset"
#include "PersistentAtomFlags.h"
#include "OwnershipBlockPool.h"
#include "DataAtom.h"

class Tag
{
public:
    hash32 tagHash = 0;

    Tag() = default;

    Tag(const std::string& tag) : tagHash(hash(tag)) {}

    Tag(const hash32 hash) : tagHash(hash) {}
};

class alignas(64) Event
{
    uint64_t timeStamp = 0;
    Tag tag = Tag("trigger");

public:
    int numAtoms = 0;
    // Pointer to the head of the linked list of data atoms.
    DataAtom* data;
    // Pointer to the last DataAtom in the linked list.
    DataAtom* tail;

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

    void setTagHashcode(const hash32 hash)
    {
        tag = hash;
    }

    hash32 getTagHash() const
    {
        return tag.tagHash;
    }

    [[nodiscard]] float getAtomValue(const int index) const
    {
        if (DataAtom* atomPtr = getAtom(index))
        {
            if (atomPtr->type == DataAtom::DataType::Float)
                return atomPtr->data.atom;
        }
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

    void shallowCopyFrom(const Event* src)
    {
        data = src->data;
        numAtoms = src->numAtoms;
        tail = src->tail;
        timeStamp = src->getTimeStamp();
        tag = src->tag;
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
};
