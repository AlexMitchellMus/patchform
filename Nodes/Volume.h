/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// VolumeNode that multiplies the outputs of two input nodes
class Volume : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Volume", "vol", NullParams);
public:
    Volume(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("A", AudioPort::PortType::Signal);
        addInputPort("B", AudioPort::PortType::Signal);
    }

    void processAudio(float* buffer, unsigned long frameCount) override
    {
        const float* buffer1 = inputPortBuffers[0]->getAudioBuffer();
        const float* buffer2 = inputPortBuffers[1]->getAudioBuffer();

        auto output = outputPort.getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = buffer1[i] * buffer2[i];  // Multiply the two input signals
        }
    }
};
