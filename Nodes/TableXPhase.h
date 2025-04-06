class TableXPhase : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXPhase", "tableXphase", true);

public:
    TableXPhase(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Wavetable, objParams)
    {
        addInputPort("a", AudioPort::Wavetable);   // source wavetable
        addInputPort("b", AudioPort::Wavetable);   // phase table
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const float* source = inputPortBuffers[0]->getAudioBuffer(); // Table A
        const float* phase = inputPortBuffers[1]->getAudioBuffer();  // Table B (phase)
        float* out = outputPortBuffers[0]->getAudioBuffer();

        for (size_t i = 0; i < defaultTableSize; ++i)
        {
            float p = std::clamp(phase[i], 0.0f, 1.0f);  // phase offset
            float pos = p * defaultTableSize - 1;
            int index = static_cast<int>(pos);
            float frac = pos - index;

            float a0 = source[index % defaultTableSize];
            float a1 = source[(index + 1) % defaultTableSize];
            out[i] = a0 + frac * (a1 - a0); // linear interpolation
        }
    }
};
