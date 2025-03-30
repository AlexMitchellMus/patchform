/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

// Drive node with soft clipping
class Drive : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Drive", "drive", true);

    std::atomic<float> driveAmount;
    FloatParameter* driveAmountParameter;

public:
    Drive(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);

        driveAmount.store(objParams.value("drive", 1.0f));
        driveAmountParameter = addParameter<FloatParameter>("drive", driveAmount, 0.0f, 100.0f);

        driveAmountParameter->informNodeOfChange = [this]()
        {
            driveAmount.store(driveAmountParameter->getValue());
        };
    }

    void processAudio(float* buffer, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const auto* input = inputPortBuffers[0]->getAudioBuffer();
        auto* output = outputPortBuffers[0]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float x = input[i] * driveAmount.load();
            output[i] = std::tanh(x); // Soft clipping
        }
    }
};
