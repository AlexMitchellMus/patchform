#include "AudioNodes.h"

#pragma once

// AddNode that sums two signals
class Add : public AudioNode {

public:
    Add(NodeContext* context) : AudioNode(context, "AddNode")
    {
        addInputPort("A");
        addInputPort("B");
    }

    void processAudio(float* out, unsigned long frameCount) override {
        if (inputPorts.size() >= 2) {
            const float* buffer1 = inputPorts[0].sumPort().data();
            const float* buffer2 = inputPorts[1].sumPort().data();

            for (unsigned long i = 0; i < frameCount; i++) {
                outputPort.getAudioBuffer()[i] = buffer1[i] + buffer2[i];  // Directly write to output buffer
            }
        }
    }
};