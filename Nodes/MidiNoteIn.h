//
// Created by alexw on 4/03/2025.
//

#pragma once

#include "AudioNodeBase.h"
#include <cmath>

class MidiNoteIn : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("MidiNoteIn", "notein", true);
    DEFINE_NODE_ALIASES("midinotein", "notein");

public:
    MidiNoteIn(std::shared_ptr<NodeContext> context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
    }

    json getSerializedNode() override
    {
        return nodeCreationData;
    }

    void processAudio(const float* in, float* out, unsigned long frameCount, std::vector<MidiMessage>& midiMessages) override
    {
        for (const auto& [message, timestamp] : midiMessages)
        {
            if (message.size() >= 3)
            {
                unsigned char status = message[0];
                unsigned char statusType = status & 0xF0;

                // Check if the message is a Note On (0x90-0x9F) or Note Off: 0x80
                if (statusType == 0x90 || statusType == 0x80)
                {
                    unsigned char velocity = message[2];
                    unsigned char noteNumber = message[1];

                    // **Allocate a new event**
                    if (auto* e = context->eventPool.getFreeEvent())
                    {
                        context->eventPool.addDataAtomTo(e, noteNumber);
                        context->eventPool.addDataAtomTo(e, velocity);

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
                        addEvent(0, e);

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

REGISTER(MidiNoteIn);
