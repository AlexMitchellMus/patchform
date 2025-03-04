//
// Created by alexw on 4/03/2025.
//

#pragma once

#include "MidiNodeBase.h"
#include <cmath>

class MidiNoteIn : public MidiNode
{
    DEFINE_AND_REGISTER_NODE("MidiNoteIn", "notein");

    float midiNote = 60.0f; // Default to middle C

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
                    // Optional: Only consider non-zero velocity as Note On.
                    unsigned char velocity = midiMessage.message[2];
                    if (velocity != 0)
                    {
                        unsigned char noteNumber = midiMessage.message[1];
                        if (auto* e = context->eventPool.getFreeEvent())
                        {
                            e->data = noteNumber;
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

