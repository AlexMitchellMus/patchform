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
    std::string nameName;
    hash32 tagHash;

    Tag(const std::string& tag) : nameName(tag), tagHash(hash(tag)) {}
};

class Event
{
    uint64_t timeStamp = 0;
    Tag tag = Tag("trigger");

public:
    // Pointer to the head of the linked list of data atoms.
    DataAtom* data;

    Event() : data(nullptr), tail(nullptr), usedAtomCount(0)
    {
        reserveAtoms(1024);
    }

    explicit Event(uint64_t timeStamp)
        : timeStamp(timeStamp), data(nullptr), tail(nullptr), usedAtomCount(0)
    {
        reserveAtoms(1024);
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

    // Reserve a fixed capacity for the atom pool.
    // This should be done once (or when you know you'll add more atoms) to avoid runtime allocation.
    void reserveAtoms(size_t capacity)
    {
        atomPool.resize(capacity);
        resetAtoms();
    }

    // Adds an atom to the end of the linked list.
    // Uses a preallocated atom from the atomPool and connects it.
    void addAtom(float value)
    {
        if (usedAtomCount >= atomPool.size())
        {
            throw std::runtime_error("Atom pool exhausted.");
        }
        // Get the next available atom.
        DataAtom* newAtom = &atomPool[usedAtomCount++];
        newAtom->atom = value;
        newAtom->next = nullptr;

        if (data == nullptr)
        {
            // First atom being added.
            data = newAtom;
            tail = newAtom;
        }
        else
        {
            tail->next = newAtom;
            tail = newAtom;
        }
    }

    int getNumAtoms()
    {
        return usedAtomCount;
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
        usedAtomCount = 0;
        data = nullptr;
        tail = nullptr;
        timeStamp = 0;
    }

private:
    std::vector<DataAtom> atomPool;
    size_t usedAtomCount;  // Number of atoms currently used in the pool.
    DataAtom* tail;        // Pointer to the last DataAtom in the linked list.
};
