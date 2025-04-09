#pragma once

class Keyboard final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Keyboard", "keyboard", false);

public:
    std::atomic<int> selectedNote = 48; // C3
    std::atomic<bool> emitOnClick = true;

    std::atomic<bool> isVertical = false;
    BoolParameter* isVerticalParam = nullptr;

#ifdef PATCHFORM_WITH_GUI
    std::function<void()> repaintFromDSP = [](){};
    moodycamel::ConcurrentQueue<int> eventQueue;
    moodycamel::ConcurrentQueue<int> eventQueueFromDSP;

    bool isDefaultUI() const override { return false; }

    class UI final : public AudioNode::UI
    {
        const int keyWidth = 20;
        const int keyHeight = 120;
        const int baseMidiNote = 36; // C2
        const int totalKeys = 48; // C2 to B5

        bool verticalLayout = false;

    public:
        std::atomic<bool> isDirty = false;

        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(getKeyboardWidth(baseMidiNote, totalKeys, keyWidth), keyHeight);

            auto kb = reinterpret_cast<Keyboard*>(audioNode);
            kb->repaintFromDSP = [this]() { isDirty.store(true); };

            kb->isVerticalParam->updateNodeUI = [this](const std::variant<int, float, std::string>& value) mutable
            {
                if (auto isVertical = std::get_if<int>(&value))
                {
                    bool newVertical = (*isVertical != 0);
                    if (verticalLayout != newVertical)
                    {
                        verticalLayout = newVertical;
                        if (verticalLayout)
                            setSize(keyHeight, getKeyboardWidth(baseMidiNote, totalKeys, keyWidth));
                        else
                            setSize(getKeyboardWidth(baseMidiNote, totalKeys, keyWidth), keyHeight);

                        repaint();
                    }
                }
            };
        }

        void updateGraphValues() override
        {
            if (isDirty.exchange(false))
            {
                repaint();
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    int mouseX = e.sdlEvent.button.x;
                    int mouseY = e.sdlEvent.button.y;

                    int note = -1;
                    int whiteIndex = 0;

                    // First pass: check black keys
                    for (int i = 0; i < totalKeys; ++i)
                    {
                        int midiNote = baseMidiNote + i;
                        int noteInOctave = midiNote % 12;
                        bool isBlack = noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8
                            || noteInOctave == 10;
                        if (!isBlack)
                        {
                            whiteIndex++;
                            continue;
                        }

                        int x = whiteIndex * keyWidth - (keyWidth / 4);
                        if (mouseX >= x && mouseX < x + keyWidth / 2 && mouseY < keyHeight * 0.6f)
                        {
                            note = midiNote;
                            break;
                        }
                    }

                    // Second pass: fallback to white keys
                    if (note == -1)
                    {
                        whiteIndex = 0;
                        for (int i = 0; i < totalKeys; ++i)
                        {
                            int midiNote = baseMidiNote + i;
                            int noteInOctave = midiNote % 12;
                            bool isBlack = noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave
                                == 8 || noteInOctave == 10;
                            if (isBlack) continue;

                            int x = whiteIndex * keyWidth;
                            if (mouseX >= x && mouseX < x + keyWidth)
                            {
                                note = midiNote;
                                break;
                            }
                            whiteIndex++;
                        }
                    }

                    auto* kb = reinterpret_cast<Keyboard*>(audioNode);
                    kb->selectedNote = note;
                    kb->eventQueue.enqueue(note);
                    kb->setNodeDirty();
                    repaint();
                }
            }
            AudioNode::UI::mouseButtonDown(e);
        }


        void drawGUI(NVGcontext* vg) override
        {
            auto* kb = reinterpret_cast<Keyboard*>(audioNode);
            int whiteIndex = 0;

            const auto white = nvgRGB(200, 200, 200);

            nvgDrawRoundedRect(vg, 1, 1, getWidth() - 2, getHeight() - 2, white, white, 5);

            // Prepass: draw highlight behind white keys
            whiteIndex = 0;
            for (int i = 0; i < totalKeys; ++i)
            {
                int midiNote = baseMidiNote + i;
                int noteInOctave = midiNote % 12;
                bool isBlack = noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
                if (isBlack) continue;

                int x = whiteIndex * keyWidth;
                if (midiNote == kb->selectedNote)
                {
                    nvgBeginPath(vg);
                    if (whiteIndex == 0) {
                        // Leftmost key: round left corners
                        nvgRoundedRectVarying(vg, x + 1, 1, keyWidth, keyHeight - 2, 5, 0, 0, 5);
                    }
                    else if (i == totalKeys - 1 || whiteIndex == getWhiteKeyCount()) {
                        // Rightmost key: round right corners
                        nvgRoundedRectVarying(vg, x, 1, keyWidth - 1, keyHeight - 2, 0, 5, 5, 0);
                    }
                    else {
                        nvgRect(vg, x, 1, keyWidth, keyHeight - 2);
                    }
                    nvgFillColor(vg, nvgRGB(100, 100, 100));
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

                int x = whiteIndex * keyWidth;
                if (whiteIndex > 0) {
                    nvgMoveTo(vg, x, isWhiteKeyAdjacent(midiNote) ? 1 : keyHeight * 0.6f);
                    nvgLineTo(vg, x, keyHeight - 1);
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

                int x = whiteIndex * keyWidth - (keyWidth / 4);
                NVGcolor col = (midiNote == kb->selectedNote) ? nvgRGB(50, 50, 50) : nvgRGB(0, 0, 0);
                nvgDrawRoundedRect(vg, x, 1, keyWidth * 0.5f, keyHeight * 0.6f, col, col, 0);
            }
        }

        bool isBlackKey(int midiNote)
        {
            int noteInOctave = midiNote % 12;
            return noteInOctave == 1 || noteInOctave == 3 || noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
        }

        bool isWhiteKeyAdjacent(int midiNote)
        {
            int noteInOctave = midiNote % 12;
            return noteInOctave == 5 || noteInOctave == 0;
        }

        int getKeyboardWidth(int startNote, int totalKeys, int keyWidth)
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

        int getWhiteKeyCount() const {
            int count = 0;
            for (int i = 0; i < totalKeys; ++i) {
                int n = (baseMidiNote + i) % 12;
                if (!(n == 1 || n == 3 || n == 6 || n == 8 || n == 10)) ++count;
            }
            return count;
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    Keyboard(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addOutputPort("note", AudioPort::PortType::Data);

        isVertical = objParams.value("Vertical", false);
        isVerticalParam = addParameter<BoolParameter>("Vertical", isVertical.load());
    }

    json getSerializedNode() override
    {
        nodeCreationData["selectedNote"] = selectedNote.load();
        nodeCreationData["emitOnClick"] = emitOnClick.load();
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        int note;
        while (eventQueue.try_dequeue(note))
        {
            selectedNote = note;
            if (emitOnClick)
            {
                if (Event* e = context->eventPool.getFreeEvent())
                {
                    context->eventPool.addDataAtomTo(e, note);
                    addEvent(0, e);
                }
            }
            eventQueueFromDSP.enqueue(note);
            repaintFromDSP();
        }
    }
#endif
};
