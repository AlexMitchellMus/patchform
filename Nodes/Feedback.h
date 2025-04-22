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

    Feedback(NodeContext* context, const json& objParams)
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

        float amount = amountVal.load();

        for (unsigned long i = 0; i < frameCount; ++i) {
            float avg = 0.5f * (lastSample + feedbackBuffer[i]);
            output[i] = amount * avg + (1.0f - amount) * feedbackBuffer[i];
            lastSample = feedbackBuffer[i];
        }

        std::copy_n(input, frameCount, feedbackBuffer.begin());
    }
};

REGISTER(Feedback);