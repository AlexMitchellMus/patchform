#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>

class PersistentAtomFlags {
public:
    std::vector<uint64_t>* bitfield = nullptr;

    void set(size_t index) const
    {
        (*bitfield)[index >> 6] |= (1ULL << (index & 63));
    }

    void clear(size_t index) const
    {
        (*bitfield)[index >> 6] &= ~(1ULL << (index & 63));
    }

    [[nodiscard]] bool isSet(size_t index) const
    {
        return ((*bitfield)[index >> 6] >> (index & 63)) & 1;
    }

    [[nodiscard]] bool isAllocated(size_t index) const
    {
        return bitfield && ((*bitfield)[index >> 6] >> (index & 63)) & 1;
    }

    void bind(std::vector<uint64_t>& bits)
    {
        bitfield = &bits;
    }
};
