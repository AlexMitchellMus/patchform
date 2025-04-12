#pragma once

class Chorus : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Chorus", "chorus", true);
    DEFINE_NODE_ALIASES("chorus");

    std::vector<float> delayBuffer;
    size_t bufferSize = 0;
    size_t writeIndex = 0;
    size_t sampleRate = 48000;

    std::atomic<float> baseDelayMs{6.0f};   // in ms
    std::atomic<float> depthMs{2.0f};       // modulation depth
    std::atomic<float> rateHz{0.25f};       // modulation speed
    std::atomic<float> feedback{0.0f};      // feedback amount [0, 0.95]

    FloatParameter* delayParam;
    FloatParameter* depthParam;
    FloatParameter* rateParam;
    FloatParameter* feedbackParam;

    float lfoPhase = 0.0f;
    float lfoInc = 0.0f;
    float lfoDir = 1.0f;

    float feedbackFiltered = 0.0f; // state for 1-pole LPF

public:
    Chorus(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::Signal, objParams)
    {
        addInputPort("In", AudioPort::Signal);

        delayParam = addParameter<FloatParameter>("Delay", baseDelayMs, 1.0f, 30.0f);
        depthParam = addParameter<FloatParameter>("Depth", depthMs, 0.0f, 10.0f);
        rateParam  = addParameter<FloatParameter>("Rate", rateHz, 0.01f, 5.0f);
        feedbackParam = addParameter<FloatParameter>("Feedback", feedback, 0.0f, 0.95f);

        sampleRate = static_cast<size_t>(context->sampleRate);
        bufferSize = sampleRate * 2; // 2 seconds safety
        delayBuffer.resize(bufferSize, 0.0f);

        delayParam->informNodeOfChange = [this]() {
            baseDelayMs.store(delayParam->getValue());
        };
        depthParam->informNodeOfChange = [this]() {
            depthMs.store(depthParam->getValue());
        };
        rateParam->informNodeOfChange = [this]() {
            rateHz.store(rateParam->getValue());
        };
        feedbackParam->informNodeOfChange = [this]() {
            feedback.store(feedbackParam->getValue());
        };
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        auto* in = inputPortBuffers[0]->getAudioBuffer();
        auto* out = outputPortBuffers[0]->getAudioBuffer();

        float delayMs = delayParam->getValue();
        float depth = depthParam->getValue();
        float rate = rateParam->getValue();
        float fb = feedback.load();

        float maxModMs = delayMs + depth;
        float maxDelaySamples = (maxModMs / 1000.0f) * sampleRate;

        lfoInc = rate / float(sampleRate);

        float lastOut = 0.0f;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            // triangle LFO
            float lfo = 2.0f * std::fabs(lfoPhase - 0.5f) - 1.0f;
            float modMs = delayMs + (lfo * depth);
            float modSamples = std::clamp((modMs / 1000.0f) * sampleRate, 1.0f, float(bufferSize - 2));

            float readIndex = writeIndex - modSamples;
            if (readIndex < 0) readIndex += bufferSize;

            int i1 = int(readIndex);
            if (i1 < 0) i1 += bufferSize;
            int i2 = (i1 + 1) % bufferSize;
            float frac = readIndex - int(readIndex);

            float delayed = (1.0f - frac) * delayBuffer[i1] + frac * delayBuffer[i2];

            // --- 1-pole lowpass filter for feedback ---
            // You can tweak alpha for smoother/stronger filtering (0.05–0.2 range typical)
            constexpr float alpha = 0.1f;
            feedbackFiltered = (1.0f - alpha) * feedbackFiltered + alpha * delayed;

            float input = in[i] + feedbackFiltered * fb;
            delayBuffer[writeIndex] = input;

            out[i] = (in[i] + delayed) * 0.5f;

            lastOut = delayed;

            writeIndex = (writeIndex + 1) % bufferSize;

            // triangle LFO update
            lfoPhase += lfoInc * lfoDir;
            if (lfoPhase >= 1.0f) {
                lfoPhase = 1.0f;
                lfoDir = -1.0f;
            }
            else if (lfoPhase <= 0.0f) {
                lfoPhase = 0.0f;
                lfoDir = 1.0f;
            }
        }
    }
};

REGISTER(Chorus);
