//
// Created by alexw on 4/03/2025.
//

#pragma once

#include "MidiNodeBase.h"
#include <cmath>

class MidiNoteIn : public MidiNode
{
    DEFINE_AND_REGISTER_NODE("MidiNoteIn", "notein", false);

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
                unsigned char statusType = status & 0xF0;

                // Check if the message is a Note On (0x90-0x9F) or Note Off: 0x80
                if (statusType == 0x90 || statusType == 0x80)
                {
                    unsigned char velocity = midiMessage.message[2];
                    unsigned char noteNumber = midiMessage.message[1];

                    // **Allocate a new event**
                    if (auto* e = context->eventPool.getFreeEvent())
                    {
                        e->addAtom(static_cast<float>(noteNumber));
                        e->addAtom(static_cast<float>(velocity));

                        // Use velocity 0 on Note On messages as Note Off
                        if (statusType == 0x90 && velocity > 0)
                        {
                            e->setTag("note-on");
                        }
                        else
                        {
                            e->setTag("note-off");
                        }

                        e->numAtoms = 2;
                        outputPortBuffers[0]->addEvent(e);

#ifdef DEBUG_MIDI
                        std::cout << "MidiNoteIn: Outputting Note-On List { " << noteNumber << ", " << velocity << " }" << std::endl;
#endif
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
