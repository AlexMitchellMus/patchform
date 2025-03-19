//
// Created by alexw on 4/03/2025.
//

#pragma once

#include "MidiNodeBase.h"
#include <cmath>

class MidiNoteIn : public MidiNode
{
    DEFINE_AND_REGISTER_NODE("MidiNoteIn", "notein");

public:
    MidiNoteIn(NodeContext* context, const json& objParams) : MidiNode(context, AudioPort::PortType::Data, objParams)
    {
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processMidi(std::vector<MidiMessage>& messages) override
    {
        for (auto& midiMessage : messages)
        {
            if (midiMessage.message.size() >= 3)
            {
                unsigned char status = midiMessage.message[0];

                // Check if the message is a Note On (0x90-0x9F)
                if ((status & 0xF0) == 0x90)
                {
                    unsigned char velocity = midiMessage.message[2];

                    // Optional: Only consider non-zero velocity as Note On.
                    if (velocity != 0)
                    {
                        unsigned char noteNumber = midiMessage.message[1];

                        // **Allocate a new event**
                        if (auto* e = context->eventPool.getFreeEvent())
                        {
                            // **Create a parent atom for the list**
                            DataAtom* listAtom = context->eventPool.allocateDataAtom();
                            if (!listAtom) return;

                            listAtom->type = DataAtom::DataType::List;
                            listAtom->data.list = nullptr; // Start empty
                            listAtom->next = nullptr;

                            // **Create atoms for note number & velocity**
                            DataAtom* noteAtom = context->eventPool.allocateDataAtom();
                            if (!noteAtom) return;
                            noteAtom->type = DataAtom::DataType::Float;
                            noteAtom->data.atom = static_cast<float>(noteNumber);
                            noteAtom->next = nullptr;

                            DataAtom* velocityAtom = context->eventPool.allocateDataAtom();
                            if (!velocityAtom) return;
                            velocityAtom->type = DataAtom::DataType::Float;
                            velocityAtom->data.atom = static_cast<float>(velocity);
                            velocityAtom->next = nullptr;

                            // **Attach note and velocity inside the list**
                            listAtom->data.list = noteAtom;
                            noteAtom->next = velocityAtom;

                            // **Assign list to event**
                            e->data = listAtom;
                            e->numAtoms = 2;

#ifdef DEBUG_MIDI
                            std::cout << "MidiNoteIn: Outputting Note-On List { "
                                      << noteNumber << ", " << velocity << " }" << std::endl;
#endif

                            outputPort.addEvent(e);
                        }
                    }
                }
            }
            else
            {
                std::cerr << "Incomplete MIDI message received\n";
            }
        }
    }
};
