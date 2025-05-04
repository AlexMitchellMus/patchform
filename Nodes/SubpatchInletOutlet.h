#pragma once

#include "AudioNodeBase.h"

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

    void process(const float* inBuffer, float* buffer, std::vector<MidiMessage>& midiMessage, unsigned long frameCount, Graph& g, const size_t index) override
    {
        // We bypass all processing for outlet
        // This is because we only use the input port and copy it to the subpatch output, which means we can't clear it after it has processed
        // So we clear the inlet 'after' the subpatch has fully processed in the subpatch node
    }
};

REGISTER(Outlet);

// ====================================
// Data Inlet Node
// ====================================

class DataInlet : public AudioNode {
    DEFINE_AND_REGISTER_NODE("d.inlet", "d.inlet", false);
    DEFINE_NODE_ALIASES("d.inlet");
public:
    DataInlet(std::shared_ptr<NodeContext> ctx, const json& objParams) : AudioNode(ctx, AudioPort::Data, objParams)
    {
    }
};

REGISTER(DataInlet);

// ====================================
// Data Outlet Node
// ====================================

class DataOutlet : public AudioNode {
    DEFINE_AND_REGISTER_NODE("d.outlet", "d.outlet", false);
    DEFINE_NODE_ALIASES("d.outlet");
public:
    DataOutlet(std::shared_ptr<NodeContext> ctx, const json& objParams)
        : AudioNode(ctx, AudioPort::None, objParams)
    {
        addInputPort("in", AudioPort::Data);
    }

    void process(const float* inBuffer, float* buffer, std::vector<MidiMessage>& midiMessage, unsigned long frameCount, Graph& g, const size_t index) override
    {
        // Totally bypass clearing events from the input as we forward them inside the subpatch
    }
};

REGISTER(DataOutlet);