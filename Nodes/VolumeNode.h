#include "AudioNodes.h"

#pragma once

// VolumeNode that multiplies the outputs of two input nodes
class VolumeNode : public AudioNode {
public:
    VolumeNode(NodeContext* context) : AudioNode(context, "VolumeNode")
    {
        addInputPort("A");
        addInputPort("B");
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        const float* buffer1 = inputPorts[0].sumPort().data();
        const float* buffer2 = inputPorts[1].sumPort().data();
        auto output = outputPort.getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = buffer1[i] * buffer2[i];  // Multiply the two input signals
        }
    }
};
