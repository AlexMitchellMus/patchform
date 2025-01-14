#pragma once

#include <cstdint>
#include <iostream>

namespace PortHelpers
{
    // Helper function to combine nodeID and portID into a single uint32_t
    static inline const uint32_t getKey(uint16_t nodeID, uint16_t portID) {
        return (static_cast<uint32_t>(nodeID) << 16) | static_cast<uint32_t>(portID);
    }

    // Helper function to extract nodeID from a combined uint32_t
    static inline const uint16_t getNodeID(uint32_t combined) {
        return static_cast<uint16_t>(combined >> 16);
    }

    // Helper function to extract portID from a combined uint32_t
    static inline const uint16_t getPortID(uint32_t combined) {
        return static_cast<uint16_t>(combined & 0xFFFF);
    }

    // Helper function to extract both nodeID and portID as a tuple
    static inline std::tuple<uint16_t, uint16_t> getNodeAndPortID(uint32_t combined) {
        return std::make_tuple(getNodeID(combined), getPortID(combined));
    }
}