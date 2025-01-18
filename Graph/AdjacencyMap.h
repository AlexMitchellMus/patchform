/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <limits>
#include <iostream>
#include <iomanip>
#include <cassert>

#include "unordered_dense.h"

class AdjacencyMap
{
public:
    // Custom Adjacency List definition using keys: node:port packed int
    using AdjacencyList = ankerl::unordered_dense::map<uint32_t, std::vector<uint32_t>>;

    // Pack a target node and port into a single uint32_t key
    static constexpr uint32_t packKey(uint32_t nodeID, uint8_t portID)
    {
        return (nodeID << 6) | portID;
    }

    // Unpack a key into nodeID and portID
    static constexpr std::pair<uint32_t, uint8_t> unpackKey(uint32_t key)
    {
        return {key >> 6, static_cast<uint8_t>(key & 0x3F)};
    }

    // Helper function to extract nodeID from a combined uint32_t
    static uint32_t getNodeID(uint32_t combined) {
        return static_cast<uint32_t>(combined >> 6);
    }

    // Helper function to extract portID from a combined uint32_t
    static constexpr uint8_t getPortID(uint32_t combined) {
        return static_cast<uint8_t>(combined & 0x3F); // Mask the lower 6 bits
    }

    void clear()
    {
        forward.clear();
        backward.clear();
    }

    void addAdjacency(uint32_t inputKey, uint32_t outputKey)
    {
        forward[outputKey].emplace_back(inputKey);
        backward[inputKey].emplace_back(outputKey);
    }

    void removeAdjacency(uint32_t inputKey, uint32_t outputKey)
    {
        std::cerr << "removeAdjacency is depreciated" << std::endl;
        return;
        // Remove inputKey from forward[outputKey]
        auto forwardIt = forward.find(outputKey);
        if (forwardIt != forward.end())
        {
            auto& inputs = forwardIt->second;
            inputs.erase(std::remove(inputs.begin(), inputs.end(), inputKey), inputs.end());

            // If the vector becomes empty, erase the entry from the map
            if (inputs.empty())
            {
                forward.erase(forwardIt);
            }
        }

        // Remove outputKey from backward[inputKey]
        auto backwardIt = backward.find(inputKey);
        if (backwardIt != backward.end())
        {
            auto& outputs = backwardIt->second;
            outputs.erase(std::remove(outputs.begin(), outputs.end(), outputKey), outputs.end());

            // If the vector becomes empty, erase the entry from the map
            if (outputs.empty())
            {
                backward.erase(backwardIt);
            }
        }
    }

    [[nodiscard]] const AdjacencyList& getForward() const
    {
        return forward;
    }

    [[nodiscard]] const AdjacencyList& getBackward() const
    {
        return backward;
    }

    bool containsAdjacency(uint32_t inputKey, uint32_t outputKey) const
    {
        std::cerr << "containsAdjacency is depreciated" << std::endl;
        return false;
        auto it = forward.find(outputKey);
        if (it != forward.end())
        {
            const auto& inputs = it->second;
            return std::find(inputs.begin(), inputs.end(), inputKey) != inputs.end();
        }
        return false;
    }

    AdjacencyList forward{};
    AdjacencyList backward{};
};
