/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

class Envelope : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Envelope", "env");

    float attackVal;
    float decayVal;
    float envValue = 0.0f;
    bool isAttack = false;  // Track whether the envelope is in attack phase

public:
    Envelope(NodeContext* context, const json& nodeData)
        : AudioNode(std::make_unique<NullState>(), context, AudioPort::PortType::Signal, nodeData)
    {
        addInputPort("Events", AudioPort::PortType::Data);
        addInputPort("Signal", AudioPort::PortType::Signal);

        attackVal = nodeData.value("attack", 0.0f) * (context->sampleRate / 1000);
        decayVal = nodeData.value("decay", 0.0f) * (context->sampleRate / 1000);
    }

    void processAudio(float* out, const unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();
        auto signal = inputPortBuffers[1]->getAudioBuffer();;
        auto output = outputPort.getAudioBuffer();

        std::vector<Event*> toRelease;
        unsigned long nextEventIndex = 0;

        for (unsigned long i = 0; i < frameCount; i++)
        {
            while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i) {
                envValue = 0.0f;
                isAttack = true;
                nextEventIndex++;
                //std::cout << "Envelope " << nodeID << " triggered" << std::endl;
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
        //for (auto e : toRelease)
        //{
        //    //context->eventPool.releaseEvent(e);
        //}
    }
};