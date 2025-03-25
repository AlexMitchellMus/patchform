/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// ValueNode that provides a constant value (e.g., for frequency modulation)
class Value : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Value", "val");

    float value = 0.0f;

public:
    Value(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        value = objParams.value("value", 0.0f);
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        const auto output = outputPortBuffers[0]->getAudioBuffer();
        std::fill_n(output, frameCount, value);
    }
};
