/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../AudioNodeBase.h"

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOut : public AudioNode {
    DEFINE_AND_REGISTER_NODE("AudioOut", "aout", true);
    DEFINE_NODE_ALIASES("aout", "audioout");
public:
    AudioOut(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::None, objParams)
    {
        addInputPort("Left", AudioPort::Signal);
        addInputPort("Right", AudioPort::Signal);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* left = inputPortBuffers[0]->getAudioBuffer();
        auto* right = inputPortBuffers[1]->getAudioBuffer();

        //TODO: Make a way to know how many channels we have!
        float* bufferR = buffer + frameCount;

        for (uint32_t i = 0; i < frameCount; ++i)
        {
            buffer[i]     = buffer[i] + left[i];
            bufferR[i]    = bufferR[i] + right[i];
        }
    }
};

REGISTER(AudioOut);