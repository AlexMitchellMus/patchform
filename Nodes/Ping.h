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
    DEFINE_AND_REGISTER_NODE("Ping", "png", false);

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
                isDirty.store(true, std::memory_order::release);
            };
        };

        void updateGraphValues() override
        {
            if (isDirty.load())
            {
                isDirty.store(false, std::memory_order::release);
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
                    auto pingNode = reinterpret_cast<Ping*>(audioNode);
                    pingNode->eventQueue.enqueue(true);
                    pingNode->setNodeDirty();
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
            auto triggerStartTime = SDL_GetTicks();

            startFrameTimer([this, triggerStartTime](uint32_t time, uint32_t deltaTime) mutable {
                if ((int32_t)(time - triggerStartTime) >= 90)  // Cast to handle wraparound correctly
                {
                    stopFrameTimer();
                    triggered = false;
                    repaint();
                }
            });
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const auto centre = getWidth() * 0.5f;
            const auto radius = getWidth() * 0.3f;

            nvgBeginPath(nvg);
            nvgCircle(nvg, centre, centre, radius);
            auto blue = nvgRGB(28, 73, 119);
            nvgFillColor(nvg, triggered ? blue : nvgRGBA(50, 50, 50, 255));  // Dark gray background
            nvgFill(nvg);
        }
    private:
        bool triggered = false;
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
    void processAudio(float* out, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        auto inputEvents = inputPortBuffers[0]->getEvents();


        if (!inputEvents.empty())
        {
            for (auto* ev : inputEvents)
            {
                //Event* e = context->eventPool.getFreeEvent();
                //if (e)
                //{
                //    e->setTimeStamp(ev->getTimeStamp());
                //    outputPort.addEvent(e);
                //}

                // Forward the same event from input to output (this should work, but just for now lets see how it goes)
                outputPortBuffers[0]->addEvent(ev);
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
                // We don't need to set the timestamp, as events from UI will not be sample accurate anyway
                outputPortBuffers[0]->addEvent(e);
            }
        };
    }
#endif
};
