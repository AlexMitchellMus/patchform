#pragma once

#include "AudioNodeBase.h"
#include <vector>
#include <algorithm>
#include <string>

class ActiveMidiNotes : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("ActiveMidiNotes", "activeMidiNotes", false);

    // Holds the list of currently active notes.
    // We store note numbers as float, consistent with event atom usage.
    std::vector<float> activeNotes;

public:
    ActiveMidiNotes(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        addInputPort("In", AudioPort::PortType::Data);

        // Reserve space for up to 127 active MIDI notes.
        activeNotes.reserve(127);
    }

    void processAudio(float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        // Retrieve incoming events.
        const auto& events = inputPortBuffers[0]->getEvents();

        // Process each event.
        for (const auto* ev : events)
        {
            auto tag = ev->getTagHash();
            // Assume that the note number is stored as the first atom.
            float note = ev->getAtomValue(0);

            switch (tag)
            {
            case hash("note-on"):
                {
                    // Insert note in sorted order if not already present.
                    auto pos = std::lower_bound(activeNotes.begin(), activeNotes.end(), note);
                    if (pos == activeNotes.end() || *pos != note)
                    {
                        activeNotes.insert(pos, note);
                    }
                    break;
                }
            case hash("note-off"):
                {
                    // Find the note in the sorted list and remove it.
                    auto it = std::lower_bound(activeNotes.begin(), activeNotes.end(), note);
                    if (it != activeNotes.end() && *it == note)
                    {
                        activeNotes.erase(it);
                    }
                    break;
                }
            default:
                {
                    // Other event types are ignored.
                    continue;
                }
            }

            // Create a new event that outputs the list of active notes.
            if (auto* e = context->eventPool.getFreeEvent())
            {
                for (const auto note : activeNotes)
                {
                    e->addAtom(note);
                }
                e->numAtoms = static_cast<int>(activeNotes.size());
                e->setTimeStamp(ev->getTimeStamp());
                outputPortBuffers[0]->addEvent(e);
            }
        }
    }
};
