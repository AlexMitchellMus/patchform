// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include <atomic>
#include <array>
#include <vector>
#include "AudioNodeBase.h"
#include "concurrentqueue.h"

class ListBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("ListBox", "lb");

    float value;
    std::function<void()> repaintFromDSP = []() {};

public:
#ifdef PATCHFORM_WITH_GUI

    static const size_t MAX_DISPLAY_ATOMS_PER_EVENT = 64;

    using EventDataBuffer = std::string;

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<EventDataBuffer> queueFromDSP;

    bool isDefaultUI() const override { return false; };

    class UI final : public AudioNode::UI
    {
        std::atomic<bool> isDirty = false;
        std::string text;
        float totalWidth = 0.0f;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(150, getHeight());
            reinterpret_cast<ListBox*>(audioNode)->repaintFromDSP = [this]()
            {
                isDirty.store(true);
            };
        }

        void updateGraphValues() override
        {
            if (isDirty.exchange(false))
            {
                auto* listBox = reinterpret_cast<ListBox*>(audioNode);
                EventDataBuffer buffer;
                while (listBox->queueFromDSP.try_dequeue(buffer));

                text.clear();
                totalWidth = 10.0f; // Padding

                text = buffer;
                totalWidth = getTextWidthForFont("Regular", 16, text) + 20.0f;

                // Adjust the node size based on the total width of values
                setSize(std::max(30, static_cast<int>(totalWidth)), height);
                repaint();
            }
        }

        void render(NVGcontext* nvg) override
        {
            nvgBeginPath(nvg);
            auto bgCol = nvgRGB(33, 33, 33);
            auto outLineCol = nvgRGB(45, 45, 45);
            if (getIsHovered()) bgCol = outLineCol;
            if (getIsSelected()) outLineCol = nvgRGB(28, 73, 119);
            nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);

            nvgFontSize(nvg, 16.0f);
            nvgFontFace(nvg, "Regular");
            nvgFillColor(nvg, nvgRGB(190, 190, 190));
            nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

            float xOffset = 10.0f;

            nvgText(nvg, xOffset, height / 2, text.c_str(), nullptr);
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    ListBox(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Value_input", AudioPort::PortType::Data);
    }

    void processAudio(float* out, const unsigned long frameCount) override
    {
        const auto& aEvents = inputPortBuffers[0]->getEvents();

        if (!aEvents.empty())
        {
            for (const auto& event : aEvents)
            {
                EventDataBuffer buffer;
                buffer = event->getAtom(0)->toString();

                queueFromDSP.enqueue(buffer);
                outputPort.addEvent(event);
            }
            repaintFromDSP();
        }
    }
};
