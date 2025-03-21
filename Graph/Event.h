#pragma once

#include <vector>
#include <iostream>
#include <stdexcept>
#include "../Utility/Hash.h"

class DataAtom
{
public:
    enum class DataType { Float, List };

    DataType type = DataType::Float;  // Default to Float

    union data {
        float atom;           // Stores a float value
        DataAtom* list;       // If type == List, this points to a sublist
    } data;

    DataAtom* next = nullptr;  // Next item in the main chain

    bool isPersistent = false;

    // **Recursively convert DataAtom chain into a readable string**
    [[nodiscard]] std::string toString(const bool recursive = true) const
    {
        std::stringstream ss;
        const DataAtom* walk = this;

        while (walk)
        {
            if (walk->type == DataType::Float)
            {
                ss << walk->data.atom;
            }
            else
            {
                ss << "{ ";
                ss << walk->data.list->toString(); // **Recursively call `toString()` on sublist**
                ss << " }";
            }

            if (recursive)
            {
                if (walk->next) ss << ", ";  // **Separate elements with commas**
                walk = walk->next;
            }
        }

        return ss.str();
    }

    void makePersistent(bool toBePersistent)
    {
        makePersistent(this, toBePersistent);
    }

    [[nodiscard]] DataAtom* getAtom(const int index)
    {
        DataAtom* current = this;
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

    [[nodiscard]] size_t getAtomCount()
    {
        DataAtom* current = this;
        int count = 0;
        while (current != nullptr)
        {
            current = current->next;
            ++count;
        }
        return count;
    }

private:
    void makePersistent(DataAtom* atom, const bool toBePersistent)
    {
        while (atom != nullptr)
        {
            atom->isPersistent = toBePersistent;
            // If this is a list atom, mark its sublist persistent as well.
            if (atom->type == DataAtom::DataType::List && atom->data.list)
                makePersistent(atom->data.list, toBePersistent);
            atom = atom->next;
        }
    }
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
