class BinCombine final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("BinCombine", "bincombine", true);

public:
    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    BinCombine(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Spectral, objParams)
    {
        addInputPort("a_real", AudioPort::Spectral);
        addInputPort("a_imag", AudioPort::Spectral);
        addInputPort("b_real", AudioPort::Spectral);
        addInputPort("b_imag", AudioPort::Spectral);

        addOutputPort("imag", AudioPort::Spectral);

        modeParam = addParameter<StringParameter>("mode", objParams.value("mode", "formant"));
        modeHash.store(hash(modeParam->getValue()));

        modeParam->informNodeOfChange = [this]() {
            modeHash.store(hash(modeParam->getValue()));
        };
    }

    void processAudio(const float*, float*, const unsigned long, std::vector<MidiMessage>&) override {
        const float* aR = inputPortBuffers[0]->getAudioBuffer();
        const float* aI = inputPortBuffers[1]->getAudioBuffer();
        const float* bR = inputPortBuffers[2]->getAudioBuffer();
        const float* bI = inputPortBuffers[3]->getAudioBuffer();

        float* outR = outputPortBuffers[0]->getAudioBuffer();
        float* outI = outputPortBuffers[1]->getAudioBuffer();

        std::array<float, FREQ_BINS> bMagnitudes{};
        for (size_t i = 0; i < FREQ_BINS; ++i)
            bMagnitudes[i] = std::sqrt(bR[i] * bR[i] + bI[i] * bI[i]);

        std::array<size_t, 5> peakBins{};
        std::array<float, 5> peakMags{};

        for (size_t i = 1; i < FREQ_BINS - 1; ++i) {
            if (bMagnitudes[i] > bMagnitudes[i - 1] && bMagnitudes[i] > bMagnitudes[i + 1]) {
                for (size_t j = 0; j < 5; ++j) {
                    if (bMagnitudes[i] > peakMags[j]) {
                        for (size_t k = 4; k > j; --k) {
                            peakBins[k] = peakBins[k - 1];
                            peakMags[k] = peakMags[k - 1];
                        }
                        peakBins[j] = i;
                        peakMags[j] = bMagnitudes[i];
                        break;
                    }
                }
            }
        }

        auto m = modeHash.load();

        for (size_t i = 0; i < FREQ_BINS; ++i) {
            float realA = aR[i];
            float imgA = aI[i];

            float realB = bR[i];
            float imgB = bI[i];

            float re = 0.0f, im = 0.0f;

            switch (m) {
            case hash("vocoder"):
                {
                    float lenA;
                    float angA;
                    float lenB;
                    float angB;
                    carToPol(realA, imgA, &lenA, &angA);
                    carToPol(realB, imgB, &lenB, &angB);

                    float mult = lenA * lenB;

                    polToCar(mult, angB, &re, &im);
                }
                break;
            default:
                re = realA;
                im = imgA;
                break;
            }

            outR[i] = re;
            outI[i] = im;
        }
    }

private:
    inline void carToPol(float real, float imag, float* length, float* angle)
    {
        *length = std::sqrt(real * real + imag * imag);
        *angle = std::atan2(imag, real);
    }

    inline void polToCar(float length, float angle, float* real, float* imag)
    {
        *real = length * std::cos(angle);
        *imag = length * std::sin(angle);
    }

    StringParameter* modeParam = nullptr;
    std::atomic<uint32_t> modeHash;
};
