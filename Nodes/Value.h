/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

class ValueState : public AudioNode::StateBase
{
public:
    ValueState(float val) : value(val){};
    float value = 0.0f;

    ENABLE_COPY(ValueState);
};

// ValueNode that provides a constant value (e.g., for frequency modulation)
class Value : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Value", "val");

public:
    Value(NodeContext* context, float val) : AudioNode(std::make_unique<ValueState>(val), context, AudioPort::PortType::Signal){}

    void setValue(float value)
    {
        auto* currentState = dynamic_cast<ValueState*>(activeState);
        currentState->value = value;
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();
        auto* currentState = dynamic_cast<ValueState*>(activeState);
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = currentState->value;
        }
    }
};
