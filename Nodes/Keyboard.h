#pragma once

class Keyboard final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Keyboard", "keyboard", false);
    DEFINE_NODE_ALIASES("keyboard");

public:
    std::atomic<int> noteState[128] = {};
    std::atomic<int> selectedNote = 48; // C3
    std::atomic<bool> emitOnClick = true;

    std::atomic<bool> isVertical = false;
    BoolParameter* isVerticalParam = nullptr;

    std::atomic<bool> holdMode = false;
    BoolParameter* holdModeParam = nullptr;

    std::atomic<bool> isDirty = false;
    std::atomic<bool> hasUI = false;

#ifdef PATCHFORM_WITH_GUI
    moodycamel::ConcurrentQueue<int> eventQueue;
    moodycamel::ConcurrentQueue<int> eventQueueFromDSP;

    bool isDefaultUI() const override { return false; }

    class UI final : public AudioNode::UI
    {
        const int keyWidth = 20;
        const int keyHeight = 120;
        const int baseMidiNote = 36; // C2
        const int totalKeys = 48; // C2 to B5
        int lastNotePressed = -1;

        bool verticalLayout = false;

        bool holdModeVal = false;

        std::atomic<int> noteState[128] = {};

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(getKeyboardWidth(baseMidiNote, totalKeys, keyWidth), keyHeight);

            auto kb = reinterpret_cast<Keyboard*>(audioNode);

            kb->isVerticalParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable
            {
                if (auto isVertical = std::get_if<int>(&value))
                {
                    bool newVertical = (*isVertical != 0);
                    if (verticalLayout != newVertical)
                    {
                        verticalLayout = newVertical;

                        // Update layout logic, but don't call setSize.
                        // Instead, get current bounds and just repaint/redraw.
                        repaint();

                        if (const auto cnv = findParentOfClass<Canvas>())
                            cnv->updateConnectionsPosition();
                    }
                }
            };

            kb->holdModeParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable
            {
                if (auto isHoldMode = std::get_if<int>(&value))
                {
                    holdModeVal = (*isHoldMode != 0);
                }
            };

            holdModeVal = kb->holdMode.load();
        }

        ~UI() override
        {
            const auto kb = reinterpret_cast<Keyboard*>(audioNode);
            kb->hasUI.store(false);
        }

        void updateGraphValues() override
        {
            auto kb = reinterpret_cast<Keyboard*>(audioNode);
            if (kb->isDirty.exchange(false))
            {
                int note = 0;
                while (kb->eventQueueFromDSP.try_dequeue(note))
                {
                    if (note < 0)
                        noteState[-note] = false;
                    else
                        noteState[note] = true;

                    repaint();
                }
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (!cnv->isInLockedMode())
                {
                    AudioNode::UI::mouseButtonDown(e);
                    return;
                }

                float mouseX = static_cast<float>(e.sdlEvent.button.x);
                float mouseY = static_cast<float>(e.sdlEvent.button.y);
                int whiteCount = getWhiteKeyCount();
                float scaledKeyWidth = getWidth() / static_cast<float>(whiteCount);
                float scaledKeyHeight = static_cast<float>(getHeight());

                int note = -1;
                int whiteIndex = 0;

                // First pass: black keys
                for (int i = 0; i < totalKeys; ++i)
                {
                    int midiNote = baseMidiNote + i;
                    if (!isBlackKey(midiNote))
                    {
                        whiteIndex++;
                        continue;
                    }

                    float x = whiteIndex * scaledKeyWidth - (scaledKeyWidth * 0.25f);
                    if (mouseX >= x && mouseX < x + scaledKeyWidth * 0.5f && mouseY < scaledKeyHeight * 0.6f)
                    {
                        note = midiNote;
                        break;
                    }
                }

                // Second pass: white keys
                if (note == -1)
                {
                    whiteIndex = 0;
                    for (int i = 0; i < totalKeys; ++i)
                    {
                        int midiNote = baseMidiNote + i;
                        if (isBlackKey(midiNote)) continue;

                        float x = whiteIndex * scaledKeyWidth;
                        if (mouseX >= x && mouseX < x + scaledKeyWidth)
                        {
                            note = midiNote;
                            break;
                        }
                        whiteIndex++;
                    }
                }

                auto* kb = reinterpret_cast<Keyboard*>(audioNode);
                if (holdModeVal && kb->selectedNote != note)
                    kb->eventQueue.enqueue(-kb->selectedNote.load());

                kb->selectedNote.store(note);
                lastNotePressed = note;
                kb->eventQueue.enqueue(note);
                kb->setNodeDirty();
                repaint();
            }
        }

        void mouseButtonUp(pptk::CompEvent& e) override
        {
            auto* kb = reinterpret_cast<Keyboard*>(audioNode);
            if (!holdModeVal && lastNotePressed >= 0)
            {
                kb->eventQueue.enqueue(-lastNotePressed);
                kb->setNodeDirty();
                repaint();
                lastNotePressed = -1;
            }

            AudioNode::UI::mouseButtonUp(e);
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& point, pptk::Button button) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (!cnv->isInLockedMode())
                {
                    AudioNode::UI::mouseDrag(position, point, button);
                    return;
                }

                auto* kb = reinterpret_cast<Keyboard*>(audioNode);
                if (!kb) return;

                float mouseX = position.x;
                float mouseY = position.y;
                int whiteCount = getWhiteKeyCount();
                float scaledKeyWidth = getWidth() / static_cast<float>(whiteCount);
                float scaledKeyHeight = static_cast<float>(getHeight());

                int hoveredNote = -1;
                int whiteIndex = 0;

                // First pass: black keys
                for (int i = 0; i < totalKeys; ++i)
                {
                    int midiNote = baseMidiNote + i;
                    if (!isBlackKey(midiNote))
                    {
                        whiteIndex++;
                        continue;
                    }

                    float x = whiteIndex * scaledKeyWidth - (scaledKeyWidth * 0.25f);
                    if (mouseX >= x && mouseX < x + scaledKeyWidth * 0.5f && mouseY < scaledKeyHeight * 0.6f)
                    {
                        hoveredNote = midiNote;
                        break;
                    }
                }

                // Second pass: white keys
                if (hoveredNote == -1)
                {
                    whiteIndex = 0;
                    for (int i = 0; i < totalKeys; ++i)
                    {
                        int midiNote = baseMidiNote + i;
                        if (isBlackKey(midiNote)) continue;

                        float x = whiteIndex * scaledKeyWidth;
                        if (mouseX >= x && mouseX < x + scaledKeyWidth)
                        {
                            hoveredNote = midiNote;
                            break;
                        }
                        whiteIndex++;
                    }
                }

                if (hoveredNote >= 0 && hoveredNote != lastNotePressed)
                {
                    if (lastNotePressed >= 0)
                        kb->eventQueue.enqueue(-lastNotePressed);

                    kb->eventQueue.enqueue(hoveredNote);
                    kb->setNodeDirty();
                    repaint();
                    lastNotePressed = hoveredNote;
                }
            }
        }


        void drawGUI(NVGcontext* vg) override
        {
            if (verticalLayout)
                drawVertical(vg);
            else
                drawHorizontal(vg);
        }

        void drawVertical(NVGcontext* vg) const
        {
            auto* kb = reinterpret_cast<Keyboard*>(audioNode);
            const auto white = nvgRGB(200, 200, 200);

            const auto selectedNoteCol = nvgRGB(36, 130, 210);

            nvgDrawRoundedRect(vg, 1, 1, getWidth() - 2, getHeight() - 2, white, white, 5);

            int whiteCount = getWhiteKeyCount();
            float keyHeightAdjusted = static_cast<float>(getHeight()) / whiteCount;
            int whiteIndex = 0;

            // Highlight selected white key
            for (int n = 0; n < totalKeys; ++n)
            {
                int i = totalKeys - 1 - n;
                int midiNote = baseMidiNote + i;
                if (isBlackKey(midiNote)) continue;

                float y = whiteIndex * keyHeightAdjusted;
                float h = keyHeightAdjusted;
                float w = getWidth();

                if (midiNote == kb->selectedNote.load())
                {
                    nvgBeginPath(vg);
                    if (whiteIndex == 0)
                    {
                        nvgRoundedRectVarying(vg, 0, y + 1, w, h - 2, 5, 5, 0, 0);
                    }
                    else if (i == 0 || whiteIndex == whiteCount - 1)
                    {
                        nvgRoundedRectVarying(vg, 0, y, w, h - 1, 0, 0, 5, 5);
                    }
                    else
                    {
                        nvgRect(vg, 0, y, w, h - 2);
                    }
                    nvgFillColor(vg, selectedNoteCol);
                    nvgFill(vg);
                }
                whiteIndex++;
            }

            // Divider lines
            nvgBeginPath(vg);
            whiteIndex = 0;
            for (int n = 0; n < totalKeys; ++n)
            {
                int i = totalKeys - 1 - n;
                int midiNote = baseMidiNote + i;
                if (isBlackKey(midiNote)) continue;

                float y = whiteIndex * keyHeightAdjusted;

                if (whiteIndex > 0)
                {
                    nvgMoveTo(vg, 0, y);
                    nvgLineTo(vg, width - 1, y);
                }
                whiteIndex++;
            }
            nvgStrokeColor(vg, nvgRGB(0, 0, 0));
            nvgStrokeWidth(vg, 1.0f);
            nvgLineStyle(vg, NVG_SOLID);
            nvgStroke(vg);

            // Draw black keys
            whiteIndex = 0;
            for (int n = 0; n < totalKeys; ++n)
            {
                int i = totalKeys - 1 - n;
                int midiNote = baseMidiNote + i;
                if (!isBlackKey(midiNote))
                {
                    whiteIndex++;
                    continue;
                }

                float y = whiteIndex * keyHeightAdjusted - (keyHeightAdjusted / 4.0f);
                float h = keyHeightAdjusted * 0.5f;
                float w = getWidth() * 0.6f;
                NVGcolor col = (midiNote == kb->selectedNote.load()) ? selectedNoteCol : nvgRGB(5, 5, 5);
                NVGcolor outline = nvgRGB(0, 0, 0);
                nvgDrawRoundedRect(vg, 0, y, w, h, col, outline, 0);
            }
        }

        void drawHorizontal(NVGcontext* vg) const
        {
            int whiteKeyCount = getWhiteKeyCount();
            float scaledKeyWidth = getWidth() / whiteKeyCount;
            float scaledKeyHeight = getHeight();

            int whiteIndex = 0;

            const auto white = nvgRGB(200, 200, 200);
            const auto whiteSelCol = nvgRGB(36, 130, 210);
            const auto blackSelCol = nvgRGB(whiteSelCol.r / 2, whiteSelCol.g / 2, whiteSelCol.b / 2);

            nvgDrawRoundedRect(vg, 1, 1, getWidth() - 2, getHeight() - 2, white, white, 5);

            // Prepass: draw highlight behind white keys
            whiteIndex = 0;
            for (int i = 0; i < totalKeys; ++i)
            {
                int midiNote = baseMidiNote + i;
                int noteInOctave = midiNote % 12;
                bool isBlack = noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 ||
                    noteInOctave == 10;
                if (isBlack) continue;

                int x = whiteIndex * scaledKeyWidth;
                if (noteState[midiNote])
                {
                    nvgBeginPath(vg);
                    if (whiteIndex == 0)
                    {
                        // Leftmost key: round left corners
                        nvgRoundedRectVarying(vg, x + 1, 1, scaledKeyWidth, scaledKeyHeight - 2, 5, 0, 0, 5);
                    }
                    else if (i == totalKeys - 1 || whiteIndex == getWhiteKeyCount())
                    {
                        // Rightmost key: round right corners
                        nvgRoundedRectVarying(vg, x, 1, scaledKeyWidth - 1, scaledKeyHeight - 2, 0, 5, 5, 0);
                    }
                    else
                    {
                        nvgRect(vg, x, 1, scaledKeyWidth, scaledKeyHeight - 2);
                    }
                    nvgFillColor(vg, whiteSelCol);
                    nvgFill(vg);
                }

                whiteIndex++;
            }

            // Vertical lines for white key divisions
            nvgBeginPath(vg);
            whiteIndex = 0;
            for (int i = 0; i < totalKeys; ++i)
            {
                int midiNote = baseMidiNote + i;
                if (isBlackKey(midiNote)) continue;

                const auto x = whiteIndex * scaledKeyWidth;
                if (whiteIndex > 0)
                {
                    nvgMoveTo(vg, x, isWhiteKeyAdjacent(midiNote) ? 1 : scaledKeyHeight * 0.6f);
                    nvgLineTo(vg, x, scaledKeyHeight - 1);
                }
                whiteIndex++;
            }
            nvgStrokeColor(vg, nvgRGB(0, 0, 0));
            nvgStrokeWidth(vg, 1.0f);
            nvgLineStyle(vg, NVG_SOLID);
            nvgStroke(vg);

            // Draw black keys
            whiteIndex = 0;
            for (int i = 0; i < totalKeys; ++i)
            {
                int midiNote = baseMidiNote + i;
                if (!isBlackKey(midiNote))
                {
                    whiteIndex++;
                    continue;
                }

                const auto x = whiteIndex * scaledKeyWidth - (scaledKeyWidth / 4);
                const auto blackCol = nvgRGB(0, 0, 0);
                const auto col = noteState[midiNote] ? blackSelCol : nvgRGB(0, 0, 0);
                nvgDrawRoundedRect(vg, x, 1, scaledKeyWidth * 0.5f, scaledKeyHeight * 0.6f, col, blackCol, 0);
            }
        }

        static bool isBlackKey(const int midiNote)
        {
            int noteInOctave = midiNote % 12;
            return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave ==
                10;
        }

        static bool isWhiteKeyAdjacent(const int midiNote)
        {
            int noteInOctave = midiNote % 12;
            return noteInOctave == 5 || noteInOctave == 0;
        }

        static int getKeyboardWidth(const int startNote, const int totalKeys, const int keyWidth)
        {
            int whiteKeyCount = 0;

            for (int i = 0; i < totalKeys; ++i)
            {
                int midiNote = startNote + i;
                int noteInOctave = midiNote % 12;

                bool isWhite = !(noteInOctave == 1 || noteInOctave == 3 ||
                    noteInOctave == 6 || noteInOctave == 8 ||
                    noteInOctave == 10);

                if (isWhite)
                    whiteKeyCount++;
            }

            return whiteKeyCount * keyWidth;
        }

        int getWhiteKeyCount() const
        {
            int count = 0;
            for (int i = 0; i < totalKeys; ++i)
            {
                int n = (baseMidiNote + i) % 12;
                if (!(n == 1 || n == 3 || n == 6 || n == 8 || n == 10)) ++count;
            }
            return count;
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        hasUI.store(true);
        return std::make_unique<UI>(this);
    }
#endif

    Keyboard(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        isVertical = objParams.value("Vertical", false);
        isVerticalParam = addParameter<BoolParameter>("Vertical", isVertical.load());

        holdMode = objParams.value("holdMode", false);
        holdModeParam = addParameter<BoolParameter>("Hold", holdMode);

        holdModeParam->informNodeOfChange = [this]() { holdMode.store(holdModeParam->getValue()); };

        addInputPort("midi-in", AudioPort::PortType::Data);
    }

    json getSerializedNode() override
    {
        nodeCreationData["selectedNote"] = selectedNote.load();
        nodeCreationData["holdMode"] = holdMode.load();
        nodeCreationData["emitOnClick"] = emitOnClick.load();
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        auto& inputEv = inputPortBuffers[0]->getEvents();

        for (const auto* ev : inputEv) // "midi-in" is input port 0
        {
            const auto tag = ev->getTagHash();
            if (tag != hash("note-on") && tag != hash("note-off"))
                continue;

            if (!ev->data)
                continue;

            int note = static_cast<int>(ev->data->data.atom);
            bool isOn = (tag == hash("note-on"));

            selectedNote = note;
            if (note < 0 || note >= 128)
                continue;
            noteState[note] = isOn ? 1 : 0;

            if (Event* out = context->eventPool.getFreeEvent())
            {
                out->shallowCopyFrom(ev);
                addEvent(0, out);
            }

#ifdef PATCHFORM_WITH_GUI
            if (hasUI.load())
            {
                eventQueueFromDSP.enqueue(isOn ? note : -note);
                isDirty.store(true);
            }
#endif
        }

        int note;
        while (eventQueue.try_dequeue(note))
        {
            bool isOff = note < 0;
            note = std::abs(note);

            selectedNote = note;

            if (note < 0 || note > 127)
                continue;

            bool isNoteOn = false;
            if (isOff)
                noteState[note] = 0;
            else
                isNoteOn = ++noteState[note] == 1;

            if (emitOnClick)
            {
                if (Event* e = context->eventPool.getFreeEvent())
                {
                    context->eventPool.addDataAtomTo(e, note);
                    e->setTagHashcode(isNoteOn ? hash("note-on") : hash("note-off"));
                    addEvent(0, e);
                }
            }
            if (hasUI.load())
            {
                eventQueueFromDSP.enqueue(isNoteOn ? note : -note);
                isDirty.store(true);
            }
        }
    }
#endif
};

REGISTER(Keyboard);
