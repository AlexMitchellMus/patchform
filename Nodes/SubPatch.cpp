// SubpatchNode.cpp

#include "Subpatch.h"
#include "../Graph/GraphHolder.h"
#include "../Graph/GraphManager.h"

Subpatch::Subpatch(std::shared_ptr<NodeContext> context, const json& creationData)
    : AudioNode(context, AudioPort::None, creationData)
{
    nodeCreationData = creationData;
}

void Subpatch::postCreate()
{
    subManager = std::make_shared<GraphManager>(graphManagerParent);
    subManager->owningSubpatch = this;

    static const json emptySubpatch = {{"nodes", json::array()}, {"connections", json::array()}};

    setupSubgraph(nodeCreationData.value("subpatch", emptySubpatch));
}

void Subpatch::setupSubgraph(const json& subpatchJson)
{
    auto [objects, edges] = subManager->loadGraph("subpatch", subpatchJson, false, [this](std::shared_ptr<GraphHolder>& g)
        {
            rebuildPortsFromGraph(*g);
        }
    );
}

void Subpatch::rebuildPortsFromGraph(GraphHolder& graph)
{
    inputPortBuffers.clear();
    outputPortBuffers.clear();
    graph.subInputs.clear();
    graph.subOutputs.clear();

    SDL_Delay(1000);

    int inputCounter = 0;
    int outputCounter = 0;

    for (auto* node : graph.getObjects())
    {
        if (auto* inlet = dynamic_cast<Inlet*>(node))
        {
            graph.subInputs.push_back(inlet->getOutputPort(0));
            addInputPort("in_" + std::to_string(inputCounter++), AudioPort::PortType::Signal);
        }
        else if (auto* outlet = dynamic_cast<Outlet*>(node))
        {
            graph.subOutputs.push_back(outlet->getInputPort(0));
            addOutputPort("out_" + std::to_string(outputCounter++), AudioPort::PortType::Signal);
        }
    }
}

void Subpatch::process(const float*, float*, std::vector<MidiMessage>& midi, unsigned long frames, Graph& g, int index)
{
    if (!subManager)
        return;

    subManager->swapGraphState();

    auto* subGraph = subManager->getActiveGraph();
    auto& subInputs = subGraph->subInputs;
    auto& subOutputs = subGraph->subOutputs;

    // Pull input from parent via graph's port map
    const auto& upstream = g.outputInputPortMap[index];
    for (size_t i = 0; i < subInputs.size() && i < upstream.size(); ++i)
    {
        float* dst = subInputs[i]->getAudioBuffer();
        const auto& group = upstream[i];

        for (auto* src : group.connectedPorts)
        {
            const float* srcBuf = src->getAudioBuffer();
            for (unsigned s = 0; s < frames; ++s)
                dst[s] += srcBuf[s];
        }
    }

    // Process subgraph
    subManager->process(nullptr, nullptr, frames, midi);

    // Push output to parent using downstream port map
    const auto& downstream = g.downstreamPortMap[index];
    for (const auto& group : downstream)
    {
        uint8_t portIndex = group.outputPortNumber;
        if (portIndex >= subOutputs.size())
            continue;

        const float* src = subOutputs[portIndex]->getAudioBuffer();

        for (const auto& conn : group.downstreamConnections)
        {
            float* dst = conn.dst;
            size_t n = conn.bufferSize;

            for (size_t s = 0; s < n; ++s)
                dst[s] += src[s];
        }
    }

    // Clear subgraph ports
    for (auto* port : subInputs)
    {
        if (port->isSignal()) port->clear(frames);
        port->clearEvents();
    }

    for (auto* port : subOutputs)
    {
        if (port->isSignal()) port->clear(frames);
        port->clearEvents();
    }
}


json Subpatch::getSerializedNode()
{
    json j = nodeCreationData;
    if (subManager)
        j["subpatch"] = subManager->graphToJSON();
    return j;
}
