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

    // Write a string representation of the DataAtom chain into the provided preallocated buffer.
    // The caller must preallocate 'textBuffer' to at least MAX_BUFFER characters (e.g. textBuffer.resize(MAX_BUFFER)).
    // No dynamic allocation is performed within this function.
    void toString(std::string& textBuffer, bool recursive = true) const
    {
        constexpr int MAX_BUFFER = 1024;
        constexpr int MAX_DEPTH = 8;
        // Ensure the buffer is large enough.
        if (textBuffer.size() < MAX_BUFFER)
        {
            textBuffer.resize(MAX_BUFFER);
        }
        int pos = 0; // current write position

        // A simple stack frame to track our traversal state.
        struct StackFrame
        {
            const DataAtom* atom; // current element in the chain at this level
            int depth; // current nesting depth (0 for root)
            bool firstElement; // true if no element has been output yet at this level
        };

        // Fixed-size stack to avoid recursion.
        StackFrame stack[MAX_DEPTH];
        int stackTop = 0;
        stack[stackTop++] = {this, 0, true};

        char* buffer = &textBuffer[0];

        while (stackTop > 0 && pos < MAX_BUFFER - 1)
        {
            // leave room for null terminator
            StackFrame& frame = stack[stackTop - 1];
            const DataAtom* current = frame.atom;

            // End of this chain: pop the frame.
            if (current == nullptr)
            {
                stackTop--;
                if (frame.depth > 0)
                {
                    // Append closing brace if not at root.
                    int n = snprintf(buffer + pos, MAX_BUFFER - pos, " }");
                    if (n > 0) pos += n;
                }
                if (stackTop > 0)
                    stack[stackTop - 1].firstElement = false;
                continue;
            }

            // Add a comma separator if not the first element.
            if (!frame.firstElement)
            {
                int n = snprintf(buffer + pos, MAX_BUFFER - pos, ", ");
                if (n > 0) pos += n;
            }
            frame.firstElement = false;

            // Process the current DataAtom based on its type.
            if (current->type == DataType::Float)
            {
                float value = current->data.atom;
                // If the value is very close to its rounded version, print as an integer.
                if (fabsf(value - roundf(value)) < 1e-6)
                {
                    int n = snprintf(buffer + pos, MAX_BUFFER - pos, "%.0f", value);
                    if (n > 0) pos += n;
                }
                else
                {
                    // Otherwise, print with a fixed number of decimal places.
                    int n = snprintf(buffer + pos, MAX_BUFFER - pos, "%.6f", value);
                    if (n > 0) pos += n;
                }
            }
            else if (current->type == DataType::Symbol)
            {
                int n = snprintf(buffer + pos, MAX_BUFFER - pos, "@$%u$@", current->data.symbol);
                if (n > 0) pos += n;
            }
            else
            {
                // List type.
                int n = snprintf(buffer + pos, MAX_BUFFER - pos, "{ ");
                if (n > 0) pos += n;
                if (recursive && frame.depth + 1 < MAX_DEPTH)
                {
                    // Push a new frame for the nested list.
                    stack[stackTop++] = {current->data.list, frame.depth + 1, true};
                }
                else
                {
                    // If max depth reached or recursion disabled, write ellipsis and close.
                    int n2 = snprintf(buffer + pos, MAX_BUFFER - pos, "...");
                    if (n2 > 0) pos += n2;
                    int n3 = snprintf(buffer + pos, MAX_BUFFER - pos, " }");
                    if (n3 > 0) pos += n3;
                }
            }

            // Move to the next element in the current chain.
            frame.atom = current->next;
        }

        // Ensure null termination and adjust the std::string size.
        if (pos >= MAX_BUFFER)
            pos = MAX_BUFFER - 1;
        buffer[pos] = '\0';
        textBuffer.resize(pos);
    }


    void makePersistent(int nodeID, const bool toBePersistent, OwnershipTokenPool& ownerList)
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
    static void makePersistent(DataAtom* atom, const bool toBePersistent, const int nodeID, OwnershipTokenPool& ownerList)
    {
        while (atom != nullptr)
        {
            if (toBePersistent)
                addOwnership(atom, nodeID, ownerList);
            else
                removeOwnership(atom, nodeID, ownerList);

            // Update the persistent flag: true if any node owns this atom.
            const OwnershipToken* curOwners = atom->ownerList;
            atom->isPersistent = (curOwners != nullptr);

            // If this is a list atom, recursively update its sublist.
            if (atom->type == DataAtom::DataType::List)
                makePersistent(atom->data.list, toBePersistent, nodeID, ownerList);

            atom = atom->next;
        }
    }

    static inline void addOwnership(DataAtom* atom, const int nodeID, OwnershipTokenPool& ownerList)
    {
        const OwnershipToken* cur = atom->ownerList;
        while (cur) {
            if (cur->nodeId == nodeID)
                return; // already owned
            cur = cur->next;
        }
        OwnershipToken* newLink = ownerList.acquire();
        newLink->nodeId = nodeID;
        newLink->next = atom->ownerList;
        atom->ownerList = newLink;
    }

    static inline void removeOwnership(DataAtom* atom, const int nodeID, OwnershipTokenPool& ownerList)
    {
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
