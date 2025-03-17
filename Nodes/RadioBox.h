/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "AudioPort.h"
#include "Print.h"

class RadioBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("RadioBox", "radiobox");

    int selectedIndex = 0;
    int radioCount = 8;

    IntParameter* numOptionsParam = nullptr;

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override
    {
        return false;
    }

    std::function<void()> repaintFromDSP = [](){};

    // Lock-free queue for UI -> Audio communication
    moodycamel::ConcurrentQueue<int> eventQueue;
    moodycamel::ConcurrentQueue<int> eventQueueFromDSP;

    class UI final : public AudioNode::UI
    {
        int selectedIndex = 0;
        int radioCount = 0;

        int boxWidth = 0;
        const int boxMargin = 6;
    public:
        std::atomic<bool> isDirty = std::atomic<bool>(false);

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            auto radio = reinterpret_cast<RadioBox*>(audioNode);
            radioCount = radio->radioCount;

            boxWidth = getHeight() - (boxMargin * 2);

            setSize(calculateWidth(), getHeight());

            radio->repaintFromDSP = [this]()
            {
                isDirty.store(true, std::memory_order::release);
            };

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

        void updateGraphValues() override
        {
            if (isDirty.load())
            {
                isDirty.store(false, std::memory_order::release);
                auto radio = reinterpret_cast<RadioBox*>(audioNode);

                int receivedEvent = 0;
                if (radio->eventQueueFromDSP.try_dequeue(receivedEvent))
                {
                    selectedIndex = receivedEvent;
                    repaint();
                }
            }
        }

        int calculateWidth()
        {
            return radioCount * boxWidth + (radioCount + 1) * boxMargin;
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    const int boxTotalWidth = boxWidth + boxMargin;
                    int relativeX = e.sdlEvent.button.x - boxMargin / 2; // shift hit area 2px to left
                    int clickedIndex = relativeX / boxTotalWidth;

                    // Clamp the clickedIndex to valid range
                    clickedIndex = std::clamp(clickedIndex, 0, radioCount - 1);

                    selectedIndex = clickedIndex;

                    if (auto radio = dynamic_cast<RadioBox*>(audioNode))
                        radio->eventQueue.enqueue(clickedIndex);

                    repaint();
                }

                AudioNode::UI::mouseButtonDown(e);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            for (int i = 0; i < radioCount; ++i)
            {
                int x = i * boxWidth + (i + 1) * boxMargin;
                int y = boxMargin;

                nvgBeginPath(nvg);
                auto col = (i == selectedIndex) ? nvgRGB(68, 68, 68) : nvgRGB(43, 43, 43);
                nvgDrawRoundedRect(nvg, x, y, boxWidth, boxWidth, col, col, 4);
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

        numOptionsParam = addParameter<IntParameter>("Cells:", radioCount, 1, 128);

        addInputPort("input", AudioPort::PortType::Data);
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

        auto inputEvents = inputPortBuffers[0]->getEvents();

        if (!inputEvents.empty())
        {
            for (auto* ev : inputEvents)
            {
                if (ev->numAtoms)
                    selectedIndex = ev->getAtomValue(0);
                // Forward the same event from input to output (this should work, but just for now lets see how it goes)
                outputPort.addEvent(ev);
            }
            eventQueueFromDSP.enqueue(selectedIndex);
            repaintFromDSP();
        }

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
