#pragma once

#include <vector>
#include <iostream>
#include <stdexcept>
#include "../Utility/Hash.h"

struct OwnershipToken {
    int nodeId;             // The ID of the Get node
    OwnershipToken* next;    // Pointer to the next owner
};

// A simple pool for OwnershipToken objects.
class OwnershipTokenPool {
public:
    OwnershipTokenPool(size_t capacity) : freeList(nullptr) {
        tokens.resize(capacity);
        // Initialize the free list.
        for (size_t i = 0; i < tokens.size(); i++) {
            tokens[i].next = freeList;
            freeList = &tokens[i];
        }
    }

    // Acquire a token from the pool (throws if exhausted).
    OwnershipToken* acquire() {
        if (!freeList)
            throw std::runtime_error("OwnershipTokenPool exhausted");
        OwnershipToken* token = freeList;
        freeList = freeList->next;
        token->next = nullptr;
        return token;
    }

    // Return a token to the pool.
    void release(OwnershipToken* token) {
        token->next = freeList;
        freeList = token;
    }

private:
    std::vector<OwnershipToken> tokens;
    OwnershipToken* freeList;
};


class DataAtom
{
public:
    enum class DataType { Float, List, Symbol };

    DataType type = DataType::Float;  // Default to Float

    union data {
        float atom;           // float value
        hash32 symbol;
        DataAtom* list;       // sublist
    } data;

    DataAtom* next = nullptr;  // Next item in the main chain

    bool isPersistent = false;

    OwnershipToken* ownerList = nullptr;

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
            else if (walk->type == DataType::Symbol)
            {
                ss << "@$" << walk->data.symbol << "$@";
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

    void makePersistent(bool toBePersistent, int nodeID, OwnershipTokenPool& ownerList)
    {
        makePersistent(this, toBePersistent, nodeID, ownerList);
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
    void makePersistent(DataAtom* atom, const bool toBePersistent, const int nodeID, OwnershipTokenPool& ownerList)
    {
        while (atom != nullptr)
        {
            if (toBePersistent)
                addOwnership(atom, nodeID, ownerList);
            else
                removeOwnership(atom, nodeID, ownerList);

            // Update the persistent flag: true if any node owns this atom.
            atom->isPersistent = (atom->ownerList != nullptr);

            // If this is a list atom, recursively update its sublist.
            if (atom->type == DataAtom::DataType::List)
                makePersistent(atom->data.list, toBePersistent, nodeID, ownerList);

            atom = atom->next;
        }
    }

    // Helper to add an ownership link for a given node.
    void addOwnership(DataAtom* atom, int nodeID, OwnershipTokenPool& ownerList) {
        // If the node already owns this atom, do nothing.
        OwnershipToken* cur = atom->ownerList;
        while (cur) {
            if (cur->nodeId == nodeID)
                return;
            cur = cur->next;
        }
        // Otherwise, allocate a new link, set its nodeId, and prepend it.
        OwnershipToken* newLink = ownerList.acquire();
        newLink->nodeId = nodeID;         // Set the node ID.
        newLink->next = atom->ownerList;    // Link the current list.
        atom->ownerList = newLink;          // Prepend the new token.
    }

    // Helper to remove an ownership link for a given node.
    void removeOwnership(DataAtom* atom, int nodeID, OwnershipTokenPool& ownerList) {
        OwnershipToken** cur = &atom->ownerList;
        while (*cur) {
            if ((*cur)->nodeId == nodeID) {
                OwnershipToken* toRelease = *cur;
                *cur = toRelease->next;
                ownerList.release(toRelease);
                break;
            }
            cur = &((*cur)->next);
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
