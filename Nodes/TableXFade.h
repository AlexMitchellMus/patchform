class TableXFade : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXfade", "tableXfade", true);

public:
    TableXFade(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Wavetable, objParams)
    {
        addInputPort("a", AudioPort::Wavetable);
        addInputPort("b", AudioPort::Wavetable);
        addInputPort("x", AudioPort::Signal); // Blend 0–1
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* a = inputPortBuffers[0]->getAudioBuffer();
        const float* b = inputPortBuffers[1]->getAudioBuffer();
        const float* x = inputPortBuffers[2]->getAudioBuffer();
        float* out = outputPortBuffers[0]->getAudioBuffer();

        // Assume all buffers are same size (i.e., table length)
        for (size_t i = 0; i < defaultTableSize; ++i)
        {
            float mix = std::clamp(x[0], 0.0f, 1.0f); // Use first sample of control signal
            out[i] = (1.0f - mix) * a[i] + mix * b[i];
        }
    }
};
