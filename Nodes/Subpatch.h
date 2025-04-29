// SubpatchNode.h
#pragma once

#include "AudioNodeBase.h"

class GraphManager;
class GraphHolder;
class Subpatch final : public AudioNode
{

public:
    DEFINE_AND_REGISTER_NODE("Subpatch", "subpatch", true);
    DEFINE_NODE_ALIASES("subpatch");

    std::shared_ptr<GraphManager> subManager;
    std::vector<AudioPort*> subInputs;
    std::vector<AudioPort*> subOutputs;

    Subpatch(std::shared_ptr<NodeContext> context, const json& creationData);

    void setupSubgraph(const json& subpatchJson);
    void rebuildPortsFromGraph(const GraphHolder& graph);

    void process(const float* inBuffer, float* outBuffer, std::vector<MidiMessage>& midi, unsigned long frames, Graph& g, int index) override;

    json getSerializedNode() override;

    GraphManager* getSubgraph() const
    {
        return subManager.get();
    }
};

REGISTER(Subpatch);
