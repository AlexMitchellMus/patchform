#pragma once

#include "AudioNodeBase.h"

// ====================================
// Inlet/Outlet Helpers
// ====================================

class InletOutletHelpers {
public:
    static void copyAudioBuffers(const AudioNode* node, const unsigned long frames)
    {
        if (node && node->inputPortBuffers.size() > 0 && node->outputPortBuffers.size() > 0) {
            std::memcpy(node->outputPortBuffers[0]->getAudioBuffer(), node->inputPortBuffers[0]->getAudioBuffer(), frames * sizeof(float));
        }
    }
};

// ====================================
// Inlet Node
// ====================================

class Inlet : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Inlet", "inlet", true);
    DEFINE_NODE_ALIASES("inlet");
public:
    Inlet(std::shared_ptr<NodeContext> ctx, const json& objParams)
        : AudioNode(ctx, AudioPort::Signal, objParams)
    {
    }

    void processAudio(const float*, float*, unsigned long frames, std::vector<MidiMessage>&) override
    {
        InletOutletHelpers::copyAudioBuffers(this, frames);
    }
};

REGISTER(Inlet);

// ====================================
// Outlet Node
// ====================================

class Outlet : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Outlet", "outlet", true);
    DEFINE_NODE_ALIASES("outlet");
public:
    Outlet(std::shared_ptr<NodeContext> ctx, const json& objParams)
        : AudioNode(ctx, AudioPort::None, objParams)
    {
        addInputPort("in", AudioPort::Signal);
    }

    void process(const float* inBuffer, float* buffer, std::vector<MidiMessage>& midiMessage, unsigned long frameCount, Graph& g, int index) override
    {
        // We bypass all processing for outlet
        // This is because we only use the input port and copy it to the subpatch output, which means we can't clear it after it has processed
        // So we clear the inlet 'after' the subpatch has fully processed in the subpatch node
    }
};

REGISTER(Outlet);