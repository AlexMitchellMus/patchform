#include "pffft.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <tuple>

class TableXSpectral : public AudioNode {
    DEFINE_AND_REGISTER_NODE("TableXSpectral", "tableXspectral", true);

public:
    TableXSpectral(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Wavetable, objParams)
    {
        addInputPort("a", AudioPort::Wavetable);
        addInputPort("b", AudioPort::Wavetable);
        addInputPort("x", AudioPort::Signal); // blend 0–1

        setup = pffft_new_setup(2048, PFFFT_REAL);
        fftA = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        fftB = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        fftOut = (float*)pffft_aligned_malloc(2048 * sizeof(float));
        tempTime = (float*)pffft_aligned_malloc(2048 * sizeof(float));
    }

    ~TableXSpectral() override {
        pffft_aligned_free(fftA);
        pffft_aligned_free(fftB);
        pffft_aligned_free(fftOut);
        pffft_aligned_free(tempTime);
        pffft_destroy_setup(setup);
    }

    struct Peak {
        float bin;
        float mag;
        float phase;
    };

    static void extractPeaks(const float* fft, Peak* peaks, int& count, int maxPeaks) {
        count = 0;
        for (int i = 1; i < 1023; ++i) {
            float re = fft[2 * i];
            float im = fft[2 * i + 1];
            float mag = std::sqrt(re * re + im * im);
            if (mag > 1e-6f && count < maxPeaks) {
                float phase = std::atan2(im, re);
                peaks[count++] = { (float)i, mag, phase };
            }
        }
        std::sort(peaks, peaks + count, [](const Peak& a, const Peak& b) {
            return a.mag > b.mag;
        });
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override {
        const float* a = inputPortBuffers[0]->getAudioBuffer();
        const float* b = inputPortBuffers[1]->getAudioBuffer();
        const float* x = inputPortBuffers[2]->getAudioBuffer();
        float* out = outputPortBuffers[0]->getAudioBuffer();

        float blend = std::clamp(x[0], 0.0f, 1.0f);

        pffft_transform_ordered(setup, a, fftA, nullptr, PFFFT_FORWARD);
        pffft_transform_ordered(setup, b, fftB, nullptr, PFFFT_FORWARD);

        Peak peaksA[128];
        Peak peaksB[128];
        int countA = 0, countB = 0;
        extractPeaks(fftA, peaksA, countA, 128);
        extractPeaks(fftB, peaksB, countB, 128);
        size_t count = std::min(countA, countB);

        std::fill(fftOut, fftOut + 2048, 0.0f);

        for (size_t i = 0; i < count; ++i) {
            float bin = (1.0f - blend) * peaksA[i].bin + blend * peaksB[i].bin;
            float mag = (1.0f - blend) * peaksA[i].mag + blend * peaksB[i].mag;
            float phase = (1.0f - blend) * peaksA[i].phase + blend * peaksB[i].phase;

            int binLo = (int)std::floor(bin);
            float frac = bin - binLo;
            float re = mag * std::cos(phase);
            float im = mag * std::sin(phase);

            if (binLo >= 1 && binLo < 2047) {
                fftOut[2 * binLo    ] += (1.0f - frac) * re;
                fftOut[2 * binLo + 1] += (1.0f - frac) * im;
                fftOut[2 * (binLo + 1)    ] += frac * re;
                fftOut[2 * (binLo + 1) + 1] += frac * im;
            }
        }

        // Interpolate DC and Nyquist
        fftOut[0] = (1.0f - blend) * fftA[0] + blend * fftB[0];
        fftOut[1] = (1.0f - blend) * fftA[1] + blend * fftB[1];

        pffft_transform_ordered(setup, fftOut, tempTime, nullptr, PFFFT_BACKWARD);

        for (int i = 0; i < 2048; ++i)
            out[i] = tempTime[i] * (1.0f / 2048.0f);
    }

private:
    PFFFT_Setup* setup = nullptr;
    float* fftA = nullptr;
    float* fftB = nullptr;
    float* fftOut = nullptr;
    float* tempTime = nullptr;
};
