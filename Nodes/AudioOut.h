/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOut : public AudioNode {
    DEFINE_AND_REGISTER_NODE("AudioOut");
public:
    AudioOut(NodeContext* context) : AudioNode(context, "AudioOutNode", AudioPort::PortType::None)
    {
        addInputPort("Signal");
    }

    void processAudio(float* buffer, unsigned long frameCount) override
    {
        // The input port audio is directly sent to the PortAudio stream
        auto inputPort = inputPorts[0].sumAudio().data();
        std::copy(inputPort, inputPort + frameCount, buffer);
    }
};