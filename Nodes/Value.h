/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

struct ValueState : public AudioNode::StateBase
{
    float value = 0.0f;

    std::unique_ptr<StateBase> clone() const override
    {
        return std::make_unique<ValueState>(*this); // Copy this ValueState
    };
};

// ValueNode that provides a constant value (e.g., for frequency modulation)
class Value : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Value");

public:
    Value(NodeContext* context) : AudioNode(std::make_unique<ValueState>(), context, AudioPort::PortType::Signal){}

    void setValue(float value)
    {
        //std::cout << "setting value to : " << value << std::endl;
        auto* currentState = dynamic_cast<ValueState*>(activeState);
        currentState->value = value;
        //setDirty();
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto output = outputPort.getAudioBuffer();
        auto* currentState = dynamic_cast<ValueState*>(activeState);
        for (unsigned long i = 0; i < frameCount; i++) {
            output[i] = currentState->value;
        }
    }
};
