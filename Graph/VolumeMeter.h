#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <simde/arm/neon.h>
#include <simde/x86/sse.h>

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define PATCHFORM_USE_NEON
#elif defined(__SSE2__) || defined(__x86_64__) || defined(_M_X64)
    #define PATCHFORM_USE_SSE
#else
    #define PATCHFORM_USE_SCALAR
#endif

class VolumeMeter {
public:
    VolumeMeter(float sampleRate, unsigned long frameSize, int channels)
        : numChannels(channels),
          rms(channels, 0.0f),
          peak(channels, 0.0f),
          peakHold(channels, 0.0f),
          holdCounters(channels, 0),
          alpha(std::exp(-1.0f / (sampleRate * rmsTimeConstant))),
          holdFrames(static_cast<int>(sampleRate * peakHoldTimeSeconds / frameSize))
    {}

    void updateFrameSize(float sampleRate, unsigned long frameSize, int channels = -1) {
        alpha = std::exp(-1.0f / (sampleRate * rmsTimeConstant));
        holdFrames = static_cast<int>(sampleRate / frameSize);

        if (channels > 0 && channels != numChannels) {
            numChannels = channels;
            rms.assign(channels, 0.0f);
            peak.assign(channels, 0.0f);
            peakHold.assign(channels, 0.0f);
            holdCounters.assign(channels, 0);
        }
    }

    void process(const float* interleaved, unsigned long frameCount) {
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* src = interleaved + ch * frameCount;
            float sumSq = 0.0f;
            float maxPeak = 0.0f;

#if defined(PATCHFORM_USE_NEON)
            float32x4_t sumSqVec = vdupq_n_f32(0.0f);
            float32x4_t peakVec = vdupq_n_f32(0.0f);
            size_t i = 0;
            for (; i + 4 <= frameCount; i += 4) {
                float32x4_t v = vld1q_f32(src + i);
                float32x4_t absV = vabsq_f32(v);
                sumSqVec = vmlaq_f32(sumSqVec, v, v);
                peakVec = vmaxq_f32(peakVec, absV);
            }
            float temp[4];
            vst1q_f32(temp, sumSqVec);
            sumSq = temp[0] + temp[1] + temp[2] + temp[3];
            vst1q_f32(temp, peakVec);
            maxPeak = std::max({temp[0], temp[1], temp[2], temp[3]});
#elif defined(PATCHFORM_USE_SSE)
            simde__m128 sumSqVec = simde_mm_setzero_ps();
            simde__m128 peakVec = simde_mm_setzero_ps();
            size_t i = 0;
            for (; i + 4 <= frameCount; i += 4) {
                simde__m128 v = simde_mm_loadu_ps(src + i);
                simde__m128 absV = simde_mm_andnot_ps(simde_mm_set1_ps(-0.0f), v);
                sumSqVec = simde_mm_add_ps(sumSqVec, simde_mm_mul_ps(v, v));
                peakVec = simde_mm_max_ps(peakVec, absV);
            }
            float temp[4];
            simde_mm_storeu_ps(temp, sumSqVec);
            sumSq = temp[0] + temp[1] + temp[2] + temp[3];
            simde_mm_storeu_ps(temp, peakVec);
            maxPeak = std::max({temp[0], temp[1], temp[2], temp[3]});
#else
            size_t i = 0;
#endif
            for (; i < frameCount; ++i) {
                float v = src[i];
                sumSq += v * v;
                maxPeak = std::max(maxPeak, std::abs(v));
            }

            // RMS
            rms[ch] = fastSqrt(alpha * rms[ch] * rms[ch] + (1.0f - alpha) * sumSq / frameCount);

            // Peak (fast)
            peak[ch] = maxPeak;

            // Peak Hold (slow decay)
            if (maxPeak >= peakHold[ch]) {
                peakHold[ch] = maxPeak;
                holdCounters[ch] = holdFrames;
            } else if (--holdCounters[ch] <= 0) {
                peakHold[ch] *= peakDecayRate;
            }
        }
    }

    void getAllValues(float& outRmsL, float& outPeakL, float& outPeakHoldL,
                      float& outRmsR, float& outPeakR, float& outPeakHoldR) const {
        outRmsL = (numChannels > 0) ? rms[0] : 0.0f;
        outPeakL = (numChannels > 0) ? peak[0] : 0.0f;
        outPeakHoldL = (numChannels > 0) ? peakHold[0] : 0.0f;

        outRmsR = (numChannels > 1) ? rms[1] : 0.0f;
        outPeakR = (numChannels > 1) ? peak[1] : 0.0f;
        outPeakHoldR = (numChannels > 1) ? peakHold[1] : 0.0f;
    }

    bool shouldEmit() {
        return (++frameCounter >= emitInterval);
    }

    void resetEmit() {
        frameCounter = 0;
    }

private:
    static inline float fastSqrt(float x) {
        union { float f; uint32_t i; } u = { x };
        u.i = 0x5f3759df - (u.i >> 1);
        float y = u.f;
        y = y * (1.5f - 0.5f * x * y * y);
        return x * y;
    }

    int numChannels;
    std::vector<float> rms;
    std::vector<float> peak;
    std::vector<float> peakHold;
    std::vector<int> holdCounters;

    float alpha;
    int holdFrames;
    int emitInterval = 4;
    int frameCounter = 0;

    static constexpr float rmsTimeConstant = 0.3f;
    static constexpr float peakDecayRate = 0.996f;
    static constexpr float peakHoldTimeSeconds = 1.5f;
};
