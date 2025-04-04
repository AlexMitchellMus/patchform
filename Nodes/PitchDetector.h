#pragma once

#include "AudioNodeBase.h"
#include <cmath>
#include <array>
#include <algorithm>
#include "PitchMPM.h"

class PitchDetector : public AudioNode {
    DEFINE_AND_REGISTER_NODE("PitchDetect", "pitch", true);

    static constexpr size_t WINDOW_SIZE = 4096;

    float smoothedPitch = 0.0f;
    float smoothingFactor = 0.3f;
    float clarityThreshold = 0.7f;

    std::array<float, WINDOW_SIZE> buffer{};
    size_t writeHead = 0;
    PitchMPM pitchDetector;

public:
    PitchDetector(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams),
          pitchDetector(WINDOW_SIZE) {

        pitchDetector.setSampleRate(context->sampleRate);
        addInputPort("in", AudioPort::Signal);
    }

    void processAudio(const float* in, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const float* input = inputPortBuffers[0]->getAudioBuffer();

        for (size_t i = 0; i < frameCount; ++i) {
            buffer[writeHead] = input[i];
            writeHead = (writeHead + 1) % WINDOW_SIZE;
        }

        std::array<float, WINDOW_SIZE> linearBuffer;
        for (size_t i = 0; i < WINDOW_SIZE; ++i)
            linearBuffer[i] = buffer[(writeHead + i) % WINDOW_SIZE];

        float freq = pitchDetector.getPitch(linearBuffer.data());

        if (freq < 20.0f || freq > 5000.0f || !std::isfinite(freq))
            return;

        float pitch = 69.0f + 12.0f * std::log2(freq / 440.0f);
        if (!std::isfinite(pitch)) return;

        smoothedPitch = (smoothedPitch == 0.0f) ? pitch : (1.0f - smoothingFactor) * smoothedPitch + smoothingFactor * pitch;

        if (Event* ev = context->eventPool.getFreeEvent()) {
            context->eventPool.addDataAtomTo(ev, smoothedPitch);
            outputPortBuffers[0]->addEvent(ev);
        }
    }
};
