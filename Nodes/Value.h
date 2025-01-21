/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

struct ValueData
{
    float value = 0.0f;
};

// ValueNode that provides a constant value (e.g., for frequency modulation)
class Value : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Value", "val", ValueData);

    float value = 0.0f;

public:
    Value(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        parseObjectParams(paramData);
        value = paramData.value;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto output = outputPort.getAudioBuffer();
        std::fill(output, output+frameCount, value);
    }
};
