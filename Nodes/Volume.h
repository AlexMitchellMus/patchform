/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// VolumeNode that multiplies the outputs of two input nodes
class Volume : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Volume", "vol");
public:
    Volume(NodeContext* context) : AudioNode(std::make_unique<NullState>(), context, AudioPort::PortType::Signal)
    {
        addInputPort("A");
        addInputPort("B");
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        sumInputBuffers(inputPortBuffers);

        //const float* buffer1 = inputPorts[0].sumAudio().data();
        //const float* buffer2 = inputPorts[1].sumAudio().data();

        const float* buffer1 = inputPortBuffers[0].data();
        const float* buffer2 = inputPortBuffers[1].data();

        auto output = outputPort.getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = buffer1[i] * buffer2[i];  // Multiply the two input signals
        }
    }
};
