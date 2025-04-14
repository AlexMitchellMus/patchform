// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.

#pragma once

#include <atomic>
#include <array>
#include <vector>
#include <sstream>
#include <algorithm>
#include <regex>

#include "AudioNodeBase.h"
#include "concurrentqueue.h"
#ifdef PATCHFORM_WITH_GUI
#include "../UI_ToolKit//TextEditor.h"
#endif

class ListBox final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("ListBox", "lb", false);
    DEFINE_NODE_ALIASES("lb", "listbox");

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

    std::string textBuffer;
    std::string newTextBuffer;
    // Editable list text stored as a string.
    std::string listText;

    std::atomic<bool> listChanged = false;
    std::atomic<bool> triggerSendFromAudio = false;

#endif

#ifdef PATCHFORM_WITH_GUI

public:
    // The UI class with an editable text field.
    class UI final : public AudioNode::UI
    {
        bool isInit = false;
        std::unique_ptr<pptk::TextEditor> textEditor;

        std::string uiText;

    public:
        std::atomic<bool> shouldRepaint = false;

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

            textEditor->onTextReturned = [this, listBox]()
            {
                textEditor->setInteractable(false);
                listBox->queueToDSP.enqueue(formatSymbols(textEditor->getText()));
                listBox->setNodeDirty();
            };
        }

        // Helper to check if a token is numeric.
        // This simple regex matches integers or floats.
        bool isNumeric(const std::string &s) {
            static const std::regex numRegex(R"(^[-+]?[0-9]*\.?[0-9]+$)");
            return std::regex_match(s, numRegex);
        }

        // Compute the hash of a given string token.
        unsigned int computeHash(const std::string &s) {
            auto listBox = reinterpret_cast<ListBox*>(audioNode);
            return listBox->context->stringMap.internString(s);
        }

        std::string formatSymbols(const std::string& listText) {
            // This regex matches tokens that exclude whitespace, commas, and curly braces.
            std::regex tokenRegex(R"([^\s,{}]+)");

            std::string result;
            size_t lastPos = 0;

            // Iterate over all matches using sregex_iterator.
            auto begin = std::sregex_iterator(listText.begin(), listText.end(), tokenRegex);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                std::smatch match = *it;
                // Append the text between the last match and this match.
                result.append(listText, lastPos, match.position() - lastPos);

                std::string token = match.str();
                if (isNumeric(token)) {
                    // If the token is numeric, leave it unchanged.
                    result += token;
                } else {
                    // Otherwise, compute the hash and format it.
                    unsigned int hashValue = computeHash(token);
                    result += "@$" + std::to_string(hashValue) + "$@";
                }

                // Update last position after the current match.
                lastPos = match.position() + match.length();
            }

            // Append any remaining text after the last match.
            result.append(listText, lastPos, listText.size() - lastPos);

            return result;
        }

        std::string unformatSymbols(const std::string& listText) {
            // This regex matches tokens in the form "@$<digits>$@"
            std::regex symbolRegex(R"(@\$([0-9]+)\$@)");
            std::string result;
            size_t lastPos = 0;

            auto begin = std::sregex_iterator(listText.begin(), listText.end(), symbolRegex);
            auto end = std::sregex_iterator();

            for (auto it = begin; it != end; ++it) {
                std::smatch match = *it;
                // Append text from the previous match to the current match.
                result.append(listText, lastPos, match.position() - lastPos);
                // Capture the numeric hash (first capture group)
                std::string hashStr = match[1].str();
                unsigned int hashValue = static_cast<unsigned int>(std::stoul(hashStr));
                // Lookup the original symbol using the stringMap.
                auto listBox = reinterpret_cast<ListBox*>(audioNode);
                if (auto symbolString = listBox->context->stringMap.find(hashValue))
                {
                    result += *symbolString;
                }
                lastPos = match.position() + match.length();
            }
            // Append any remaining text.
            result.append(listText, lastPos, listText.size() - lastPos);
            return result;
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            // Enable editing on double-click.
            if (e.sdlEvent.button.clicks == 2)
                textEditor->setInteractable(true);
            else
                textEditor->setInteractable(false);

            AudioNode::UI::mouseButtonDown(e);
        }

        void resized() override
        {
            textEditor->setBounds(0, 0, getWidth(), getHeight());
        }

        // In updateGraphValues, we check for DSP events that update the list.
        void updateGraphValues() override
        {
            auto listBox = reinterpret_cast<ListBox*>(audioNode);
            if (listBox->listChanged.load())
            {
                listBox->listChanged.store(false);
                listBox->triggerSendFromAudio.store(true);
                listBox->setNodeDirty();
            }

            if (!shouldRepaint.load() && isInit)
                return;

            if (!isInit)
            {
                isInit = true;
                updateWidth();
            }

            shouldRepaint.store(false);

            // Only update if the user is not actively editing.
            if (!textEditor->getIsInteractable())
            {
                std::string buffer;
                bool updated = false;
                // Dequeue DSP events; the last event replaces all values.
                while (listBox->queueFromDSP.try_dequeue(buffer))
                {
                    updated = true;
                }
                if (updated)
                {
                    uiText = unformatSymbols(buffer);
                    //std::cout << "buffer: " << buffer << " unformat: " << uiText << std::endl;
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
    ListBox(NodeContext* context, const json& objParams): AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("Value_input", AudioPort::PortType::Data);

        textBuffer.reserve(1024);
        newTextBuffer.reserve(1024);
        listText.reserve(1024);

        listText = objParams.value("list", "");
    }

    json getSerializedNode() override
    {
        nodeCreationData["list"] = listText;
        return nodeCreationData;
    }

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
            listAtom->data.list = firstChild;
            return listAtom;
        }
        // Check for symbol token: starts with "@$" and ends with "$@".
        else if (*s == '@' && *(s + 1) == '$')
        {
            s += 2; // Skip the initial "@$"
            // Find the closing "$@"
            const char* endSymbol = std::strstr(s, "$@");
            if (!endSymbol)
            {
                // No closing marker found. In a real implementation you might want to handle this error.
                return nullptr;
            }
            // Extract the inner string (expected to be a numeric hash value)
            std::string symbolToken(s, endSymbol);
            // Convert the extracted string to an unsigned int.
            unsigned int hashValue = static_cast<unsigned int>(std::stoul(symbolToken));
            s = endSymbol + 2; // Move past the "$@" marker

            auto dataAtom = pool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Symbol;
            dataAtom->data.symbol = hashValue;
            return dataAtom;
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
            dataAtom->type = DataAtom::DataType::Float;
            dataAtom->data.atom = value;
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
    void processAudio(const float* in, float* out, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        if (!savedData && !listText.empty())
        {
            const char* p = listText.c_str();
            savedData = parseChain(p, context->eventPool);
            context->makeDataPersistent(savedData, true, nodeID);
        }

        const auto& aEvents = inputPortBuffers[0]->getEvents();

        bool dataUpdated = false;

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
                        addEvent(0, newEvent);
                    }
                }
                break;
            default:
                {
                    // Only update if the data has actually changed.
                    if (savedData != event->data)
                    {
                        dataUpdated = true;
                        // Persist the old data as no longer active.
                        if (savedData)
                            context->makeDataPersistent(savedData, false, nodeID);

                        // Save and persist the new data.
                        savedData = event->data;
                        context->makeDataPersistent(savedData, true, nodeID);
                    }
                    addEvent(0, event);
                }
            }
        }

        if (dataUpdated)
        {
            listChanged.store(true);
        }

        if (triggerSendFromAudio.load())
        {
            triggerSendFromAudio.store(false);

            savedData->getAtom(0)->toString(newTextBuffer);

            if (newTextBuffer != textBuffer) // Only update if content changed
            {
                textBuffer = newTextBuffer;

                if (queueFromDSP.try_enqueue(textBuffer))
                {
                    auto listBoxUI = reinterpret_cast<ListBox::UI*>(getOrCreateUI());
                    if (listBoxUI && !listBoxUI->shouldRepaint.exchange(true)) {
                        updateUI(); // Only trigger if not already dirty
                    }
                }
            }
        }

        while (queueToDSP.try_dequeue(textBuffer))
        {
            listText = textBuffer;
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
            addEvent(0, newEvent);
        }
    }
};

REGISTER(ListBox);
