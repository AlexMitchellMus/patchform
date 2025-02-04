/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

// VolumeNode that multiplies the outputs of two input nodes
class Volume : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Volume", "vol");

    float eventVal1 = 0.0f;
    float eventVal2 = 0.0f;

public:
    Volume(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("A", AudioPort::PortType::Signal);
        addInputPort("B", AudioPort::PortType::Signal);
    }

    void processAudio(float* buffer, unsigned long frameCount) override
    {
        const auto buffer1   = inputPortBuffers[0]->getAudioBuffer();
        const auto buffer2   = inputPortBuffers[1]->getAudioBuffer();
        const auto volEvents1 = inputPortBuffers[0]->getEvents();
        const auto volEvents2 = inputPortBuffers[1]->getEvents();
        bool useSignalFreq1   = inputPortBuffers[0]->isAnyConnectedPortSignal;
        bool useSignalFreq2   = inputPortBuffers[1]->isAnyConnectedPortSignal;

        const auto output = outputPort.getAudioBuffer();

        unsigned int nextVolEventIndex1 = 0;
        unsigned int nextVolEventIndex2 = 0;

        for (unsigned long i = 0; i < frameCount; i++)
        {
            if (!useSignalFreq1 && useSignalFreq2)
            {
                while (nextVolEventIndex1 < volEvents1.size() && volEvents1[nextVolEventIndex1]->getTimeStamp() == i)
                {
                    eventVal1 = volEvents1[nextVolEventIndex1]->data;
                    nextVolEventIndex1++;
                }
                output[i] = eventVal1 * buffer2[i];
            }
            else if (useSignalFreq1 && !useSignalFreq2)
            {
                while (nextVolEventIndex2 < volEvents2.size() && volEvents2[nextVolEventIndex2]->getTimeStamp() == i)
                {
                    eventVal2 = volEvents2[nextVolEventIndex2]->data;
                    nextVolEventIndex2++;
                }

                output[i] = buffer1[i] * eventVal2;
            }
            else if (!useSignalFreq1)
            {
                while (nextVolEventIndex1 < volEvents1.size() && volEvents1[nextVolEventIndex1]->getTimeStamp() == i)
                {
                    eventVal1 = volEvents1[nextVolEventIndex1]->data;
                    nextVolEventIndex1++;
                }
                while (nextVolEventIndex2 < volEvents2.size() && volEvents2[nextVolEventIndex2]->getTimeStamp() == i)
                {
                    eventVal2 = volEvents2[nextVolEventIndex2]->data;
                    nextVolEventIndex2++;
                }
                output[i] = eventVal1 * eventVal2;
            }
            else
            {
                output[i] = buffer1[i] * buffer2[i];
            }
        }
    }
};
