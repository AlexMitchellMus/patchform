class TableOsc : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableOsc", "tblosc", true);

    float phase = 0.0f;
    float freq = 440.0f;

public:
    TableOsc(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("waveform", AudioPort::PortType::Spectral);  // expects 256-point table
        addInputPort("frequency", AudioPort::PortType::Data);
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto waveform = inputPortBuffers[0]->getAudioBuffer();  // 256 samples hardcoded for now
        const auto& fEvents = inputPortBuffers[1]->getEvents();
        const auto output = outputPortBuffers[0]->getAudioBuffer();

        for (auto e : fEvents)
        {
            if (e->data && e->data->type == DataAtom::DataType::Float)
                freq = e->data->data.atom;
        }

        // Check if the wavetable buffer is a power of two
        // TODO: we need to make audio buffer's dynamic size (they are already vector<float> so shouldn't be hard)
        const bool isPowerOfTwo = false;

        switch (isPowerOfTwo)
        {
            // Faster processing (as we can use bit-shift instead of modulo for po2)
        case true:
            {
                for (unsigned long i = 0; i < frameCount; ++i)
                {
                    const float invSampleRate = 1.0f / context->sampleRate;
                    const float tableSize = 256.0f;

                    phase += freq * invSampleRate;
                    if (phase >= 1.0f) phase -= 1.0f;

                    float idx = phase * tableSize;
                    int i0 = static_cast<int>(idx);
                    float frac = idx - i0;
                    int i1 = (i0 + 1) & 255;

                    float s0 = waveform[i0];
                    float s1 = waveform[i1];
                    output[i] = s0 + frac * (s1 - s0);
                }
            }
            break;
        default:
        case false:
            {
                for (unsigned long i = 0; i < frameCount; ++i)
                {
                    float phaseInc = freq / context->sampleRate;
                    phase += phaseInc;
                    if (phase >= 1.0f)
                        phase -= 1.0f;

                    float idx = phase * 255.0f;
                    int i0 = static_cast<int>(idx);
                    int i1 = (i0 + 1) % 256;
                    float frac = idx - i0;
                    output[i] = waveform[i0] + frac * (waveform[i1] - waveform[i0]);
                }
            }
            break;
        }
    }
};
