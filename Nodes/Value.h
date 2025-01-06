/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "AudioNodes.h"

#pragma once

// ValueNode that provides a constant value (e.g., for frequency modulation)
class Value : public AudioNode
{
    float value;

public:
    Value(NodeContext* context, float value) : AudioNode(context, "ValueNode"), value(value) {}

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = value;
        }
    }
};
