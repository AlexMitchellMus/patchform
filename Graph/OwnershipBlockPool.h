/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <stdexcept>
#include <cstdint>

struct alignas(64) OwnershipBlock {
    int blockIndex;                 // Represents owner IDs from blockIndex * 320 to (blockIndex + 1) * 320 - 1
    uint64_t bits[5];               // 5 x 64-bit bitmask = 320 owners per block
    OwnershipBlock* next;           // Pointer to the next block
};

class OwnershipBlockPool {
public:
    static constexpr int kBitsPerWord = 64;
    static constexpr int kWordsPerBlock = 5;
    static constexpr int kBitsPerBlock = kBitsPerWord * kWordsPerBlock;

    explicit OwnershipBlockPool(std::size_t capacity) : freeList(nullptr) {
        blocks.resize(capacity);
        for (auto & block : blocks) {
            block.next = freeList;
            freeList = &block;
        }
    }

    OwnershipBlock* acquire() {
        if (!freeList)
            throw std::runtime_error("OwnershipBlockPool exhausted");

        OwnershipBlock* block = freeList;
        freeList = block->next;
        block->next = nullptr;
        block->blockIndex = -1;
        std::memset(block->bits, 0, sizeof(block->bits)); // Clear all bits
        return block;
    }

    void release(OwnershipBlock* block) {
        block->next = freeList;
        freeList = block;
    }

    // --- Bit math helpers ---

    static int getBlockIndex(int nodeID) {
        return nodeID / kBitsPerBlock;
    }

    static int getWordIndex(int nodeID) {
        return (nodeID % kBitsPerBlock) / kBitsPerWord;
    }

    static uint64_t getBitMask(int nodeID) {
        return 1ULL << (nodeID % kBitsPerWord);
    }

private:
    std::vector<OwnershipBlock> blocks;
    OwnershipBlock* freeList;
};
