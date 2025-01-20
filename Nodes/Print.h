/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Print : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Print", "pnt");

public:
    Print(NodeContext* context, const json& nodeData) : AudioNode(context, AudioPort::PortType::None, nodeData)
    {
        addInputPort("A", AudioPort::PortType::Data); // hot port
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto aEvents = inputPortBuffers[0]->getEvents();

        for (auto event : aEvents)
        {
            Logger::getInstance().logEvent(this, event->getTimeStamp(), event->data);
        }
    }
};