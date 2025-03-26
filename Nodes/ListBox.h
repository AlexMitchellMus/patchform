// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include <atomic>
#include <array>
#include <vector>
#include <sstream>
#include <algorithm>
#include "AudioNodeBase.h"
#include "concurrentqueue.h"
#ifdef PATCHFORM_WITH_GUI
#include "../UI_ToolKit//TextEditor.h"
#endif

class ListBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("ListBox", "lb");

    float value;

    bool isDefaultUI() const override { return false; };

#ifdef PATCHFORM_WITH_GUI

public:
    static const size_t MAX_DISPLAY_ATOMS_PER_EVENT = 64;
    using EventDataBuffer = std::string;

    std::function<void()> updateUI = [](){};

    // Lock-free queue for DSP -> UI communication
    moodycamel::ConcurrentQueue<EventDataBuffer> queueFromDSP;
    // Lock-free queue for UI -> DSP communication
    moodycamel::ConcurrentQueue<EventDataBuffer> queueToDSP;

    DataAtom* savedData = nullptr;
#endif

    // Editable list text stored as a string.
    std::string listText;

#ifdef PATCHFORM_WITH_GUI

public:
    // The UI class with an editable text field.
    class UI final : public AudioNode::UI
    {
        bool isInit = false;
        std::unique_ptr<pptk::TextEditor> textEditor;

        std::string uiText;

        std::atomic<bool> shouldRepaint = false;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            auto listBox = reinterpret_cast<ListBox*>(audioNode);

            listBox->updateUI = [this]()
            {
                shouldRepaint.store(true);;
            };

            textEditor = std::make_unique<pptk::TextEditor>(false);
            addComponent(textEditor.get());

            // Initialize with the current listText.
            textEditor->setText(listBox->listText);
            uiText = listBox->listText;

            textEditor->setInteractable(false);

            // When the user edits text, update listText and reparse.
            textEditor->onTextChanged = [this, listBox, ed = textEditor.get()]()
            {
                uiText = ed->getText();
                updateWidth();
            };

            textEditor->onTextReturned = [this, listBox, ed = textEditor.get()]()
            {
                ed->setInteractable(false);
                listBox->queueToDSP.enqueue(textEditor->getText());
            };
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (!cnv->isInLockedMode())
                {
                    // Enable editing on double-click.
                    if (e.sdlEvent.button.clicks == 2)
                        textEditor->setInteractable(true);
                    else
                        textEditor->setInteractable(false);

                    AudioNode::UI::mouseButtonDown(e);
                }
            }
        }

        void resized() override
        {
            textEditor->setBounds(0, 0, getWidth(), getHeight());
        }

        // In updateGraphValues, we check for DSP events that update the list.
        void updateGraphValues() override
        {
            if (!shouldRepaint.load() && isInit)
                return;

            if (!isInit)
            {
                isInit = true;
                updateWidth();
            }

            shouldRepaint.store(false);

            auto listBox = reinterpret_cast<ListBox*>(audioNode);
            // Only update if the user is not actively editing.
            if (!textEditor->getIsInteractable())
            {
                std::string buffer;
                bool updated = false;
                // Dequeue DSP events; the last event replaces all values.
                while (listBox->queueFromDSP.try_dequeue(buffer))
                {
                    uiText = buffer; // Replace entire listText.
                    updated = true;
                }
                if (updated)
                {
                    // Update the text editor to reflect DSP changes.
                    textEditor->setText(uiText);
                    updateWidth();
                    repaint();
                }
            }
        }

        void updateWidth()
        {
            auto textWidth = getTextWidthForFont("Regular", 14, uiText) + 20;
            setSize(std::max(30, static_cast<int>(textWidth)), getHeight());
        }

        void render(NVGcontext* nvg) override
        {
            nvgBeginPath(nvg);
            auto bgCol = nvgRGB(33, 33, 33);
            auto outLineCol = nvgRGB(45, 45, 45);
            if (getIsHovered()) bgCol = outLineCol;
            if (getIsSelected()) outLineCol = nvgRGB(28, 73, 119);
            nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

public:
    ListBox(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Value_input", AudioPort::PortType::Data);
        // Initialize listText from JSON parameters (or a default if not provided).
        listText = objParams.value("list", "");
    }

    json getSerializedNode() override
    {
        nodeCreationData["list"] = listText;
        return nodeCreationData;
    }

    // Helper function to parse a single atom (either a float or a list).
    DataAtom* parseAtom(const char*& s, EventPool& pool)
    {
        // Skip leading whitespace.
        while (std::isspace(*s)) s++;

        // If we encounter an opening brace, parse a list.
        if (*s == '{')
        {
            s++; // skip '{'
            auto listAtom = pool.allocateDataAtom();
            listAtom->type = DataAtom::DataType::List; // Mark as list type.

            // Parse the child chain recursively.
            DataAtom* firstChild = nullptr;
            DataAtom* lastChild = nullptr;
            while (*s && *s != '}')
            {
                DataAtom* child = parseAtom(s, pool);
                if (child)
                {
                    if (!firstChild)
                    {
                        firstChild = child;
                        lastChild = child;
                    }
                    else
                    {
                        lastChild->next = child;
                        lastChild = child;
                    }
                }
                // Skip any whitespace or commas between atoms.
                while (std::isspace(*s)) s++;
                if (*s == ',') s++;
            }
            if (*s == '}') s++; // Skip the closing brace.
            // Assume the list is stored in a union field called 'list'.
            listAtom->data.list = firstChild;
            return listAtom;
        }
        else
        {
            // Otherwise, parse a float.
            char* endPtr = nullptr;
            float value = std::strtof(s, &endPtr);
            if (s == endPtr)
            {
                // No valid float found.
                return nullptr;
            }
            s = endPtr;
            auto dataAtom = pool.allocateDataAtom();
            dataAtom->data.atom = value; // Sets data.atom = value.
            return dataAtom;
        }
    }

    // Helper function to parse a chain of atoms separated by commas.
    DataAtom* parseChain(const char*& s, EventPool& pool)
    {
        DataAtom* first = nullptr;
        DataAtom* last = nullptr;
        while (*s)
        {
            while (std::isspace(*s)) s++;
            if (!*s)
                break;

            DataAtom* atom = parseAtom(s, pool);
            if (atom)
            {
                if (!first)
                {
                    first = atom;
                    last = atom;
                }
                else
                {
                    last->next = atom;
                    last = atom;
                }
            }
            while (std::isspace(*s)) s++;
            if (*s == ',')
                s++; // Skip comma delimiter.
            else
                break;
        }
        return first;
    }


    // processAudio receives DSP events that replace the list values.
    void processAudio(float* out, const unsigned long frameCount) override
    {
        if (!savedData && !listText.empty())
        {
            const char* p = listText.c_str();
            savedData = parseChain(p, context->eventPool);
            context->makeDataPersistent(savedData, true, nodeID);
        }

        const auto& aEvents = inputPortBuffers[0]->getEvents();

        for (const auto& event : aEvents)
        {
            switch (event->getTagHash())
            {
            case hash("output"):
                {
                    // If "output" tagged event is sent to listbox, output the saved data to a new event (even if the data is empty)
                    if (auto* newEvent = context->eventPool.getFreeEvent())
                    {
                        newEvent->data = savedData;
                        newEvent->setTimeStamp(event->getTimeStamp());
                        outputPortBuffers[0]->addEvent(newEvent);
                    }
                }
                break;
            default:
                {
                    EventDataBuffer buffer;
                    buffer = event->getAtom(0)->toString();
                    if (savedData)
                        context->makeDataPersistent(savedData, false, nodeID);

                    savedData = event->data;
                    context->makeDataPersistent(savedData, true, nodeID);
                    // Enqueue DSP event – it will replace the entire listText.
                    queueFromDSP.enqueue(buffer);
                    updateUI();
                    outputPortBuffers[0]->addEvent(event);
                }
            }
        }

        EventDataBuffer uiBuffer;
        while (queueToDSP.try_dequeue(uiBuffer))
        {
            // Update internal DSP state with the new text.
            listText = uiBuffer;

            // Get a free event from the event pool.
            auto newEvent = context->eventPool.getFreeEvent();

            // Parse the entire string into a linked chain of DataAtoms.
            const char* p = listText.c_str();
            DataAtom* mainAtomChain = parseChain(p, context->eventPool);

            if (savedData)
                context->makeDataPersistent(savedData, false, nodeID);

            savedData = mainAtomChain;
            context->makeDataPersistent(savedData, true, nodeID);

            // Attach the constructed atom chain to the event.
            newEvent->data = savedData;

            // Send the event downstream.
            outputPortBuffers[0]->addEvent(newEvent);
        }
    }
};
