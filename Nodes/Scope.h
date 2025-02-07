/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include <vector>
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

    bool isDefaultUI() const override
    {
        return false;
    }

    // We enqueue entire audio buffers as vectors.
    // Each enqueued vector will contain exactly 1024 samples.
    moodycamel::ConcurrentQueue<std::vector<float>> eventQueue;

    class UI final : public AudioNode::UI
    {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            // Set a fixed widget size.
            // (The widget width in pixels is independent of the number of samples drawn.)
            setSize(400, 100);
        }

        // Called (e.g., via a timer) to update the UI.
        void updateGraphValues() override
        {
            repaint();
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const int w = getWidth();
            const int h = getHeight();

            // Draw a dark, rounded background.
            auto bg = nvgRGBA(30, 30, 30, 255);
            nvgDrawRoundedRect(nvg, 1, 1, w - 2, h - 2, bg, bg, 5);

            // Drain any new audio buffers from the node’s queue.
            auto scope = reinterpret_cast<Scope*>(audioNode);
            std::vector<float> newBuffer;
            while (scope->eventQueue.try_dequeue(newBuffer))
            {
                // Instead of appending, simply replace the current waveform
                // with the newly received 1024-sample buffer.
                waveform = std::move(newBuffer);
            }

            // If no waveform is available, nothing to draw.
            if (waveform.empty())
                return;

            // We want to display exactly 1024 samples horizontally.
            constexpr size_t DISPLAY_SIZE = 1024;
            // (Even if the received vector is not 1024 samples, use whatever is available.)
            size_t numSamples = (waveform.size() < DISPLAY_SIZE) ? waveform.size() : DISPLAY_SIZE;

            // Map the 1024 (or fewer) samples evenly along the widget’s width.
            float xStep = static_cast<float>(w) / (DISPLAY_SIZE - 1);
            float x = 0.0f;
            // Assume sample values are in [-1, 1]; map them so that 1 is at the top and -1 at the bottom.
            float y = h * 0.5f - waveform[0] * (h * 0.5f);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, x, y);

            // Draw a line through each sample.
            for (size_t i = 1; i < DISPLAY_SIZE && i < waveform.size(); ++i)
            {
                x = i * xStep;
                y = h * 0.5f - waveform[i] * (h * 0.5f);
                nvgLineTo(nvg, x, y);
            }

            nvgLineStyle(nvg, NVG_SOLID);
            nvgStrokeColor(nvg, nvgRGBA(200, 200, 200, 255));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);
        }
    private:
        // Local buffer that holds the most recent 1024 samples.
        std::vector<float> waveform;
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
    // The audio processing callback.
    void processAudio(float* out, const unsigned long frameCount) override
    {
        // Get the pointer to the first channel’s audio buffer.
        const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        if (!inputBuffer)
            return;

        // Accumulate incoming samples.
        accumulationBuffer.insert(accumulationBuffer.end(), inputBuffer, inputBuffer + frameCount);

        // When we have at least 1024 samples, package exactly 1024 samples into a vector
        // and enqueue it for the UI to display.
        constexpr size_t DISPLAY_SIZE = 1024;
        while (accumulationBuffer.size() >= DISPLAY_SIZE)
        {
            // Create a vector containing the first 1024 samples.
            std::vector<float> buffer(accumulationBuffer.begin(),
                                      accumulationBuffer.begin() + DISPLAY_SIZE);
            eventQueue.enqueue(std::move(buffer));

            // Remove the enqueued samples from the accumulation buffer.
            accumulationBuffer.erase(accumulationBuffer.begin(),
                                     accumulationBuffer.begin() + DISPLAY_SIZE);
        }

        // Optionally, if you want to pass audio through, you can copy the input to the output.
        // std::copy(inputBuffer, inputBuffer + frameCount, out);
    }
#endif

private:
#ifdef PATCHFORM_WITH_GUI
    // Buffer to accumulate incoming samples.
    std::vector<float> accumulationBuffer;
#endif
};
