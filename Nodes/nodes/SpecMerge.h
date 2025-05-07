#pragma once

#include "../SpectralHelpers.h"

class SpecMerge final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("SpecMerge", "specMerge", true);
    DEFINE_NODE_ALIASES("specmerge");

public:
    static constexpr size_t FFT_SIZE = 512;
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2;

    SpecMerge(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Spectral, objParams)
    {
        addInputPort("a_real", AudioPort::Spectral);
        addInputPort("a_imag", AudioPort::Spectral);
        addInputPort("b_real", AudioPort::Spectral);
        addInputPort("b_imag", AudioPort::Spectral);

        addOutputPort("imag", AudioPort::Spectral);

        modeParam = addParameter<ListParameter>("mode", std::vector<std::string>{ "vocoder", "multiply", "phasereplace", "magreplace", "add", "sub", "maxmag", "crossfade", "magdiff", "warp" }, objParams.value("mode", "vocoder"));
        modeHash.store(hash(modeParam->getValue()));

        modeParam->informNodeOfChange = [this]() {
            modeHash.store(hash(modeParam->getValue()));
        };

        context->stringMap.intern("vocoder", "multiply", "phasereplace", "magreplace", "add", "sub", "maxmag", "crossfade", "magdiff", "warp");
    }

    json getSerializedNode() override
    {
        if (auto modeString = context->stringMap.find(modeHash.load()))
            nodeCreationData["mode"] = *modeString;
        return nodeCreationData;
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
            case hash("vocoder"): {
                    float lenA, lenB, angB;
                    SpectralHelpers::carToPol(realA, imgA, &lenA, nullptr);
                    SpectralHelpers::carToPol(realB, imgB, &lenB, &angB);
                    float mult = lenA * lenB;
                    SpectralHelpers::polToCar(mult, angB, &re, &im);
                    break;
            }
            case hash("multiply"): {
                    re = realA * realB - imgA * imgB;
                    im = realA * imgB + imgA * realB;
                    break;
            }
            case hash("phasereplace"): {
                    float lenA, angB;
                    SpectralHelpers::carToPol(realA, imgA, &lenA, nullptr);
                    SpectralHelpers::carToPol(realB, imgB, nullptr, &angB);
                    SpectralHelpers::polToCar(lenA, angB, &re, &im);
                    break;
            }
            case hash("magreplace"): {
                    float lenB, angA;
                    SpectralHelpers::carToPol(realB, imgB, &lenB, nullptr);
                    SpectralHelpers::carToPol(realA, imgA, nullptr, &angA);
                    SpectralHelpers::polToCar(lenB, angA, &re, &im);
                    break;
            }
            case hash("add"): {
                    re = realA + realB;
                    im = imgA + imgB;
                    break;
            }
            case hash("sub"): {
                    re = realA - realB;
                    im = imgA - imgB;
                    break;
            }
            case hash("maxmag"): {
                    float magA = realA * realA + imgA * imgA;
                    float magB = realB * realB + imgB * imgB;
                    if (magA > magB) {
                        re = realA;
                        im = imgA;
                    } else {
                        re = realB;
                        im = imgB;
                    }
                    break;
            }
            case hash("crossfade"): {
                    float x = 0.5f; // TODO: make this a parameter
                    re = (1.0f - x) * realA + x * realB;
                    im = (1.0f - x) * imgA + x * imgB;
                    break;
            }
            case hash("magdiff"): {
                    float lenA, angA, lenB;
                    SpectralHelpers::carToPol(realA, imgA, &lenA, &angA);
                    SpectralHelpers::carToPol(realB, imgB, &lenB, nullptr);
                    float diff = std::max(0.0f, lenA - lenB);
                    SpectralHelpers::polToCar(diff, angA, &re, &im);
                    break;
            }
            case hash("warp"): {
                    float lenA, angA, angB;
                    SpectralHelpers::carToPol(realA, imgA, &lenA, &angA);
                    SpectralHelpers::carToPol(realB, imgB, nullptr, &angB);

                    float phaseDelta = angB - angA;

                    // nonlinear warp factor (could be a parameter later)
                    float warp = std::tanh(phaseDelta * 2.0f);  // squashed warp
                    float warpedAngle = angA + warp;

                    SpectralHelpers::polToCar(lenA, warpedAngle, &re, &im);
                    break;
            }
            default: {
                    re = realA;
                    im = imgA;
                    break;
            }
            }


            outR[i] = re;
            outI[i] = im;
        }
    }

private:
    ListParameter* modeParam = nullptr;
    std::atomic<uint32_t> modeHash;
};

REGISTER(SpecMerge);
