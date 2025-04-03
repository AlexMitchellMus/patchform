/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// AudioOutNode that outputs audio to the PortAudio stream
class AudioOut : public AudioNode {
    DEFINE_AND_REGISTER_NODE("AudioOut", "aout", true);
public:
    AudioOut(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        addInputPort("Signal", AudioPort::PortType::Signal);
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        // The input port audio is directly sent to the PortAudio stream
        const auto* inputPort = inputPortBuffers[0]->getAudioBuffer();

        // We can't copy here, because we may have multiple audio outs in the patch
        for (uint32_t i = 0; i < frameCount; i++)
            buffer[i] = std::clamp(buffer[i] + inputPort[i], -1.0f, 1.0f);
    }
};