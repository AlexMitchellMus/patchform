/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once
#include <vector>
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
