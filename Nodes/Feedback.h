#pragma once

class Feedback : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Feedback", "feedback", true);
    DEFINE_NODE_ALIASES("feedback");

    std::vector<float> feedbackBuffer;
    float lastSample = 0.0f;
    std::atomic<float> amountVal = 0.0f;

    FloatParameter* feedbackAmountParam = nullptr;
public:
    bool canFeedback() override { return true; };

    Feedback(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);

        feedbackBuffer.resize(context->frameCount, 0.0f);

        amountVal.store(objParams.value("smooth", 0.0f));

        feedbackAmountParam = addParameter<FloatParameter>("smooth", amountVal, 0.0f, 1.0f);

        feedbackAmountParam->informNodeOfChange = [this]() {
            amountVal.store(feedbackAmountParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["smooth"] = amountVal.load();
        return nodeCreationData;
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* input = inputPortBuffers[0]->getAudioBuffer();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        float amount = std::clamp(amountVal.load(), 0.0f, 0.999f); // Clamp to prevent runaway
        float smoothed = std::isfinite(lastSample) ? lastSample : 0.0f;

        for (unsigned long i = 0; i < frameCount; ++i) {
            float raw = feedbackBuffer[i];

            // guard against overflow nan
            if (!std::isfinite(raw))
                raw = 0.0f;

            if (amount == 0.0f)
                smoothed = raw;
            else
                smoothed += amount * (raw - smoothed);

            // guard against overflow nan
            if (!std::isfinite(smoothed))
                smoothed = 0.0f;

            output[i] = smoothed;
        }

        lastSample = smoothed;
        std::copy_n(input, frameCount, feedbackBuffer.begin());
    }
};

REGISTER(Feedback);