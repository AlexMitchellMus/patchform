/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "AudioPort.h"
#include "Print.h"
#include <cmath>    // for std::ceil and std::sqrt
#include <algorithm> // for std::clamp

class RadioBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("RadioBox", "radiobox", false);
    DEFINE_NODE_ALIASES("radiobox");

public:
    enum class LayoutType { Horizontal, Vertical, Grid };

private:
    int selectedIndex = 0;
    std::atomic<int> radioCount = 8;
    std::atomic<bool> emitOnClick = false;
    LayoutType layout = LayoutType::Horizontal;

    IntParameter* radioCountParam = nullptr;
    StringParameter* layoutParam = nullptr;
    BoolParameter* emitOnClickParam = nullptr;

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override { return false; }

    std::function<void()> repaintFromDSP = [](){};

    // Lock-free queues for UI -> Audio and vice versa.
    moodycamel::ConcurrentQueue<int> eventQueue;
    moodycamel::ConcurrentQueue<int> eventQueueFromDSP;

    class UI final : public AudioNode::UI
    {
        int selectedIndex = 0;
        int radioCount = 0;
        LayoutType layout;

        int boxWidth = 0;
        int boxHeight = 0;
        // Outer margin remains at 6 px.
        const int boxMargin = 6;
        // The gap between cells is double the margin.
        const int gap = 2 * boxMargin;

    public:
        std::atomic<bool> isDirty = std::atomic<bool>(false);

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            auto radio = reinterpret_cast<RadioBox*>(audioNode);
            radioCount = radio->radioCount;
            layout = radio->layout;

            // Compute the dimensions of each radio button (square).
            // The available height is the box height plus two outer margins.
            boxWidth = getHeight() - 2 * boxMargin;
            boxHeight = boxWidth;

            setSize(calculateWidth(), calculateHeight());

            radio->repaintFromDSP = [this]() {
                isDirty.store(true, std::memory_order::release);
            };

            // Update UI when the number of cells changes.
            radio->radioCountParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable {
                if (auto newCount = std::get_if<int>(&value))
                {
                    if (*newCount != radioCount)
                    {
                        radioCount = *newCount;
                        int newHeight = calculateHeight();
                        bool heightChanged = (getHeight() != newHeight);
                        setSize(calculateWidth(), newHeight);
                        if (heightChanged)
                        {
                            if (const auto cnv = findParentOfClass<Canvas>())
                            {
                                cnv->updateConnectionsPosition();
                            }
                        }
                        repaint();
                    }
                }
            };

            // Update UI when the layout type changes.
            radio->layoutParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable {
                if (auto layoutStr = std::get_if<std::string>(&value))
                {
                    auto newLayout = RadioBox::getLayoutType(*layoutStr);
                    if (newLayout != layout)
                    {
                        layout = newLayout;
                        setSize(calculateWidth(), calculateHeight());
                        if (const auto cnv = findParentOfClass<Canvas>())
                        {
                            cnv->updateConnectionsPosition();
                        }
                    }
                }
            };
        }

        int getSelectedIndex() const
        {
            return selectedIndex;
        }

        LayoutType getLayoutType() const
        {
            return layout;
        }

        int calculateWidth()
        {
            if (layout == LayoutType::Horizontal)
                return radioCount * boxWidth + (radioCount - 1) * gap + 2 * boxMargin;
            else if (layout == LayoutType::Vertical)
                return boxWidth + 2 * boxMargin;
            else if (layout == LayoutType::Grid)
            {
                int cols = static_cast<int>(std::ceil(std::sqrt(radioCount)));
                return cols * boxWidth + (cols - 1) * gap + 2 * boxMargin;
            }
            return 0;
        }

        int calculateHeight()
        {
            if (layout == LayoutType::Horizontal)
                return boxHeight + 2 * boxMargin;
            else if (layout == LayoutType::Vertical)
                return radioCount * boxHeight + (radioCount - 1) * gap + 2 * boxMargin;
            else if (layout == LayoutType::Grid)
            {
                int cols = static_cast<int>(std::ceil(std::sqrt(radioCount)));
                int rows = static_cast<int>(std::ceil(static_cast<float>(radioCount) / cols));
                return rows * boxHeight + (rows - 1) * gap + 2 * boxMargin;
            }
            return 0;
        }

        void updateGraphValues() override
        {
            if (isDirty.load())
            {
                isDirty.store(false, std::memory_order::release);
                auto radio = reinterpret_cast<RadioBox*>(audioNode);

                int receivedEvent = 0;
                while (radio->eventQueueFromDSP.try_dequeue(receivedEvent))
                {
                    selectedIndex = std::min(radioCount - 1, receivedEvent);
                    repaint();
                }
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            // Only process left mouse button clicks.
            if (e.sdlEvent.button.button != SDL_BUTTON_LEFT)
            {
                AudioNode::UI::mouseButtonDown(e);
                return;
            }

            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    int clickedIndex = -1;
                    if (layout == LayoutType::Horizontal)
                    {
                        // Compute effective hit index by adding half the gap (6px)
                        int cellTotalWidth = boxWidth + gap;
                        int relativeX = e.sdlEvent.button.x - boxMargin;
                        clickedIndex = (relativeX + gap / 2) / cellTotalWidth;
                    }
                    else if (layout == LayoutType::Vertical)
                    {
                        int cellTotalHeight = boxHeight + gap;
                        int relativeY = e.sdlEvent.button.y - boxMargin;
                        clickedIndex = (relativeY + gap / 2) / cellTotalHeight;
                    }
                    else if (layout == LayoutType::Grid)
                    {
                        int cols = static_cast<int>(std::ceil(std::sqrt(radioCount)));
                        int cellTotalWidth = boxWidth + gap;
                        int cellTotalHeight = boxHeight + gap;
                        int relativeX = e.sdlEvent.button.x - boxMargin;
                        int relativeY = e.sdlEvent.button.y - boxMargin;
                        int col = (relativeX + gap / 2) / cellTotalWidth;
                        int row = (relativeY + gap / 2) / cellTotalHeight;
                        clickedIndex = row * cols + col;
                    }

                    // Clamp the clicked index to a valid range.
                    clickedIndex = std::clamp(clickedIndex, 0, radioCount - 1);
                    selectedIndex = clickedIndex;

                    if (auto radio = dynamic_cast<RadioBox*>(audioNode))
                    {
                        radio->eventQueue.enqueue(clickedIndex);
                        radio->setNodeDirty();
                    }

                    repaint();
                }

                AudioNode::UI::mouseButtonDown(e);
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            for (int i = 0; i < radioCount; ++i)
            {
                int x = 0, y = 0;
                if (layout == LayoutType::Horizontal)
                {
                    x = boxMargin + i * (boxWidth + gap);
                    y = boxMargin;
                }
                else if (layout == LayoutType::Vertical)
                {
                    x = boxMargin;
                    y = boxMargin + i * (boxHeight + gap);
                }
                else if (layout == LayoutType::Grid)
                {
                    int cols = static_cast<int>(std::ceil(std::sqrt(radioCount)));
                    x = boxMargin + (i % cols) * (boxWidth + gap);
                    y = boxMargin + (i / cols) * (boxHeight + gap);
                }

                nvgBeginPath(nvg);
                auto col = (i == selectedIndex) ? nvgRGB(68, 68, 68) : nvgRGB(43, 43, 43);
                nvgDrawRoundedRect(nvg, x, y, boxWidth, boxHeight, col, col, 4);
            }
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    // Helper function to parse a layout type string.
    static LayoutType getLayoutType(const std::string& layoutStr)
    {
        switch (hash(layoutStr))
        {
        case hash("Vertical"):
        case hash("vertical"):
            return LayoutType::Vertical;
        case hash("grid"):
            return LayoutType::Grid;
        default:
            return LayoutType::Horizontal;
        }
    }

    RadioBox(NodeContext* context, const json& objParams)
    : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        radioCount = objParams.value("numOptions", 8);
        selectedIndex = objParams.value("selectedIndex", 0);
        std::string layoutName = objParams.value("layoutType", "horizontal");
        layout = getLayoutType(layoutName);

        emitOnClick.store(JsonHelpers::getBoolOrIntFallback(objParams, "emitOnClick", true));

        radioCountParam = addParameter<IntParameter>("Cells:", radioCount, 1, 1024);
        layoutParam = addParameter<StringParameter>("Layout:", layoutName);
        emitOnClickParam = addParameter<BoolParameter>("emitOnClick:", emitOnClick);

        emitOnClickParam->informNodeOfChange = [this]()
        {
            emitOnClick.store(emitOnClickParam->getValue());
        };

        radioCountParam->informNodeOfChange = [this]()
        {
            radioCount.store(radioCountParam->getValue());
        };

        addInputPort("input", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["numOptions"] = radioCount.load();
        // Index can change from the audio thread, so we use the UI's selected index.
        nodeCreationData["selectedIndex"] = reinterpret_cast<RadioBox::UI*>(getOrCreateUI())->getSelectedIndex();

        const auto layoutType = reinterpret_cast<RadioBox::UI*>(getOrCreateUI())->getLayoutType();
        nodeCreationData["layoutType"] = (layoutType == LayoutType::Vertical) ? "vertical" :
                                         (layoutType == LayoutType::Grid) ? "grid" : "horizontal";
        nodeCreationData["emitOnClick"] = emitOnClickParam->getValue();
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float* in, float* out, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        auto inputEvents = inputPortBuffers[0]->getEvents();

        if (!inputEvents.empty())
        {
            for (auto* ev : inputEvents)
            {
                if (ev->data)
                {
                    selectedIndex = ev->getAtomValue(0);
                    // Forward the input event to the output.
                    addEvent(0, ev);
                }
                else
                {
                    auto outEvent = context->eventPool.getFreeEvent();
                    context->eventPool.addDataAtomTo(outEvent, selectedIndex);
                    addEvent(0, outEvent);
                }
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
                if (emitOnClick)
                {
                    if (Event* e = context->eventPool.getFreeEvent())
                    {
                        context->eventPool.addDataAtomTo(e, selectedIndex);
                        addEvent(0, e);
                    }
                }
            }
        }
    }
#endif
};

REGISTER(RadioBox);
