/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

// AddNode that sums two signals
class Print : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Print");

public:
    Print(NodeContext* context) : AudioNode(context, "Print")
    {
        addInputPort("A"); // hot port
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].combineEvents();

        for (auto event : aEvents)
        {
            std::cout << "Print: " << event->data << std::endl;
            context->eventPool.returnFreeEvent(event);
        }
    }
};