#pragma once

#include "AudioNodeBase.h"

class Chorus : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Chorus", "chorus", true);
    DEFINE_NODE_ALIASES("chorus");

    std::vector<float> delayBuffer;
    size_t bufferSize = 0;
    size_t writeIndex = 0;
    size_t sampleRate = 48000;

    std::atomic<float> baseDelayMs{6.0f};
    std::atomic<float> depthMs{2.0f};
    std::atomic<float> rateHz{0.25f};
    std::atomic<float> feedback{0.0f};

    FloatParameter* delayParam;
    FloatParameter* depthParam;
    FloatParameter* rateParam;
    FloatParameter* feedbackParam;

    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
    float lfoDir = 1.0f;

    float feedbackFiltered = 0.0f;
    float smoothedDelayMs = 6.0f;

public:
    Chorus(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::Signal, objParams)
    {
        addInputPort("In", AudioPort::Signal);

        baseDelayMs = objParams.value("Delay", 6.0f);
        depthMs = objParams.value("Depth", 2.0f);
        rateHz = objParams.value("Rate", 0.25f);
        feedback = objParams.value("Feedback", 0.0f);

        delayParam = addParameter<FloatParameter>("Delay", baseDelayMs, 1.0f, 30.0f);
        depthParam = addParameter<FloatParameter>("Depth", depthMs, 0.0f, 10.0f);
        rateParam  = addParameter<FloatParameter>("Rate", rateHz, 0.01f, 5.0f);
        feedbackParam = addParameter<FloatParameter>("Feedback", feedback, 0.0f, 0.95f);

        sampleRate = static_cast<size_t>(context->sampleRate);
        bufferSize = sampleRate * 2;
        delayBuffer.resize(bufferSize, 0.0f);

        delayParam->informNodeOfChange = [this]() { baseDelayMs.store(delayParam->getValue()); };
        depthParam->informNodeOfChange = [this]() { depthMs.store(depthParam->getValue()); };
        rateParam->informNodeOfChange  = [this]() { rateHz.store(rateParam->getValue()); };
        feedbackParam->informNodeOfChange = [this]() { feedback.store(feedbackParam->getValue()); };
    }

    json getSerializedNode() override
    {
        nodeCreationData["Delay"] = baseDelayMs.load();
        nodeCreationData["Depth"] = depthMs.load();
        nodeCreationData["Rate"] = rateHz.load();
        nodeCreationData["Feedback"] = feedback.load();
        return nodeCreationData;
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* in = inputPortBuffers[0]->getAudioBuffer();
        auto* out = outputPortBuffers[0]->getAudioBuffer();

        // Smoothed parameters (block smoothing = low CPU)
        constexpr float smoothing = 0.01f;
        smoothedDelayMs += (baseDelayMs.load() - smoothedDelayMs) * smoothing;
        float delayMs = smoothedDelayMs;

        float depth = depthMs.load();
        float rate = rateHz.load();
        float fb = feedback.load();

        lfoInc = rate / sampleRate;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float lfo = 2.0f * std::fabs(lfoPhase - 0.5f) - 1.0f;
            float modMs = delayMs + (lfo * depth);
            float modSamples = std::clamp((modMs / 1000.0f) * sampleRate, 1.0f, float(bufferSize - 2));

            float readIndex = writeIndex - modSamples;
            if (readIndex < 0) readIndex += bufferSize;

            int i1 = int(readIndex);
            int i2 = (i1 + 1) % bufferSize;
            float frac = readIndex - i1;

            float delayed = (1.0f - frac) * delayBuffer[i1] + frac * delayBuffer[i2];

            constexpr float alpha = 0.1f;
            feedbackFiltered = (1.0f - alpha) * feedbackFiltered + alpha * delayed;

            float input = in[i] + feedbackFiltered * fb;
            delayBuffer[writeIndex] = input;

            out[i] = (in[i] + delayed) * 0.5f;

            writeIndex = (writeIndex + 1) % bufferSize;

            lfoPhase += lfoInc * lfoDir;
            if (lfoPhase >= 1.0f) {
                lfoPhase = 1.0f;
                lfoDir = -1.0f;
            } else if (lfoPhase <= 0.0f) {
                lfoPhase = 0.0f;
                lfoDir = 1.0f;
            }
        }
    }
};

REGISTER(Chorus);
