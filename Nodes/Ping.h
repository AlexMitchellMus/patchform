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
    DEFINE_AND_REGISTER_NODE("Ping", "png");

    std::function<void()> repaintFromDSP = [](){};

public:
#ifdef PATCHFORM_WITH_GUI

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<bool> eventQueue;
    moodycamel::ConcurrentQueue<bool> eventQueueFromDSP;

    bool isDefaultUI() const override { return false; };

    class UI final : public AudioNode::UI
    {
        std::atomic<bool> isDirty = std::atomic<bool>(false);
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            auto pingNode = static_cast<Ping*>(node);

            setSize(pingNode->width, pingNode->height);

            pingNode->repaintFromDSP = [this]()
            {
                isDirty.store(true);
            };
        };

        void updateGraphValues() override
        {
            if (isDirty.load())
            {
                auto ping = reinterpret_cast<Ping*>(audioNode);

                bool receivedEvent = false;
                if (ping->eventQueueFromDSP.try_dequeue(receivedEvent))
                {
                    triggerLight();
                }
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    reinterpret_cast<Ping*>(audioNode)->eventQueue.enqueue(true);
                    triggerLight();
                }
                else
                    AudioNode::UI::mouseButtonDown(e);
            }
        }

        void triggerLight()
        {
            triggered = true;
            repaint();
            counter = 0;
            startFrameTimer([this]() mutable {
                if (counter > 15)
                {
                    stopFrameTimer();
                    triggered = false;
                    repaint();
                    counter = 0;
                }
                counter++;
            });
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const auto centre = getWidth() * 0.5f;
            const auto radius = getWidth() * 0.3f;
            // Draw dial background
            nvgBeginPath(nvg);
            nvgCircle(nvg, centre, centre, radius);
            auto blue = nvgRGB(28, 73, 119);
            nvgFillColor(nvg, triggered ? blue : nvgRGBA(50, 50, 50, 255));  // Dark gray background
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
    float width = 40;
    float height = 40;

    Ping(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        width = objParams.value("width", 40);
        height = objParams.value("height", 40);

        addInputPort("Events", AudioPort::PortType::Data);
    }
#ifdef PATCHFORM_WITH_GUI
    void processAudio(float* out, const unsigned long frameCount) override
    {
        auto inputEvents = inputPortBuffers[0]->getEvents();


        if (!inputEvents.empty())
        {
            for (const auto* ev : inputEvents)
            {
                Event* e = context->eventPool.getFreeEvent();

                if (e)
                {
                    e->setTimeStamp(ev->getTimeStamp());
                    outputPort.addEvent(e);
                }
            }
            eventQueueFromDSP.enqueue(true);
            repaintFromDSP();
        }

        bool newValue;
        while (eventQueue.try_dequeue(newValue))
        {
            Event* e = context->eventPool.getFreeEvent();

            if (e)
            {
                outputPort.addEvent(e);
            }
        };
    }
#endif
};
