/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Print : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Print");

public:
    Print(NodeContext* context) : AudioNode(std::make_unique<AudioNode::NullState>(), context, AudioPort::PortType::None)
    {
        addInputPort("A"); // hot port
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPorts[0].sumEvents();

        for (auto event : aEvents)
        {
            // TODO: do not print from audio callback, this is for early testing only
            // Use a lockfree queue
            Logger::getInstance().logEvent(this, event->getTimeStamp(), event->data);
        }
    }
};