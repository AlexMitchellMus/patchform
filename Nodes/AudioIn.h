/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AudioOutNode that outputs audio to the PortAudio stream
class AudioIn : public AudioNode {
    DEFINE_AND_REGISTER_NODE("AudioIn", "ain", true);
    DEFINE_NODE_ALIASES("ain", "audioin");
public:
    AudioIn(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::None, objParams)
    {
        addOutputPort("Left", AudioPort::Signal);
        addOutputPort("Right", AudioPort::Signal);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* left = outputPortBuffers[0]->getAudioBuffer();
        auto* right = outputPortBuffers[1]->getAudioBuffer();

        //TODO: Make a way to know how many channels we have!
        const float* inR = in + frameCount;

        std::memcpy(left, in, frameCount * sizeof(float));
        std::memcpy(right, inR, frameCount * sizeof(float));
    }
};

REGISTER(AudioIn);