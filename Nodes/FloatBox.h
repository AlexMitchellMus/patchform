/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <atomic>

#include "AudioNodeBase.h"

class FloatBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Floatbox", "fb");

    float value;

    std::function<void()> repaintFromDSP = [](){};

public:
#ifdef PATCHFORM_WITH_GUI

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<float> queueFromDSP;

    bool isDefaultUI() const override { return false; };

    class UI final : public AudioNode::UI
    {
        std::atomic<bool> isDirty = std::atomic<bool>(false);
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(150, getHeight());

            reinterpret_cast<FloatBox*>(audioNode)->repaintFromDSP = [this]()
            {
                isDirty.store(true);
            };
        };

        void updateGraphValues() override
        {
            if (isDirty.load())
            {
                auto floatBox = reinterpret_cast<FloatBox*>(audioNode);

                std::vector<float> buffer(16); // Adjust size as needed
                size_t count = floatBox->queueFromDSP.try_dequeue_bulk(buffer.begin(), buffer.size());

                if (count > 0)
                {
                    float floatValue = buffer[count - 1]; // Use the last value (UI can't update faster than monitor refresh rate)
                    value = std::format("{:.4f}", floatValue);
                    repaint();
                }
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

            nvgText(nvg, 10, height / 2, value.c_str(), nullptr);
        }
    private:
        std::string value;
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
#endif
    FloatBox(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Value_input", AudioPort::PortType::Data);
    }
#ifdef PATCHFORM_WITH_GUI
    void processAudio(float* out, const unsigned long frameCount) override
    {
        const auto aEvents = inputPortBuffers[0]->getEvents();

        if (aEvents.size())
        {
            for (auto event : aEvents)
            {
                queueFromDSP.enqueue(event->data);
            }
            repaintFromDSP();
        }
    }
#endif
};
