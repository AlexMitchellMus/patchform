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

    // Use a fixed buffer size for DSP.
    static constexpr size_t DSP_BUFFER_SIZE = 1024;
    // Define BufferType as a fixed-size std::array.
    using BufferType = std::array<float, DSP_BUFFER_SIZE>;

    // Enqueue fixed–size buffers.
    moodycamel::ConcurrentQueue<BufferType> eventQueue;

class UI final : public AudioNode::UI
{
public:
    // Define the fixed DSP buffer size.
    static constexpr size_t DSP_BUFFER_SIZE = 1024;
    // Use std::array for fixed-size buffers.
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
        // TODO: make bg/fg colour a parameter
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

        // Define a threshold (in pixels) for detecting discontinuities.
        const float discontinuityThreshold = 20.0f; // Adjust as needed

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
    // DSP processing callback that uses a fixed-size array.
    void processAudio(float* /*out*/, const unsigned long frameCount) override
    {
        const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        if (!inputBuffer)
            return;

        // Process each incoming sample.
        for (unsigned long i = 0; i < frameCount; ++i)
        {
            dspBuffer[dspBufferIndex++] = inputBuffer[i];

            // When dspBuffer is full, sync on a rising edge.
            if (dspBufferIndex == DSP_BUFFER_SIZE)
            {
                int triggerIndex = -1;
                // Look for a rising edge: a transition from negative to zero or positive.
                for (size_t j = 1; j < DSP_BUFFER_SIZE; ++j)
                {
                    if (dspBuffer[j - 1] < 0.0f && dspBuffer[j] >= 0.0f)
                    {
                        triggerIndex = static_cast<int>(j);
                        break;
                    }
                }
                // If no rising edge is found, default to 0.
                if (triggerIndex < 0)
                    triggerIndex = 0;

                // Create a fixed–size buffer (std::array) with the synchronized data.
                BufferType buffer;
                for (size_t j = 0; j < DSP_BUFFER_SIZE; ++j)
                {
                    buffer[j] = dspBuffer[(triggerIndex + j) % DSP_BUFFER_SIZE];
                }
                // Enqueue the fixed–size buffer.
                eventQueue.enqueue(std::move(buffer));

                // Reset the dspBuffer index for new incoming samples.
                dspBufferIndex = 0;
            }
        }

        // std::copy(inputBuffer, inputBuffer + frameCount, out);
    }
#endif

private:
#ifdef PATCHFORM_WITH_GUI
    // Fixed–size DSP buffer to accumulate samples.
    std::array<float, DSP_BUFFER_SIZE> dspBuffer{};
    // Current index in dspBuffer.
    size_t dspBufferIndex = 0;
#endif
};
