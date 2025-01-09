/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodes.h"

class Envelope : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Envelope");

    float attackVal;
    float decayVal;
    float envValue = 0.0f;
    bool isAttack = true;  // Track whether the envelope is in attack phase

public:
    Envelope(NodeContext* context, float attackVal, float decayVal)
        : AudioNode(context, "Envelope")
        , attackVal(attackVal * (context->sampleRate / 1000))
        , decayVal(decayVal * (context->sampleRate / 1000))
    {
        addInputPort("Events");
        addInputPort("Signal");
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        auto output = outputPort.getAudioBuffer();
        auto events = inputPorts[0].combineEvents();
        auto signal = inputPorts[1].sumPort().data();

        for (unsigned long i = 0; i < frameCount; i++)
        {
            while (!events.empty() && events.front()->getTimeStamp() == i) {
                envValue = 0.0f;
                isAttack = true;
                context->eventPool.returnFreeEvent(events.front());
                events.erase(events.begin());
            }

            if (isAttack)
            {
                // Attack phase: Ramp up from 0 to 1
                envValue += (1.0f / attackVal);
                if (envValue >= 1.0f) {
                    envValue = 1.0f;
                    isAttack = false;  // Switch to decay phase after reaching 1
                }
            }
            else
            {
                // Decay phase: Ramp down from 1 towards 0
                envValue -= (1.0f / decayVal);
                if (envValue <= 0.0f) {
                    envValue = 0.0f;
                }
            }

            // Apply envelope to the signal
            output[i] = signal[i] * envValue;
        }
    }
};