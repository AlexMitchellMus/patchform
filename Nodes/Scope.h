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
    DEFINE_AND_REGISTER_NODE("Scope", "scope", true);

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override { return false; }

    FloatParameter* negRangeParam;
    FloatParameter* posRangeParam;

    float negRange;
    float posRange;

    static constexpr size_t DATA_BUFFER_QUANT_RES = 4096;
    // Use a fixed DSP buffer size.
    static constexpr size_t DSP_BUFFER_SIZE = 1024;
    // FULL_BUFFER_SIZE is 2x DSP_BUFFER_SIZE to allow a contiguous block after rotation.
    static constexpr size_t DOUBLE_BUFFER_SIZE = 2 * DSP_BUFFER_SIZE;
    // Define BufferType as a fixed-size std::array of DSP_BUFFER_SIZE samples.
    using BufferType = std::array<float, DSP_BUFFER_SIZE>;
    using BufferTypeInt = std::array<int, DSP_BUFFER_SIZE>;

    // Enqueue fixed–size buffers.
    moodycamel::ConcurrentQueue<BufferTypeInt> eventQueue = moodycamel::ConcurrentQueue<BufferTypeInt>(6);

    class UI final : public AudioNode::UI
    {
    public:
        // Define the fixed DSP buffer size.
        static constexpr size_t DSP_BUFFER_SIZE = 1024;
        using BufferType = std::array<float, DSP_BUFFER_SIZE>;

        float negRange;
        float posRange;

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            // Set a fixed size for the oscilloscope widget.
            setSize(400, 100);
            waveform.fill(0.0f);
        }

        // updateGraphValues() drains the queue and updates the waveform state.
        void updateGraphValues() override
        {
            auto scope = reinterpret_cast<Scope*>(audioNode);
            BufferTypeInt newBuffer;
            bool gotNewBuffer = false;

            // Drain any new fixed-size buffers from the queue.
            while (scope->eventQueue.try_dequeue(newBuffer))
            {
                gotNewBuffer = true;
            };

            if (!freeze && gotNewBuffer)
            {
                // Compare the current waveform data to the new data.
                // We can do this because the waveform data is in int (as the resolution of display is much less than float precision)
                if (waveformData != newBuffer)
                {
                    waveformData = newBuffer;
                    waveformValid = true;
                    newData = true;
                    for (int i = 0; i < waveformData.size(); i++)
                    {
                        waveform[i] = waveformData[i] / static_cast<float>(DATA_BUFFER_QUANT_RES);
                    }
                }
            }

            // FIXME: We probably want param without queue here?
            // Params for this object are not used in the audio thread
            auto newNegRange = reinterpret_cast<Scope*>(audioNode)->negRangeParam->getValue();
            auto newPosRange = reinterpret_cast<Scope*>(audioNode)->posRangeParam->getValue();

            // Only trigger a repaint if we got new data.
            if (newData || newNegRange != negRange || newPosRange != posRange)
            {
                posRange = newPosRange;
                negRange = newNegRange;
                repaint();
                newData = false;
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    freeze = true;
                }
                // FIXME: Has to be a better way to give the object the mousebutton event, without having to do it manually!
                Object::mouseButtonDown(e);
            }
        }

        void mouseButtonUp(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    freeze = false;
                }
                Object::mouseButtonUp(e);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const int w = getWidth();
            const int h = getHeight();

            // Draw a dark, rounded background.
            auto bg = nvgRGBA(11, 11, 11, 255);
            nvgDrawRoundedRect(nvg, 1, 1, w - 2, h - 2, bg, bg, 5);

            //if (!waveformValid)
            //return;

            // Calculate the full range width.
            float rangeWidth = posRange - negRange;
            if (rangeWidth == 0.0f) {
                // Avoid division by zero.
                return;
            }

            // Draw exactly DSP_BUFFER_SIZE samples along the horizontal axis.
            constexpr size_t DISPLAY_SIZE = DSP_BUFFER_SIZE;
            float xStep = static_cast<float>(w) / (DISPLAY_SIZE - 1);

            nvgBeginPath(nvg);
            float x = 0.0f;

            // For the first sample, map it from [negRange, posRange] into [0, 1]
            float normalized = (waveform[0] - negRange) / rangeWidth;
            // Then map [0,1] to [h,0]: normalized value 0 gives y = h (bottom),
            // normalized value 1 gives y = 0 (top)
            float prevY = h * (1.0f - normalized);
            nvgMoveTo(nvg, x, prevY);

            for (size_t i = 1; i < DISPLAY_SIZE; ++i)
            {
                x = i * xStep;
                normalized = (waveform[i] - negRange) / rangeWidth;
                float currentY = h * (1.0f - normalized);
                nvgLineTo(nvg, x, currentY);
            }

            // Scissor the waveform only
            nvgSave(nvg);
            nvgScissor(nvg, 0, 1, width, height - 2);

            nvgLineStyle(nvg, NVG_SOLID);
            nvgStrokeColor(nvg, nvgRGBA(200, 200, 200, 255));
            nvgStrokeWidth(nvg, 1.0f);
            nvgStroke(nvg);

            nvgRestore(nvg);
        }



    private:
        // Buffer holding the most recent DSP_BUFFER_SIZE samples.
        BufferTypeInt waveformData;
        BufferType waveform;
        bool waveformValid = false;
        bool freeze = false;
        bool newData = true;
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

        negRange = objParams.value("neg range", -1.0f);
        posRange = objParams.value("pos range", 1.0f);

        negRangeParam     = addParameter<FloatParameter>("signal range -", negRange, -10000.0f, 10000);
        posRangeParam     = addParameter<FloatParameter>("signal range +", posRange, -10000.0f, 10000);
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float* in, float* /*out*/, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
        if (!inputBuffer)
            return;

        // Shift the dspBuffer left by frameCount and append new input at the end
        std::memmove(dspBuffer.data(), dspBuffer.data() + frameCount, (DOUBLE_BUFFER_SIZE - frameCount) * sizeof(float));
        std::memcpy(dspBuffer.data() + (DOUBLE_BUFFER_SIZE - frameCount), inputBuffer, frameCount * sizeof(float));

        if (eventQueue.size_approx() > 0)
            return;

        // Only search for the trigger in the first DSP_BUFFER_SIZE (1024) samples.
        int detectedTrigger = 0;
        bool found = false;
        float prevCFD = 0.0f;

        // Initialize using the sample at index 0 and the one at index cfdDelay,
        // provided cfdDelay is less than DSP_BUFFER_SIZE.
        if (cfdDelay < DSP_BUFFER_SIZE)
            prevCFD = dspBuffer[cfdDelay] - cfdFraction * dspBuffer[0];

        // Search for the zero-crossing (CFD trigger) in the first 1024 samples.
        for (size_t j = cfdDelay + 1; j < DSP_BUFFER_SIZE; ++j)
        {
            float currCFD = dspBuffer[j] - cfdFraction * dspBuffer[j - cfdDelay];
            if (prevCFD < 0.0f && currCFD >= 0.0f)
            {
                detectedTrigger = static_cast<int>(j);
                found = true;
                break;
            }
            prevCFD = currCFD;
        }

        // If no trigger is found, default to the start.
        if (!found)
            detectedTrigger = 0;

        // Now output a 1024-sample window starting at the trigger.
        BufferTypeInt buffer;
        for (size_t j = 0; j < DSP_BUFFER_SIZE; ++j)
        {
            // Assumes that (detectedTrigger + j) is within dspBuffer's bounds.
            buffer[j] = static_cast<int>(dspBuffer[detectedTrigger + j] * DATA_BUFFER_QUANT_RES);
        }

        // Enqueue for UI
        eventQueue.enqueue(buffer);
    }
#endif


private:
#ifdef PATCHFORM_WITH_GUI
    // Use a double–sized DSP buffer.
    std::array<float, DOUBLE_BUFFER_SIZE> dspBuffer{};
    // Current index in dspBuffer.
    size_t dspBufferIndex = 0;

    // --- Constant Fraction Discriminator (CFD) parameters ---
    // The constant fraction (for example, 0.5 means the CFD is computed as: current sample minus 0.5 times the delayed sample).
    float cfdFraction = 0.5f;
    // The delay in samples (set this according to the expected rising edge width of your signal).
    size_t cfdDelay = 10;

#endif
};
