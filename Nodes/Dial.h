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

    float dailValue;

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
        };

        float valueToAngle(float value)
        {
            return (minAngle - NVG_PI * 0.5f) + (maxAngle - minAngle) * value;
        }

        void mouseDrag(const Point& position, const Point& delta, Button button) override
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

        void render(NVGcontext* nvg) override
        {
            auto radius = getWidth() * 0.5f;
            // Draw dial background
            nvgBeginPath(nvg);
            nvgCircle(nvg, radius, radius, radius);
            nvgFillColor(nvg, nvgRGBA(50, 50, 50, 255));  // Dark gray background
            nvgFill(nvg);

            // Calculate dot position based on angle
            float dotRadius = radius * 0.2f;  // Small dot
            auto angle = valueToAngle(value);
            float dotX = radius + (radius - dotRadius - 10) * cos(angle);
            float dotY = radius + (radius - dotRadius - 10) * sin(angle);

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
    }

    void processAudio(float* out, const unsigned long frameCount) override
    {
        float newValue;
        while (eventQueue.try_dequeue(newValue))
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                e->data = newValue * 400 + 200;
                e->setTimeStamp(0); // Set event at time 0
                outputPort.addEvent(e);
            }
        };
    }

};
