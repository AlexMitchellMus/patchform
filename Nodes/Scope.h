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

    FloatParameter* negRangeParam;
    FloatParameter* posRangeParam;

    float negRange;
    float posRange;

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

        float negRange;
        float posRange;

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
                if (!freeze)
                {
                    waveform = newBuffer;
                    waveformValid = true;
                    newData = true;
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

            if (!waveformValid)
                return;

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
        BufferType waveform;
        bool waveformValid = false;
        bool freeze = false;
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
void processAudio(float* /*out*/, const unsigned long frameCount) override {
    const float* inputBuffer = inputPortBuffers[0]->getAudioBuffer();
    if (!inputBuffer)
        return;

    // Define the sweep size (number of samples per sweep).
    constexpr size_t SWEEP_SIZE = DSP_BUFFER_SIZE;

    // Use the beginning of dspBuffer as the accumulation buffer.
    // Use static variables to keep state between calls.
    static size_t accumulationIndex = 0;
    static bool triggeredState = false;
    static bool waitingForFallingEdge = true;

    // Trigger threshold and hysteresis (adjust as needed or expose as parameters)
    float triggerThreshold = 0.0f;
    float hysteresis = 0.05f; // margin to avoid chatter

    // Process each incoming sample.
    for (size_t i = 0; i < frameCount; i++) {
        float sample = inputBuffer[i];

        // If we haven't started a sweep (i.e. not triggered yet),
        // then check for a trigger condition.
        if (!triggeredState) {
            // First, wait for a falling edge so we’re ready for a rising edge.
            if (waitingForFallingEdge) {
                if (sample < triggerThreshold - hysteresis) {
                    waitingForFallingEdge = false; // now ready for a rising edge
                }
            }
            // When not waiting, look for a rising edge.
            if (!waitingForFallingEdge && sample > triggerThreshold + hysteresis) {
                // Rising edge detected: start accumulating the sweep.
                triggeredState = true;
                accumulationIndex = 0;
            }
        }

        // If triggered, record the sample.
        if (triggeredState) {
            if (accumulationIndex < SWEEP_SIZE) {
                dspBuffer[accumulationIndex] = sample;
                accumulationIndex++;
            }
        }
    }

    // When we've filled a complete sweep, publish it and reset the state.
    if (accumulationIndex >= SWEEP_SIZE) {
        BufferType buffer;
        std::copy(dspBuffer.begin(), dspBuffer.begin() + SWEEP_SIZE, buffer.begin());
        eventQueue.enqueue(buffer);

        // Reset state for the next sweep.
        accumulationIndex = 0;
        triggeredState = false;
        waitingForFallingEdge = true;
    }
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
