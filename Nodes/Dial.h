/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class Dial final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Dial", "dial");

    float dialValue;

    FloatParameter* defaultValueParam = nullptr;
    FloatParameter* minValueParam = nullptr;
    FloatParameter* maxValueParam = nullptr;

    float minValue = 0.0f;
    float maxValue = 1.0f;

public:
#ifdef PATCHFORM_WITH_GUI

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<float> eventQueue;

    class UI final : public AudioNode::UI
    {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(100, 100);
            setGuiIsTransparent(true);

            auto dial = reinterpret_cast<Dial*>(audioNode);

            // Compute the normalized value.
            float normalizedValue = 0.0f;
            if (dial->maxValue != dial->minValue)
            {
                value = (dial->dialValue - dial->minValue) / (dial->maxValue - dial->minValue);
            }
            else
            {
                // If minValue and maxValue are the same, we default the normalized value (avoid division by zero).
                value = 0.0f;
            }
        };

        float valueToAngle(float value)
        {
            return (minAngle - NVG_PI * 0.5f) + (maxAngle - minAngle) * value;
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    value -= delta.y * 0.005f * cnv->scale;
                    value = fmax(0.0f, fmin(value, 1.0f));

                    reinterpret_cast<Dial*>(audioNode)->eventQueue.enqueue(value);

                    repaint();
                }
                else AudioNode::UI::mouseDrag(position, delta, button);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const auto centre = getWidth() * 0.5f;
            const auto radius = getWidth() * 0.4f;
            // Draw dial background
            nvgBeginPath(nvg);
            nvgCircle(nvg, centre, centre, radius);
            nvgFillColor(nvg, nvgRGBA(50, 50, 50, 255));  // Dark gray background
            nvgFill(nvg);

            // Calculate dot position based on angle
            float dotRadius = radius * 0.2f;  // Small dot
            auto angle = valueToAngle(value);
            float dotX = centre + (radius - dotRadius - 10) * cos(angle);
            float dotY = centre + (radius - dotRadius - 10) * sin(angle);

            // Draw dot
            nvgBeginPath(nvg);
            nvgCircle(nvg, dotX, dotY, dotRadius);
            nvgFillColor(nvg, nvgRGBA(28, 28, 28, 255));
            nvgFill(nvg);
        }
    private:
        float value = 0;

        const float minAngle = -NVG_PI * 0.75f; // -135 degrees
        const float maxAngle =  NVG_PI * 0.75f; //  135 degrees
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
#endif
    Dial(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        minValue = objParams.value("min", 0.0f);
        maxValue = objParams.value("max", 1.0f);
        dialValue = objParams.value("value", 0.0f);

        defaultValueParam = addParameter<FloatParameter>("Default:", dialValue, 0.0f, std::numeric_limits<float>::max());
        minValueParam     = addParameter<FloatParameter>("Min:", minValue, 0.0f, std::numeric_limits<float>::max());
        maxValueParam     = addParameter<FloatParameter>("Max:", maxValue, 0.0f, std::numeric_limits<float>::max());
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(float* out, const unsigned long frameCount) override
    {

        minValue = minValueParam->getValue();
        maxValue = maxValueParam->getValue();

        while (eventQueue.try_dequeue(dialValue))
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->data = dialValue * (maxValue - minValue) + minValue;
                e->setTimeStamp(0); // Set event at time 0
                outputPort.addEvent(e);
            }
        };
    }
#endif

};
