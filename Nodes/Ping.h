/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class Ping final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Dial", "dial");

public:
#ifdef PATCHFORM_WITH_GUI

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<bool> eventQueue;

    class UI final : public AudioNode::UI
    {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(40, 40);
        };

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    reinterpret_cast<Ping*>(audioNode)->eventQueue.enqueue(true);
                    triggered = true;
                    repaint();
                    startFrameTimer([this]() mutable {
                        if (counter > 30)
                        {
                            stopFrameTimer();
                            triggered = false;
                            repaint();
                            counter = 0;
                        }
                        counter++;
                    });
                }
                else
                    AudioNode::UI::mouseButtonDown(e);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const auto centre = getWidth() * 0.5f;
            const auto radius = getWidth() * 0.3f;
            // Draw dial background
            nvgBeginPath(nvg);
            nvgCircle(nvg, centre, centre, radius);
            nvgFillColor(nvg, triggered ? nvgRGBA(255, 0, 0, 255) : nvgRGBA(50, 50, 50, 255));  // Dark gray background
            nvgFill(nvg);
        }
    private:
        bool triggered = false;
        unsigned int counter = 0;
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
#endif
    Ping(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
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
                e->setTimeStamp(0); // Set event at time 0
                outputPort.addEvent(e);
            }
        };
    }

};
