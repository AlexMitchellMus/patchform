/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AddNode that sums two signals
class Add_Audio : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Add_Audio", "aadd", NullParams);

public:
    Add_Audio(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("A", AudioPort::PortType::Signal);
        addInputPort("B", AudioPort::PortType::Signal);
    }

    void processAudio(float* out, unsigned long frameCount) override {
        const float* buffer1 = inputPortBuffers[0]->getAudioBuffer();
        const float* buffer2 = inputPortBuffers[1]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            outputPort.getAudioBuffer()[i] = buffer1[i] + buffer2[i]; // Directly write to output buffer
        }
    }
};