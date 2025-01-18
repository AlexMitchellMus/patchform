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
#include <stack>
#include <chrono>

#include "AdjacencyMap.h"

class Connection
{
    uint32_t oCanvas;
    uint32_t oNode;
    uint32_t oPort;

    uint32_t iCanvas;
    uint32_t iNode;
    uint32_t iPort;

    uint64_t conHash;

public:
    Connection(unsigned int oNode, unsigned int oPort, unsigned int iNode, unsigned int iPort)
        : iNode(iNode), iPort(iPort), oNode(oNode), oPort(oPort)
          , conHash(encodeHash(oNode, oPort, iNode, iPort))
    {
    }

    static uint64_t encodeHash(unsigned int oNode, unsigned int oPort, unsigned int iNode, unsigned int iPort)
    {
        // Encode the values into a 64-bit hash
        return (static_cast<uint64_t>(oNode) << 48) | (static_cast<uint64_t>(oPort) << 40) |
            (static_cast<uint64_t>(iNode) << 24) | (static_cast<uint64_t>(iPort) << 16);
    }

    uint64_t getHash() const
    {
        return conHash;
    }

    unsigned int getoNode()
    {
        return oNode;
    }

    unsigned int getoPort()
    {
        return oPort;
    }

    unsigned int getiNode()
    {
        return iNode;
    }

    unsigned int getiPort()
    {
        return iPort;
    }

    std::string toString()
    {
        return "oNode: " + std::to_string(oNode) +
              " oPort: " + std::to_string(oPort) +
              " iNode: " + std::to_string(iNode) +
              " iPort: " + std::to_string(iPort);
    }

    constexpr uint32_t getOut() const
    {
        return AdjacencyMap::packKey(oNode, oPort);
    }

    constexpr uint32_t getIn() const
    {
        return AdjacencyMap::packKey(iNode, iPort);
    }
};
