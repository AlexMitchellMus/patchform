/*
    // Copyright (c) 2025 Alex Mitchell
    // For information on usage and redistribution, and for a DISCLAIMER OF ALL
    // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>         // Debugging output
#include "nanovg.h"         // NanoVG drawing API
#include "concurrentqueue.h"// Moodycamel's queue
#include "pffft.h"          // PFFFT for fast FFT
#include <algorithm>
#include <cmath>            // log2f()

class Spec final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("SpectralPlot", "spec", true);

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override { return false; }

    static constexpr size_t FFT_SIZE = 1024;
    static constexpr size_t HOP_SIZE = FFT_SIZE / 4; // 75% overlap
    static constexpr size_t FREQ_BINS = FFT_SIZE / 2; // Half spectrum

    using BufferType = std::array<float, FREQ_BINS>;

    moodycamel::ConcurrentQueue<BufferType> eventQueue = moodycamel::ConcurrentQueue<BufferType>(6);

    class UI final : public AudioNode::UI
    {
    public:
        static constexpr size_t FREQ_BINS = Spec::FREQ_BINS;
        using BufferType = std::array<float, FREQ_BINS>;

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(400, 100);
        }

        void updateGraphValues() override
        {
            auto scope = reinterpret_cast<Spec*>(audioNode);
            BufferType newBuffer;
            bool newData = false;

            while (scope->eventQueue.try_dequeue(newBuffer))
            {
                spectrum = newBuffer;
                newData = true;
            }

            if (newData)
            {
                repaint();
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const float w = getWidth() - 2;
            const float h = getHeight() - 2;

            auto bg = nvgRGBA(11, 11, 11, 255);
            nvgDrawRoundedRect(nvg, 1, 1, w, h, bg, bg, 5);

            float xStep = w / (FREQ_BINS - 1);
            nvgBeginPath(nvg);
            float xPos = 0.0f;

            // **Scale Y properly for raw magnitude**
            float prevY = h * (1.0f - spectrum[0]); // Map [0, 1] → UI height

            nvgMoveTo(nvg, xPos, prevY);

            for (size_t i = 1; i < FREQ_BINS; ++i)
            {
                xPos = i * xStep;
                float currentY = h * (1.0f - spectrum[i]); // Map [0, 1] → UI height
                nvgLineTo(nvg, xPos, currentY);
            }

            nvgSave(nvg);
            nvgScissor(nvg, 1, 1, w, h);
            nvgLineStyle(nvg, NVG_SOLID);
            nvgStrokeColor(nvg, nvgRGBA(200, 200, 200, 255));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
            nvgRestore(nvg);
        }

    private:
        BufferType spectrum { 0.0f };
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    Spec(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        addInputPort("audioIn", AudioPort::Signal);
        fftSetup = pffft_new_setup(FFT_SIZE, PFFFT_REAL);
    }

    ~Spec() override
    {
        pffft_destroy_setup(fftSetup);
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(float* /*out*/, const unsigned long frameCount) override
    {
        if (eventQueue.size_approx() > 0) return;

        const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        if (!inputBuffer) return;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            dspBuffer[dspBufferIndex++] = inputBuffer[i];

            if (dspBufferIndex >= FFT_SIZE)
            {
                BufferType spectrum;
                computeFFT(spectrum);
                eventQueue.enqueue(spectrum);
                dspBufferIndex = 0; // Reset buffer
            }
        }
    }

    void computeFFT(BufferType& spectrum)
    {
        std::array<float, FFT_SIZE> timeDomainBuffer{};
        std::array<float, FFT_SIZE> freqDomainBuffer{};
        std::array<int, FREQ_BINS> binCounts{}; // Count bins for averaging

        // Apply Hann window
        for (size_t i = 0; i < FFT_SIZE; ++i)
        {
            timeDomainBuffer[i] = dspBuffer[i] * 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
        }

        // Perform FFT using PFFFT
        pffft_transform_ordered(fftSetup, timeDomainBuffer.data(), freqDomainBuffer.data(), nullptr, PFFFT_FORWARD);

        // Reset bins
        std::fill(spectrum.begin(), spectrum.end(), 0.0f);
        std::fill(binCounts.begin(), binCounts.end(), 0);

        float sampleRate = context->sampleRate;
        float nyquist = sampleRate / 2.0f;

        // **Log scale setup**
        float minFreq = 20.0f; // Start at 20 Hz (avoid log(0))
        float logMin = logf(minFreq);
        float logMax = logf(nyquist);
        float logRange = logMax - logMin;

        for (size_t i = 1; i < FFT_SIZE / 2; ++i) // Skip DC component
        {
            float real = freqDomainBuffer[i * 2];
            float imag = freqDomainBuffer[i * 2 + 1];
            float magnitude = sqrtf(real * real + imag * imag);

            // **Compute bin frequency**
            float freq = (i / (float)FFT_SIZE) * sampleRate;
            freq = std::max(freq, minFreq); // Prevent log(0)

            // **Logarithmic bin mapping**
            float logPos = (logf(freq) - logMin) / logRange; // Normalize to [0, 1]
            int binIndex = static_cast<int>(logPos * (FREQ_BINS - 1));

            // Ensure binIndex stays within valid range
            binIndex = std::clamp(binIndex, 0, (int)FREQ_BINS - 1);

            // **Smooth bin distribution**
            float frac = logPos * (FREQ_BINS - 1) - binIndex;
            spectrum[binIndex] += magnitude * (1.0f - frac);
            if (binIndex + 1 < FREQ_BINS) spectrum[binIndex + 1] += magnitude * frac;

            binCounts[binIndex]++;
        }

        // **Normalize bins (keep raw magnitude)**
        float maxMagnitude = *std::max_element(spectrum.begin(), spectrum.end());
        if (maxMagnitude > 0.0f)
        {
            for (size_t i = 0; i < FREQ_BINS; ++i)
            {
                if (binCounts[i] > 0) spectrum[i] /= binCounts[i]; // Average values
                spectrum[i] /= maxMagnitude; // Normalize to [0, 1] for UI
            }
        }
    }

#endif

private:
#ifdef PATCHFORM_WITH_GUI
    PFFFT_Setup* fftSetup;
    std::array<float, FFT_SIZE> dspBuffer { 0.0f };
    size_t dspBufferIndex = 0;
#endif
};
