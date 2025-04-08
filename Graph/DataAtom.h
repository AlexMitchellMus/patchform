#pragma once

#include "../Utility/Hash.h"
#include <cstdint>
#include <string>
#include <vector>

#include "PersistentAtomFlags.h"
#include "OwnershipBlockPool.h"

#include "SampleHandle.h"

class alignas(32) DataAtom
{
public:
    enum class DataType : uint8_t { Float, List, Symbol, Sample };

    union Data
    {
        float atom;
        hash32 symbol;
        DataAtom* list;
        SampleHandle sample;

        Data()
        {
        }

        ~Data()
        {
        }

        Data(const Data& other, DataType type)
        {
            switch (type)
            {
            case DataType::Float:
                atom = other.atom;
                break;
            case DataType::Symbol:
                symbol = other.symbol;
                break;
            case DataType::List:
                list = other.list;
                break;
            case DataType::Sample:
                new(&sample) SampleHandle(other.sample);
                break;
            }
        }

        Data(Data&& other, DataType type) noexcept
        {
            switch (type)
            {
            case DataType::Float:
                atom = other.atom;
                break;
            case DataType::Symbol:
                symbol = other.symbol;
                break;
            case DataType::List:
                list = other.list;
                break;
            case DataType::Sample:
                new(&sample) SampleHandle(std::move(other.sample));
                break;
            }
        }

        void destroy(DataType type)
        {
            if (type == DataType::Sample)
                sample.~SampleHandle();
        }
    } data;

    // ✅ Default constructor
    DataAtom()
    {
        data.atom = 0.0f;
    }

    // ✅ Copy constructor
    DataAtom(const DataAtom& other)
        : type(other.type), next(nullptr), ownerChain(nullptr)
    {
        new(&data) Data(other.data, other.type);
    }

    // ✅ Copy assignment
    DataAtom& operator=(const DataAtom& other)
    {
        if (this != &other)
        {
            data.destroy(type);
            type = other.type;
            new(&data) Data(other.data, type);
        }
        return *this;
    }

    // ✅ Move constructor
    DataAtom(DataAtom&& other) noexcept
        : type(other.type), next(nullptr), ownerChain(nullptr)
    {
        new(&data) Data(std::move(other.data), other.type);
    }

    // ✅ Move assignment
    DataAtom& operator=(DataAtom&& other) noexcept
    {
        if (this != &other)
        {
            data.destroy(type);
            type = other.type;
            new(&data) Data(std::move(other.data), type);
        }
        return *this;
    }

    ~DataAtom()
    {
        data.destroy(type);
    }

    DataAtom* next = nullptr;
    OwnershipBlock* ownerChain = nullptr;

    DataType type = DataType::Float;

    [[nodiscard]] size_t getAtomCount() const
    {
        auto current = this;
        size_t count = 0;
        while (current != nullptr)
        {
            ++count;
            current = current->next;
        }
        return count;
    }

    [[nodiscard]] const DataAtom* getAtom(const int index) const
    {
        auto current = this;
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

    void copyFrom(const DataAtom* other)
    {
        if (!other)
        {
            type = DataType::Float;
            data.atom = 0.f;
            return;
        }

        type = other->type;

        switch (type)
        {
        case DataType::Float:
            data.atom = other->data.atom;
            break;

        case DataType::Symbol:
            data.symbol = other->data.symbol;
            break;

        case DataType::List:
            data.list = other->data.list;
            break;

        case DataType::Sample:
            new(&data.sample) SampleHandle(other->data.sample);
            break;
        }
    }

    void toString(std::string& textBuffer, bool recursive = true) const
    {
        constexpr int MAX_BUFFER = 1024;
        constexpr int MAX_DEPTH = 8;

        if (textBuffer.capacity() < MAX_BUFFER)
            textBuffer.reserve(MAX_BUFFER);

        char buffer[MAX_BUFFER];
        int pos = 0;

        struct StackFrame
        {
            const DataAtom* atom;
            int depth;
            bool firstElement;
        };

        StackFrame stack[MAX_DEPTH];
        int stackTop = 0;
        stack[stackTop++] = {this, 0, true};

        while (stackTop > 0 && pos < MAX_BUFFER - 1)
        {
            StackFrame& frame = stack[stackTop - 1];
            const DataAtom* current = frame.atom;

            if (current == nullptr)
            {
                stackTop--;
                if (frame.depth > 0 && pos < MAX_BUFFER - 2)
                {
                    buffer[pos++] = ' ';
                    buffer[pos++] = '}';
                }
                if (stackTop > 0)
                    stack[stackTop - 1].firstElement = false;
                continue;
            }

            if (!frame.firstElement && pos < MAX_BUFFER - 2)
            {
                buffer[pos++] = ',';
                buffer[pos++] = ' ';
            }
            frame.firstElement = false;

            if (current->type == DataType::Float)
            {
                float v = current->data.atom;
                if (fabsf(v - roundf(v)) < 1e-6f)
                {
                    // Print as integer
                    int iv = static_cast<int>(roundf(v));
                    auto [ptr, ec] = std::to_chars(buffer + pos, buffer + MAX_BUFFER, iv);
                    pos += static_cast<int>(ptr - (buffer + pos));
                }
                else
                {
                    // Print as float, fixed 2 decimal places
                    int written = snprintf(buffer + pos, MAX_BUFFER - pos, "%.2f", v);
                    pos += written;
                }
            }
            else if (current->type == DataType::Symbol)
            {
                if (pos < MAX_BUFFER - 6)
                {
                    buffer[pos++] = '@';
                    buffer[pos++] = '$';
                    auto [ptr, ec] = std::to_chars(buffer + pos, buffer + MAX_BUFFER, current->data.symbol);
                    pos += static_cast<int>(ptr - (buffer + pos));
                    buffer[pos++] = '$';
                    buffer[pos++] = '@';
                }
            }
            else // List
            {
                if (pos < MAX_BUFFER - 2)
                {
                    buffer[pos++] = '{';
                    buffer[pos++] = ' ';
                }

                if (recursive && frame.depth + 1 < MAX_DEPTH)
                {
                    stack[stackTop++] = {current->data.list, frame.depth + 1, true};
                }
                else
                {
                    const char* ellipsis = "... }";
                    int len = static_cast<int>(strlen(ellipsis));
                    if (pos + len < MAX_BUFFER)
                    {
                        memcpy(buffer + pos, ellipsis, len);
                        pos += len;
                    }
                }
            }

            frame.atom = current->next;
        }

        if (pos >= MAX_BUFFER)
            pos = MAX_BUFFER - 1;
        buffer[pos] = '\0';

        // Only copy into textBuffer at the end
        textBuffer.assign(buffer, pos);
    }

    void makePersistent(bool toBePersistent, int nodeID, OwnershipBlockPool& pool, PersistentAtomFlags& flags,
                        const std::vector<DataAtom>& atomPool)
    {
        makePersistent(this, toBePersistent, nodeID, pool, flags, atomPool);
    }

private:
    static void makePersistent(DataAtom* atom, bool toBePersistent, int nodeID, OwnershipBlockPool& pool,
                               PersistentAtomFlags& flags, const std::vector<DataAtom>& atomPool)
    {
        while (atom != nullptr)
        {
            /*
            int ownershipCount = 0;
            auto walk = atom->ownerChain;
            while (walk != nullptr)
            {
                walk = walk->next;
                ownershipCount++;
            }

            std::cout << "ownership blocks: " << ownershipCount << std::endl;
            */

            if (toBePersistent)
                addOwnership(atom, nodeID, pool, flags, atomPool);
            else
                removeOwnership(atom, nodeID, pool, flags, atomPool);

            if (atom->type == DataType::List)
                makePersistent(atom->data.list, toBePersistent, nodeID, pool, flags, atomPool);

            atom = atom->next;
        }
    }

    static void addOwnership(DataAtom* atom, const int nodeID, OwnershipBlockPool& pool,
                             const PersistentAtomFlags& flags, const std::vector<DataAtom>& atomPool)
    {
        const auto index = static_cast<size_t>(atom - atomPool.data());

        const int blockIndex = OwnershipBlockPool::getBlockIndex(nodeID);
        const int wordIndex = OwnershipBlockPool::getWordIndex(nodeID);
        const uint64_t bit = OwnershipBlockPool::getBitMask(nodeID);
        OwnershipBlock** ppBlock = &atom->ownerChain;

        while (*ppBlock && (*ppBlock)->blockIndex < blockIndex)
            ppBlock = &((*ppBlock)->next);

        if (*ppBlock && (*ppBlock)->blockIndex == blockIndex)
        {
            (*ppBlock)->bits[wordIndex] |= bit;
        }
        else
        {
            OwnershipBlock* newBlock = pool.acquire();
            newBlock->blockIndex = blockIndex;
            newBlock->bits[wordIndex] = bit;
            newBlock->next = *ppBlock;
            *ppBlock = newBlock;
        }

        flags.set(index);
    }

    static void removeOwnership(DataAtom* atom, const int nodeID, OwnershipBlockPool& pool,
                                const PersistentAtomFlags& flags, const std::vector<DataAtom>& atomPool)
    {
        const auto index = static_cast<size_t>(atom - atomPool.data());

        const int blockIndex = OwnershipBlockPool::getBlockIndex(nodeID);
        const int wordIndex = OwnershipBlockPool::getWordIndex(nodeID);
        const uint64_t bit = OwnershipBlockPool::getBitMask(nodeID);
        OwnershipBlock** ppBlock = &atom->ownerChain;

        while (*ppBlock && (*ppBlock)->blockIndex < blockIndex)
            ppBlock = &((*ppBlock)->next);

        if (*ppBlock && (*ppBlock)->blockIndex == blockIndex)
        {
            (*ppBlock)->bits[wordIndex] &= ~bit;

            bool isEmpty = true;
            for (int i = 0; i < OwnershipBlockPool::kWordsPerBlock; ++i)
            {
                if ((*ppBlock)->bits[i] != 0)
                {
                    isEmpty = false;
                    break;
                }
            }

            if (isEmpty)
            {
                OwnershipBlock* toRelease = *ppBlock;
                *ppBlock = toRelease->next;
                pool.release(toRelease);
            }
        }

        if (atom->ownerChain == nullptr)
            flags.clear(index);
    }
};
