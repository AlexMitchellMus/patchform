// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include "AudioNodeBase.h"
#include <vector>
#include <atomic>

class MultiTapDelay : public AudioNode {
    DEFINE_AND_REGISTER_NODE("MultiTapDelay", "multitap_delay", true);
    DEFINE_NODE_ALIASES("multitapdelay");

    static constexpr size_t kMaxDelayMs = 2000;

    std::vector<float> delayBuffer;
    size_t q = 0;
    size_t totalDelay = 1;
    size_t sampleRate = 48000;

    std::atomic<float> d1Ms, d2Ms;
    std::atomic<float> b0, b1, b2, a1, a2;

    FloatParameter* d1Param;
    FloatParameter* d2Param;
    FloatParameter* b0Param;
    FloatParameter* b1Param;
    FloatParameter* b2Param;
    FloatParameter* a1Param;
    FloatParameter* a2Param;

    // Precomputed delays in samples (safe and updated when params change)
    std::atomic<size_t> d1Samples, d2Samples;

public:
    MultiTapDelay(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);

        d1Ms = objParams.value("d1", 125.0f);
        d2Ms = objParams.value("d2", 250.0f);
        b0 = objParams.value("b0", 1.0f);
        b1 = objParams.value("b1", 1.0f);
        b2 = objParams.value("b2", 1.0f);
        a1 = objParams.value("a1", 0.2f);
        a2 = objParams.value("a2", 0.4f);

        d1Param = addParameter<FloatParameter>("d1", d1Ms, 1.0f, 1000.0f);
        d2Param = addParameter<FloatParameter>("d2", d2Ms, 1.0f, 1000.0f);
        b0Param = addParameter<FloatParameter>("b0", b0, 0.0f, 2.0f);
        b1Param = addParameter<FloatParameter>("b1", b1, 0.0f, 2.0f);
        b2Param = addParameter<FloatParameter>("b2", b2, 0.0f, 2.0f);
        a1Param = addParameter<FloatParameter>("a1", a1, 0.0f, 1.0f);
        a2Param = addParameter<FloatParameter>("a2", a2, 0.0f, 1.0f);

        sampleRate = static_cast<size_t>(context->sampleRate);
        totalDelay = (kMaxDelayMs * sampleRate) / 1000 + 1;
        delayBuffer.resize(totalDelay, 0.0f); // one-time allocation

        // Param change hooks — no allocation, just recompute
        d1Param->informNodeOfChange = [this]() {
            float ms = std::clamp(d1Param->getValue(), 1.0f, float(kMaxDelayMs));
            d1Ms.store(ms);
            d1Samples.store(std::min(static_cast<size_t>((ms / 1000.0f) * sampleRate), totalDelay - 1));
        };

        d2Param->informNodeOfChange = [this]() {
            float ms = std::clamp(d2Param->getValue(), 1.0f, float(kMaxDelayMs));
            d2Ms.store(ms);
            d2Samples.store(std::min(static_cast<size_t>((ms / 1000.0f) * sampleRate), totalDelay - 1));
        };

        for (auto* param : { b0Param, b1Param, b2Param, a1Param, a2Param }) {
            param->informNodeOfChange = [param]() {
                // Nothing to precompute, but leave hook for UI sync or validation
                param->getValue(); // noop to trigger update
            };
        }

        // Initial precompute
        d1Param->informNodeOfChange();
        d2Param->informNodeOfChange();
    }

    void processAudio(const float* in, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* input = inputPortBuffers[0]->getAudioBuffer();
        auto* output = outputPortBuffers[0]->getAudioBuffer();

        float _b0 = b0.load(), _b1 = b1.load(), _b2 = b2.load();
        float _a1 = a1.load(), _a2 = a2.load();

        size_t d1 = d1Samples.load();
        size_t d2 = d2Samples.load();

        size_t tap1 = (q + totalDelay - d1) % totalDelay;
        size_t tap2 = (q + totalDelay - d1 - d2) % totalDelay;

        for (unsigned long i = 0; i < frameCount; ++i) {
            float s = input[i];
            float s1 = delayBuffer[tap1];
            float s2 = delayBuffer[tap2];
            float y = _b0 * s + _b1 * s1 + _b2 * s2;
            delayBuffer[q] = s + _a1 * s1 + _a2 * s2;
            output[i] = y;

            q = (q + 1) % totalDelay;
            tap1 = (q + totalDelay - d1) % totalDelay;
            tap2 = (q + totalDelay - d1 - d2) % totalDelay;
        }
    }
};

REGISTER(MultiTapDelay);