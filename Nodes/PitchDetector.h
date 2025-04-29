#pragma once

#include "AudioNodeBase.h"
#include <cmath>
#include <array>
#include <algorithm>
#include <cstring>

class PitchDetector final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("PitchDetector", "pitch", true);
    DEFINE_NODE_ALIASES("pitchdetect");

    static constexpr size_t WINDOW_SIZE = 4096;
    static constexpr size_t MIN_LAG = 16;
    static constexpr size_t MAX_LAG = WINDOW_SIZE / 2;
    static constexpr float MIN_VOLUME_THRESHOLD = 1e-6f;

    float smoothedPitch = 0.0f;
    float smoothingFactor = 0.3f;
    float clarityThreshold = 0.7f;

    std::array<float, WINDOW_SIZE> buffer{};
    size_t writeHead = 0;
    int processCounter = 0;
    static constexpr int PROCESS_INTERVAL = 8;

public:
    PitchDetector(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams) {
        addInputPort("in", AudioPort::Signal);
    }

    float yin_detect_pitch(const float* input, size_t size, float sampleRate, float threshold, float& outClarity) {
        std::array<float, WINDOW_SIZE> yin{};

        for (size_t tau = 1; tau < size; ++tau) {
            float sum = 0.0f;
            for (size_t i = 0; i < size - tau; ++i) {
                float diff = input[i] - input[i + tau];
                sum += diff * diff;
            }
            yin[tau] = sum;
        }

        yin[0] = 1.0f;
        float runningSum = 0.0f;
        for (size_t tau = 1; tau < size; ++tau) {
            runningSum += yin[tau];
            yin[tau] *= tau / runningSum;
        }

        size_t tauEstimate = 0;
        for (size_t tau = MIN_LAG; tau < MAX_LAG; ++tau) {
            if (yin[tau] < threshold) {
                while (tau + 1 < MAX_LAG && yin[tau + 1] < yin[tau])
                    tau++;
                tauEstimate = tau;
                break;
            }
        }

        if (tauEstimate == 0 || tauEstimate >= size) {
            outClarity = 0.0f;
            return -1.0f;
        }

        outClarity = 1.0f - yin[tauEstimate];
        return sampleRate / tauEstimate;
    }

    void processAudio(const float* in, float*, const unsigned long frameCount, std::vector<MidiMessage>&) override {
        const float* input = inputPortBuffers[0]->getAudioBuffer();

        // Shift buffer contents back by frameCount
        std::memmove(buffer.data(), buffer.data() + frameCount, sizeof(float) * (WINDOW_SIZE - frameCount));
        // Copy new audio samples to the end of the buffer
        std::memcpy(buffer.data() + (WINDOW_SIZE - frameCount), input, sizeof(float) * frameCount);

        if (++processCounter < PROCESS_INTERVAL) return;
        processCounter = 0;

        float energy = 0.0f;
        for (float sample : buffer)
            energy += sample * sample;

        if (energy / WINDOW_SIZE < MIN_VOLUME_THRESHOLD)
            return;

        float clarity = 0.0f;
        float freq = yin_detect_pitch(buffer.data(), WINDOW_SIZE, context->sampleRate, 0.15f, clarity);

        if (clarity < clarityThreshold || freq < 8.0f || freq > 5000.0f || !std::isfinite(freq))
            return;

        float pitch = 69.0f + 12.0f * std::log2(freq / 440.0f);
        if (!std::isfinite(pitch)) return;

        smoothedPitch = (smoothedPitch == 0.0f) ? pitch : (1.0f - smoothingFactor) * smoothedPitch + smoothingFactor * pitch;

        if (Event* ev = context->eventPool.getFreeEvent()) {
            context->eventPool.addDataAtomTo(ev, smoothedPitch);
            addEvent(0, ev);
        }
    }
};

REGISTER(PitchDetector);
