#pragma once

#include <vector>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <complex>
#include "pffft.h"

class PitchMPM {
public:
    PitchMPM(size_t bufferSize)
        : PitchMPM(44100, bufferSize) {}

    PitchMPM(int sampleRate, size_t bufferSize)
        : sampleRate(sampleRate), bufferSize(bufferSize), fftSize(2 * bufferSize),
          real(fftSize), imag(fftSize), output(fftSize), tempBuffer(fftSize) {
        fftSetup = pffft_new_setup(fftSize, PFFFT_REAL);
    }

    ~PitchMPM() {
        if (fftSetup) pffft_destroy_setup(fftSetup);
    }

    float getPitch(const float* audioBuffer) {
        if (!audioBuffer) return -1.0f;

        auto nsdf = nsdfFrequencyDomain(audioBuffer);
        auto maxPositions = peak_picking(nsdf);
        std::vector<std::pair<float, float>> estimates;

        float highestAmplitude = -FLT_MAX;
        for (auto tau : maxPositions) {
            highestAmplitude = std::max(highestAmplitude, nsdf[tau]);
            if (nsdf[tau] > 0.5f) {
                auto x = parabolic_interpolation(nsdf, tau);
                estimates.push_back(x);
                highestAmplitude = std::max(highestAmplitude, x.second);
            }
        }

        if (estimates.empty()) return -1;

        float actualCutoff = 0.93f * highestAmplitude;
        float period = 0;
        for (auto& e : estimates) {
            if (e.second >= actualCutoff) {
                period = e.first;
                break;
            }
        }

        float pitch = sampleRate / period;
        return (pitch > 8.18f) ? pitch : -1.0f;
    }

    void setSampleRate(int newRate) {
        sampleRate = newRate;
    }

    void setBufferSize(int newSize) {
        bufferSize = newSize;
        fftSize = 2 * bufferSize;
        real.resize(fftSize);
        imag.resize(fftSize);
        output.resize(fftSize);
        tempBuffer.resize(fftSize);
        if (fftSetup) pffft_destroy_setup(fftSetup);
        fftSetup = pffft_new_setup(fftSize, PFFFT_REAL);
    }

private:
    int sampleRate;
    size_t bufferSize;
    size_t fftSize;

    PFFFT_Setup* fftSetup = nullptr;
    std::vector<float> real;
    std::vector<float> imag;
    std::vector<float> output;
    std::vector<float> tempBuffer;

    static std::vector<int> peak_picking(const std::vector<float>& nsdf) {
        std::vector<int> peaks;
        int size = (int)nsdf.size();
        int pos = 0;
        while (pos < size / 3 && nsdf[pos] > 0) ++pos;
        while (pos < size - 1 && nsdf[pos] <= 0.0f) ++pos;
        if (pos == 0) pos = 1;
        int curMax = 0;
        while (pos < size - 1) {
            if (nsdf[pos] > nsdf[pos - 1] && nsdf[pos] >= nsdf[pos + 1]) {
                if (curMax == 0 || nsdf[pos] > nsdf[curMax]) {
                    curMax = pos;
                }
            }
            ++pos;
            if (pos < size - 1 && nsdf[pos] <= 0) {
                if (curMax > 0) {
                    peaks.push_back(curMax);
                    curMax = 0;
                }
                while (pos < size - 1 && nsdf[pos] <= 0.0f) ++pos;
            }
        }
        if (curMax > 0) peaks.push_back(curMax);
        return peaks;
    }

    static std::pair<float, float> parabolic_interpolation(const std::vector<float>& arr, int x) {
        if (x < 1 || x >= (int)arr.size() - 1) return { (float)x, arr[x] };
        float s0 = arr[x - 1];
        float s1 = arr[x];
        float s2 = arr[x + 1];
        float bottom = s2 + s0 - 2 * s1;
        if (bottom == 0.0f) return { (float)x, s1 };
        float delta = s0 - s2;
        float pos = x + delta / (2.0f * bottom);
        float amp = s1 - (delta * delta) / (8.0f * bottom);
        return { pos, amp };
    }

    std::vector<float> nsdfFrequencyDomain(const float* buffer) {
        std::copy(buffer, buffer + bufferSize, tempBuffer.begin());
        std::fill(tempBuffer.begin() + bufferSize, tempBuffer.end(), 0.0f);

        pffft_transform_ordered(fftSetup, tempBuffer.data(), real.data(), nullptr, PFFFT_FORWARD);

        for (size_t i = 0; i < fftSize; ++i) {
            float r = real[i];
            real[i] = r * r; // Power spectrum
        }

        pffft_transform_ordered(fftSetup, real.data(), output.data(), nullptr, PFFFT_BACKWARD);

        return output;
    }
};
