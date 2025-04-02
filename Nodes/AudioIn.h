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
public:
    AudioIn(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        auto* outputPort = outputPortBuffers[0]->getAudioBuffer();

        std::memcpy(outputPort, in, frameCount * sizeof(float));
    }
};