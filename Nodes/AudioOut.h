#include "AudioNodes.h"

#pragma once

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOut : public AudioNode {
public:
    AudioOut(NodeContext* context) : AudioNode(context, "AudioOutNode")
    {
        addInputPort("Signal");
    }

    void processAudio(float* buffer, unsigned long frameCount) override {
        // The input port audio is directly sent to the PortAudio stream
        auto inputPort = inputPorts[0].sumPort().data();
        std::copy(inputPort, inputPort + frameCount, buffer);
    }
};