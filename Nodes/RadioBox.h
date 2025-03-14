/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class RadioBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("RadioBox", "radiobox");

    int selectedIndex = 0;
    int radioCount = 8;

    IntParameter* numOptionsParam = nullptr;
    IntParameter* selectedIndexParam = nullptr;

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override
    {
        return false;
    }

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<int> eventQueue;

    class UI final : public AudioNode::UI
    {
        int selectedIndex = 0;
        int radioCount = 0;

        int boxWidth = 0;
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            auto radio = reinterpret_cast<RadioBox*>(audioNode);
            radioCount = radio->radioCount;

            boxWidth = getHeight();

            setSize(calculateWidth(), getHeight());

            // Update the Nodes UI directly from the parameter.
            // This is safe as the parameter is updated from the GUI thread
            // And the value is enqueued to the audio thread.
            radio->numOptionsParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable
            {
                auto newRadioCount = std::get_if<int>(&value);
                if (newRadioCount)
                {
                    auto radioCountValue = *newRadioCount;
                    if (radioCountValue != radioCount)
                    {
                        radioCount = radioCountValue;
                        setSize(calculateWidth(), getHeight());
                        repaint();
                    }
                }
            };
        };

        int calculateWidth()
        {
            return radioCount * boxWidth;
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    auto radio = reinterpret_cast<RadioBox*>(audioNode);
                    int clickedIndex = e.sdlEvent.button.x / boxWidth;

                    if (clickedIndex >= 0 && clickedIndex < radio->radioCount)
                    {
                        selectedIndex = clickedIndex;
                        radio->eventQueue.enqueue(clickedIndex);
                        repaint();
                    }
                }
                AudioNode::UI::mouseButtonDown(e);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            for (int i = 0; i < radioCount; ++i)
            {
                // Draw box
                nvgBeginPath(nvg);
                auto box = pptk::Rect(i * boxWidth, 0, boxWidth - 2, getHeight()).expanded(-4);
                auto col = i == selectedIndex ? nvgRGB(68, 68, 68) : nvgRGB(43, 43, 43);
                nvgDrawRoundedRect(nvg, box.x, box.y, box.w, box.h, col, col, 4);
            }
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
#endif

    RadioBox(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        radioCount = objParams.value("numOptions", 4);
        selectedIndex = objParams.value("selectedIndex", 0);

        numOptionsParam = addParameter<IntParameter>("Options:", radioCount, 1, 128);
        selectedIndexParam = addParameter<IntParameter>("Selected:", selectedIndex, 0, radioCount - 1);
    }

    json getSerializedNode() override
    {
        nodeCreationData["numOptions"] = radioCount;
        nodeCreationData["selectedIndex"] = selectedIndex;
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(float* out, const unsigned long frameCount) override
    {
        radioCount = numOptionsParam->getValue();
        selectedIndex = selectedIndexParam->getValue();

        int newIndex;
        while (eventQueue.try_dequeue(newIndex))
        {
            if (newIndex >= 0 && newIndex < radioCount)
            {
                selectedIndex = newIndex;

                if (Event* e = context->eventPool.getFreeEvent())
                {
                    e->addAtom(selectedIndex);
                    outputPort.addEvent(e);
                }
            }
        }
    }
#endif
};
