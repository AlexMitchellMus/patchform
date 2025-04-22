#pragma once

class Feedback : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Feedback", "feedback", true);
    DEFINE_NODE_ALIASES("feedback");

    std::vector<float> feedbackBuffer;

public:
    Feedback(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);

        feedbackBuffer.resize(context->frameCount, 0.0f);
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* input = inputPortBuffers[0]->getAudioBuffer();
        float* output = outputPortBuffers[0]->getAudioBuffer();

        std::copy_n(feedbackBuffer.begin(), frameCount, output);
        std::copy_n(input, frameCount, feedbackBuffer.begin());
    }
};

REGISTER(Feedback);