#pragma once

class Delay : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Delay", "delay", true);
    DEFINE_NODE_ALIASES("delay");

    std::vector<float> buffer;
    size_t writePos = 0;

    float maxDelayMs = 1000.0f; // default max
    std::atomic<float> delayMsVal = 0.0f;
    FloatParameter* delayParam = nullptr;

    size_t bufferSize = 0;
    float sampleRate = 44100.0f;

public:
    Delay(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("In", AudioPort::PortType::Signal);
        addInputPort("DelayTime", AudioPort::PortType::Signal);

        sampleRate = context->sampleRate;
        maxDelayMs = objParams.value("maxDelayMs", 1000.0f);
        float initialDelay = objParams.value("delayMs", 0.0f);

        delayMsVal.store(initialDelay);

        // Allocate circular buffer
        bufferSize = static_cast<size_t>((maxDelayMs / 1000.0f) * sampleRate) + 2; // +2 for safety
        buffer.resize(bufferSize, 0.0f);

        delayParam = addParameter<FloatParameter>("delayMs", delayMsVal, 0.0f, maxDelayMs);
        delayParam->informNodeOfChange = [this]() {
            delayMsVal.store(delayParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["maxDelayMs"] = maxDelayMs;
        nodeCreationData["delayMs"] = delayMsVal.load();
        return nodeCreationData;
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* in = inputPortBuffers[0]->getAudioBuffer();
        const float* delayIn = inputPortBuffers[1]->getAudioBuffer();
        float* out = outputPortBuffers[0]->getAudioBuffer();

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            // Write input sample into the delay buffer
            buffer[writePos] = in[i];

            // Read requested delay
            float delayMs = std::clamp(delayIn[i], 0.0f, maxDelayMs);
            float delaySamples = (delayMs * sampleRate) / 1000.0f;

            // Calculate fractional read position
            float readPos = static_cast<float>(writePos) - delaySamples;
            if (readPos < 0.0f)
                readPos += static_cast<float>(bufferSize);

            // Get integer positions
            size_t i0 = static_cast<size_t>(readPos) % bufferSize;
            size_t i1 = (i0 + 1) % bufferSize;
            float frac = readPos - static_cast<float>(i0);

            // Safe lerp between two adjacent samples
            float delayed = (1.0f - frac) * buffer[i0] + frac * buffer[i1];

            // Output the delayed sample
            out[i] = delayed;

            // Advance write position
            writePos = (writePos + 1) % bufferSize;
        }
    }
};

REGISTER(Delay);
