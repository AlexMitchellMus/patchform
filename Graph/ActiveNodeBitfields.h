/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <choc/containers/choc_SmallVector.h>
#include <cstdint>

/**
 * @brief Efficient bitfield system for tracking active audio and event nodes during graph processing.
 *
 * Instead of storing a per-node boolean flag, this structure uses two compact bitfields:
 * - activeAudioNodes: nodes that must be processed every cycle (e.g. always-on audio nodes)
 * - activeEventNodes: nodes that are activated by events (e.g. triggers, loadbang)
 *
 * Benefits:
 * - Avoids per-node boolean checks during processing
 * - Allows efficient scanning and skipping of inactive ranges
 * - Uses compact bit-level operations for fast real-time performance
 *
 * Each bitfield is sized to match the number of nodes, aligned to 64-bit words.
 * Indexing corresponds to the topologically sorted node list (`objectsSorted`).
 *
 * Audio nodes are assumed to run unconditionally via `alwaysProcess()`, but this may change in the future.
 * All bit manipulation is encapsulated here to avoid error-prone manual logic elsewhere.
 */

struct ActiveNodeBitfields
{
    choc::SmallVector<uint64_t, 16> activeAudioNodes;
    choc::SmallVector<uint64_t, 16> activeEventNodes;

    /**
     * @brief Resets the bitfields by resizing and clearing them based on node count.
     *
     * This method prepares both the audio and event bitfields for a new processing cycle.
     * All bits are cleared, and storage is resized to cover the given number of nodes.
     *
     * @warning Should only be called on a graph that is being constructed, never during real-time processing.
     *
     * @param nodeCount The number of nodes to allocate space for.
     */
    void reset(const size_t nodeCount)
    {
        const size_t wordCount = (nodeCount + 63) >> 6;

        activeAudioNodes.clear();
        activeAudioNodes.resize(wordCount);
        std::ranges::fill(activeAudioNodes, 0);

        activeEventNodes.clear();
        activeEventNodes.resize(wordCount);
        std::ranges::fill(activeEventNodes, 0);
    }

    /**
     * @brief Sets the audio active bit for a node at the given index.
     *
     * This marks the node as active for audio processing in the current cycle.
     *
     * @param index The node index (must match position in sorted node list).
     * @note This function is real-time safe and may be called from the audio thread.
     *       It performs no allocations or locks.
     */
    void setAudioBit(const size_t index)
    {
        activeAudioNodes[index >> 6] |= (1ULL << (index & 63));
    }

    /**
     * @brief Sets the event active bit for a node at the given index.
     *
     * This marks the node as triggered by an event and should be processed.
     *
     * @param index The node index (must match position in sorted node list).
     * @note This function is real-time safe and may be called from the audio thread.
     *       It performs no allocations or locks.
     */
    void setEventBit(const size_t index)
    {
        activeEventNodes[index >> 6] |= (1ULL << (index & 63));
    }

    /**
     * @brief Clears the event active bit for a node at the given index.
     *
     * This is typically done after processing an event to avoid re-triggering.
     *
     * @param index The node index to clear.
     * @note This function is real-time safe and may be called from the audio thread.
     *       It performs no allocations or locks.
     */
    void clearEventBit(const size_t index)
    {
        activeEventNodes[index >> 6] &= ~(1ULL << (index & 63));
    }

    /**
     * @brief Gets the combined (audio | event) bitmask for a 64-bit word,
     *        shifted right by a given bit offset.
     *
     * Used in active-node scanning to skip over inactive bits and locate
     * the next active node efficiently.
     *
     * @param wordIndex The word index (index >> 6).
     * @param bitOffset The bit offset to shift (index & 63).
     * @return A right-shifted 64-bit mask of the combined audio and event bits.
     * @note This function is real-time safe and may be called from the audio thread.
     *       It performs no allocations or locks.
     */
    [[nodiscard]] uint64_t getCombinedShiftedWord(const size_t wordIndex, const size_t bitOffset) const
    {
        return (activeAudioNodes[wordIndex] | activeEventNodes[wordIndex]) >> bitOffset;
    }

    /**
     * @brief Iterates over all active nodes using combined audio/event bitfields.
     *
     * Skips inactive ranges by scanning 64-bit blocks and jumping to the next set bit.
     * For each active index, calls `fn(i)` and clears the event bit afterward.
     *
     * Used during graph execution for fast dispatch of active nodes.
     *
     * @tparam Func Callable with the signature `void(size_t index)`.
     * @param totalNodes Total number of nodes in the sorted node vector.
     * @param fn Node process function to invoke for each active index.
     * @note This function is real-time safe and may be called from the audio thread.
     *       It performs no allocations or locks.
     */
    template <typename Func>
    void forEachActiveIndex(const size_t totalNodes, Func&& fn)
    {
        size_t i = 0;

        while (i < totalNodes)
        {
            const size_t word = i >> 6;
            const uint64_t combined = getCombinedShiftedWord(word, i & 63);

            if (!combined)
            {
                i = (word + 1) << 6;
                continue;
            }

            const unsigned offset = std::countr_zero(combined);
            i += offset;

            if (i >= totalNodes)
                break;

            fn(i);
            clearEventBit(i);
            ++i;
        }
    }
};
