#include "AudioNodes.h"

#pragma once

// SineWaveNode that generates sine wave audio
class SineWaveNode : public AudioNode {
    static constexpr int TABLE_SIZE = 4096;
    static float sineTable[TABLE_SIZE];

    float phase = 0.0f;

public:
    SineWaveNode(NodeContext* context) : AudioNode(context, "SineWaveNode")
    {
        addInputPort("frequency");

        // Initialize the table only once
        static bool initialized = false;
        if (!initialized) {
            for (int i = 0; i < TABLE_SIZE; i++) {
                sineTable[i] = std::sin(2.0f * M_PI * (float)i / (float)TABLE_SIZE);
            }
            initialized = true;
        }
    }

    void processAudio(float* out, unsigned long frameCount) override {
        auto freqIn = inputPorts[0].sumPort();     // Frequency input
        auto output = outputPort.getAudioBuffer(); // Node's output buffer

        for (unsigned long i = 0; i < frameCount; i++) {
            // Convert current phase to an integer index
            int idx = static_cast<int>(phase);

            // Clamp index in case of any floating error
            if (idx < 0)
                idx = 0;

            if (idx >= TABLE_SIZE)
                idx = TABLE_SIZE - 1;

            // Write the sine value
            output[i] = 0.5f * sineTable[idx];

            // Increment phase based on frequency and sample rate
            float phaseInc = (TABLE_SIZE * freqIn[i]) / context->sampleRate;
            phase += phaseInc;

            // Wrap phase within [0, TABLE_SIZE)
            while (phase >= TABLE_SIZE)
                phase -= TABLE_SIZE;

            while (phase < 0.0f)
                phase += TABLE_SIZE;
        }
    }
};

float SineWaveNode::sineTable[SineWaveNode::TABLE_SIZE];