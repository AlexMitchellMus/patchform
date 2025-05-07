/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

class DCBlock : public AudioNode {
    DEFINE_AND_REGISTER_NODE("DCBlock", "dcblock", true);
    DEFINE_NODE_ALIASES("dcblock");

    static constexpr int windowSize = 1024;
    float history[windowSize]{};
    int index = 0;
    float sum = 0.0f;

public:
    DCBlock(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* output = outputPortBuffers[0]->getAudioBuffer();
        const float* input = inputPortBuffers[0]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float old = history[index];
            float x = input[i];
            sum += x - old;
            history[index] = x;
            index = (index + 1) & (windowSize - 1); // faster- bitmask version of: index = (index + 1) % windowSize;
            output[i] = x - sum / windowSize;
        }
    }
};

REGISTER(DCBlock);
