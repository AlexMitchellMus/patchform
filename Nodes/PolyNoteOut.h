class PolyNoteOut : public AudioNode {
    DEFINE_AND_REGISTER_NODE("PolyNoteOut", "polyNoteOut", false);
    DEFINE_NODE_ALIASES("polynoteout");

    struct VoiceSlot {
        bool active = false;
        float note = -1.0f;
    };

    std::vector<VoiceSlot> voices;
    int nextIndex = 0;

public:
    PolyNoteOut(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        int numVoices = objParams.value("voices", 8);
        voices.resize(numVoices);

        for (int i = 0; i < numVoices; ++i)
            addOutputPort("v" + std::to_string(i), AudioPort::PortType::Data);

        addInputPort("midi", AudioPort::PortType::Data);
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override {
        const auto& events = inputPortBuffers[0]->getEvents();

        for (auto* ev : events) {
            auto tag = ev->getTagHash();
            float note = ev->getAtomValue(0);

            if (tag == hash("note-on")) {
                // Skip if note is already active
                bool alreadyAssigned = false;
                for (const auto& v : voices)
                    if (v.active && v.note == note)
                        alreadyAssigned = true;

                if (alreadyAssigned) continue;

                // Assign to next free slot (or steal round-robin)
                int slot = -1;
                for (int i = 0; i < voices.size(); ++i) {
                    if (!voices[i].active) {
                        slot = i;
                        break;
                    }
                }
                if (slot == -1) { // no free, steal
                    slot = nextIndex;
                    nextIndex = (nextIndex + 1) % voices.size();
                }

                voices[slot].active = true;
                voices[slot].note = note;

                if (auto* e = context->eventPool.getFreeEvent()) {
                    e->setTagHashcode(tag);
                    context->eventPool.addDataAtomTo(e, note);
                    addEvent(slot, e);
                }
            }

            else if (tag == hash("note-off")) {
                for (int i = 0; i < voices.size(); ++i) {
                    if (voices[i].active && voices[i].note == note) {
                        voices[i].active = false;

                        if (auto* e = context->eventPool.getFreeEvent()) {
                            e->setTagHashcode(tag);
                            context->eventPool.addDataAtomTo(e, note);
                            addEvent(i, e);
                        }
                        break;
                    }
                }
            }
        }
    }
};

REGISTER(PolyNoteOut);
