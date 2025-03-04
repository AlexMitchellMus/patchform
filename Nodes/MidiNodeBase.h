//
// Created by alexw on 4/03/2025.
//

#pragma once

#include "AudioNodeBase.h"

struct MidiMessage {
    std::vector<unsigned char> message;
    double timestamp;
};

class MidiNode : public AudioNode {
public:
    MidiNode(NodeContext* context, AudioPort::PortType portType, const json& objParams)
        : AudioNode(context, portType, objParams) {}

    virtual void processMidi(std::vector<MidiMessage>& message) = 0;
};