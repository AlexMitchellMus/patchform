/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AdjacencyMap.h"

class Edge
{
    uint32_t oCanvas;
    uint32_t oNode;
    uint32_t oPort;

    uint32_t iCanvas;
    uint32_t iNode;
    uint32_t iPort;

    uint64_t conHash;

public:
    Edge(unsigned int oNode, unsigned int oPort, unsigned int iNode, unsigned int iPort)
        : iNode(iNode), iPort(iPort), oNode(oNode), oPort(oPort)
          , conHash(encodeHash(oNode, oPort, iNode, iPort))
    {
    }

    // New bit layout:
    // upper 32 bits = oNode (32)
    // next 8 bits   = oPort (8)
    // next 16 bits  = iNode (16)
    // next 8 bits   = iPort (8)
    static uint64_t encodeHash(uint32_t oNode, uint32_t oPort, uint32_t iNode, uint32_t iPort)
    {
        return  (static_cast<uint64_t>(oNode) << 32) |
                (static_cast<uint64_t>(oPort) << 24) |
                (static_cast<uint64_t>(iNode) << 8)  |
                (static_cast<uint64_t>(iPort));
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
