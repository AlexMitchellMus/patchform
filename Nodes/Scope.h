/*
    // Copyright (c) 2025 Alex Mitchell
    // For information on usage and redistribution, and for a DISCLAIMER OF ALL
    // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <array>
#include <iostream>         // For debugging output
#include "nanovg.h"         // NanoVG drawing API header.
#include "concurrentqueue.h"// Moodycamel's lock-free queue header.
#include <utility>          // For std::move
#include <algorithm>        // For std::copy

class Scope final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Scope", "scope");

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override { return false; }

    // Use a fixed DSP buffer size.
    static constexpr size_t DSP_BUFFER_SIZE = 1024;
    // FULL_BUFFER_SIZE is 2x DSP_BUFFER_SIZE to allow a contiguous block after rotation.
    static constexpr size_t DOUBLE_BUFFER_SIZE = 2 * DSP_BUFFER_SIZE;
    // Define BufferType as a fixed-size std::array of DSP_BUFFER_SIZE samples.
    using BufferType = std::array<float, DSP_BUFFER_SIZE>;

    // Enqueue fixed–size buffers.
    moodycamel::ConcurrentQueue<BufferType> eventQueue = moodycamel::ConcurrentQueue<BufferType>(6);

    class UI final : public AudioNode::UI
    {
    public:
        // Define the fixed DSP buffer size.
        static constexpr size_t DSP_BUFFER_SIZE = 1024;
        using BufferType = std::array<float, DSP_BUFFER_SIZE>;

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            // Set a fixed size for the oscilloscope widget.
            setSize(400, 100);
        }

        // updateGraphValues() drains the queue and updates the waveform state.
        void updateGraphValues() override
        {
            auto scope = reinterpret_cast<Scope*>(audioNode);
            BufferType newBuffer;
            bool newData = false;

            // Drain any new fixed-size buffers from the queue.
            while (scope->eventQueue.try_dequeue(newBuffer))
            {
                waveform = newBuffer;
                waveformValid = true;
                newData = true;
            }

            // Only trigger a repaint if we got new data.
            if (newData)
                repaint();
        }

        // drawGUI() now simply draws the current waveform.
        void drawGUI(NVGcontext* nvg) override
        {
            const int w = getWidth();
            const int h = getHeight();

            // Draw a dark, rounded background.
            auto bg = nvgRGBA(11, 11, 11, 255);
            nvgDrawRoundedRect(nvg, 1, 1, w - 2, h - 2, bg, bg, 5);

            if (!waveformValid)
                return;

            // Draw exactly DSP_BUFFER_SIZE samples along the horizontal axis.
            constexpr size_t DISPLAY_SIZE = DSP_BUFFER_SIZE;
            float xStep = static_cast<float>(w) / (DISPLAY_SIZE - 1);

            nvgBeginPath(nvg);
            float x = 0.0f;
            // Map the first sample (assumed to be in [-1, 1]) vertically.
            float prevY = h * 0.5f - waveform[0] * (h * 0.5f);
            nvgMoveTo(nvg, x, prevY);

            for (size_t i = 1; i < DISPLAY_SIZE; ++i)
            {
                x = i * xStep;
                float currentY = h * 0.5f - waveform[i] * (h * 0.5f);
                nvgLineTo(nvg, x, currentY);
            }

            nvgLineStyle(nvg, NVG_SOLID);
            nvgStrokeColor(nvg, nvgRGBA(200, 200, 200, 255));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
        }

    private:
        // Buffer holding the most recent DSP_BUFFER_SIZE samples.
        BufferType waveform;
        bool waveformValid = false;
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    Scope(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        // Add an input port named "audioIn" expecting signal data.
        addInputPort("audioIn", AudioPort::Signal);
    }

#ifdef PATCHFORM_WITH_GUI
    // DSP processing callback using a double-sized buffer and a constant fraction discriminator.
    void processAudio(float* /*out*/, const unsigned long frameCount) override
    {
        // TODO: Use an atomic flag if the UI want's a buffer update?
        // Currently we just check if the queue is empty
        if (eventQueue.size_approx() > 0)
            return;

        const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        if (!inputBuffer)
            return;

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float sample = inputBuffer[i];
            dspBuffer[dspBufferIndex++] = sample;

            // When the double-sized buffer (FULL_BUFFER_SIZE samples) is full...
            if (dspBufferIndex >= DOUBLE_BUFFER_SIZE)
            {
                int detectedTrigger = 0;
                bool found = false;
                // We want to search in the first DSP_BUFFER_SIZE samples.
                // We require a delay of at least cfdDelay samples.
                // Compute the CFD output:
                //   CFD(j) = dspBuffer[j] - cfdFraction * dspBuffer[j - cfdDelay]
                // Then find the first index where CFD goes from negative to >= 0.
                float prevCFD = 0.0f;
                // Initialize the CFD value at j = cfdDelay.
                if (cfdDelay < DSP_BUFFER_SIZE)
                    prevCFD = dspBuffer[cfdDelay] - cfdFraction * dspBuffer[0];
                for (size_t j = cfdDelay + 1; j < DSP_BUFFER_SIZE; ++j)
                {
                    float currCFD = dspBuffer[j] - cfdFraction * dspBuffer[j - cfdDelay];
                    // Look for a zero crossing: previous CFD < 0 and current CFD >= 0.
                    if (prevCFD < 0.0f && currCFD >= 0.0f)
                    {
                        detectedTrigger = static_cast<int>(j);
                        found = true;
                        break;
                    }
                    prevCFD = currCFD;
                }
                // If no zero crossing was found, default to 0.
                if (!found)
                    detectedTrigger = 0;

                // Clamp the trigger index to the valid range.
                if (detectedTrigger < 0 || detectedTrigger >= static_cast<int>(DSP_BUFFER_SIZE))
                    detectedTrigger = 0;

                // Copy DSP_BUFFER_SIZE contiguous samples starting at detectedTrigger.
                BufferType buffer;
                for (size_t j = 0; j < DSP_BUFFER_SIZE; ++j)
                {
                    buffer[j] = dspBuffer[detectedTrigger + j];
                }
                // Enqueue the synchronized block for the UI.
                eventQueue.enqueue(buffer);

                // Save the last sample from the full buffer (for consistency, if needed).
                lastSample = dspBuffer[DOUBLE_BUFFER_SIZE - 1];

                // Shift the second half of dspBuffer into the beginning so we keep the last DSP_BUFFER_SIZE samples.
                std::copy(dspBuffer.begin() + DSP_BUFFER_SIZE, dspBuffer.end(), dspBuffer.begin());
                dspBufferIndex = DSP_BUFFER_SIZE;
            }
        }
        // Optionally, pass through input to output:
        // std::copy(inputBuffer, inputBuffer + frameCount, out);
    }
#endif

private:
#ifdef PATCHFORM_WITH_GUI
    // Use a double–sized DSP buffer.
    std::array<float, DOUBLE_BUFFER_SIZE> dspBuffer{};
    // Current index in dspBuffer.
    size_t dspBufferIndex = 0;

    float lastSample = 0.0f;      // Last sample from the previous full buffer; used to detect crossings between blocks.
    float triggerLevel = 0.0f;
    int trigmode = 1;
    float trigx = 0.0f;
    bool retrigger = true;
    int stableTriggerIndex = 0;

    // --- Constant Fraction Discriminator (CFD) parameters ---
    // The constant fraction (for example, 0.5 means the CFD is computed as: current sample minus 0.5 times the delayed sample).
    float cfdFraction = 0.5f;
    // The delay in samples (set this according to the expected rising edge width of your signal).
    size_t cfdDelay = 10;

#endif
};
