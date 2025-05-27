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
    std::cout << "rebuilding inlet outlet map for subpatch" << std::endl;

    visibleInputBits.reset();
    visibleOutputBits.reset();

    graph.subInputs.clear();
    graph.subOutputs.clear();
    graph.subInputOuterPorts.clear();
    graph.subInputSortedIndices.clear();
    graph.subInputNodes.clear();

    int inputIndex = 0;
    int outputIndex = 0;

    for (size_t sortedIndex = 0; sortedIndex < graph.graph->objectsSorted.size(); ++sortedIndex)
    {
        auto* node = graph.graph->objectsSorted[sortedIndex];
        if (!node) continue;

        if (auto* inlet = dynamic_cast<Inlet*>(node))
        {
            graph.subInputs.push_back(inlet->getOutputPort(0));
            auto* outer = getInputPortSafe(inputIndex, AudioPort::Signal);
            graph.subInputOuterPorts.push_back(outer);
            graph.subInputSortedIndices.push_back(static_cast<int>(sortedIndex));
            graph.subInputNodes.push_back(inlet);
            visibleInputBits.set(inputIndex++);
        }
        else if (auto* dataInlet = dynamic_cast<DataInlet*>(node))
        {
            graph.subInputs.push_back(dataInlet->getOutputPort(0));
            auto* outer = getInputPortSafe(inputIndex, AudioPort::Data);
            graph.subInputOuterPorts.push_back(outer);
            graph.subInputSortedIndices.push_back(static_cast<int>(sortedIndex));
            graph.subInputNodes.push_back(dataInlet);
            visibleInputBits.set(inputIndex++);
        }
        else if (auto* outlet = dynamic_cast<Outlet*>(node))
        {
            graph.subOutputs.push_back(outlet->getInputPort(0));
            ensureOutputPort(outputIndex, AudioPort::Signal);
            graph.subOutputOuterPorts.push_back(getOutputPort(outputIndex));
            visibleOutputBits.set(outputIndex++);
        }
        else if (auto* dataOutlet = dynamic_cast<DataOutlet*>(node))
        {
            graph.subOutputs.push_back(dataOutlet->getInputPort(0));
            ensureOutputPort(outputIndex, AudioPort::Data);
            graph.subOutputOuterPorts.push_back(getOutputPort(outputIndex));
            visibleOutputBits.set(outputIndex++);
        }
    }

//#define DEBUG_SUBPATCH_PORT_SORT
#ifdef DEBUG_SUBPATCH_PORT_SORT
    std::cout << "SubInputOuterPorts:" << std::endl;
    for (size_t i = 0; i < graph.subInputOuterPorts.size(); ++i)
    {
        auto* port = graph.subInputOuterPorts[i];
        std::cout << "  [" << i << "]: " << (port->isSignal() ? "Signal" : "Data")
                  << " port, owner = " << (port->getParentNode() ? port->getParentNode()->getShortName() : "null")
                  << ", sortedIndex = " << graph.subInputSortedIndices[i] << std::endl;
    }
#endif
}

AudioPort* Subpatch::getInputPortSafe(int index, AudioPort::PortType type)
{
    if (index >= getNumInputs())
        addInputPort("in_" + std::to_string(index), AudioPort::Signal);

    auto* port = getInputPort(index);
    port->changePortType(type);
    return port;
}

void Subpatch::ensureOutputPort(int index, AudioPort::PortType type)
{
    if (index >= getNumOutputs())
        addOutputPort("out_" + std::to_string(index), AudioPort::Signal);

    getOutputPort(index)->changePortType(type);
}

void Subpatch::process(const float* mainAudioIn, float* mainAudioOut, std::vector<MidiMessage>& midi, unsigned long frames, Graph& g, const size_t index)
{
    if (!subManager)
        return;

    subManager->swapGraphState();

    auto* subGraph = subManager->getActiveGraph();
    auto& subInputs = subGraph->subInputs;
    auto& subOutputs = subGraph->subOutputs;

    // ===========================
    // Pull input from parent via graph's port map
    // ===========================

    const auto& upstream = g.outputInputPortMap[index];
    for (size_t i = 0; i < subInputs.size() && i < upstream.size(); ++i)
    {
        float* dst = subInputs[i]->getAudioBuffer();
        const auto& group = upstream[i];

        for (auto* src : group.connectedPorts)
        {
            if (!src->isSignal())
                continue;

            const float* srcBuf = src->getAudioBuffer();
            for (unsigned s = 0; s < frames; ++s)
                dst[s] += srcBuf[s];
        }
    }

    for (int i = 0; i < subInputs.size(); ++i)
    {
        auto* outerBuf = subGraph->subInputOuterPorts[i];
        if (!outerBuf) continue;

        auto& events = outerBuf->getEvents();

        for (const auto& ev : events)
        {
            subInputs[i]->addEvent(ev);
            const int sortedIndex = subGraph->subInputSortedIndices[i];
            subGraph->getGraph()->bitfields.setEventBit(sortedIndex);
        }

        outerBuf->clearEvents();
    }

    // ===========================
    // Process subgraph
    // ===========================

    subManager->process(mainAudioIn, mainAudioOut, frames, midi);

    // ===========================
    // Push output to parent using downstream port map
    // ===========================

    const auto& downstream = g.downstreamPortMap[index];
    for (const auto& group : downstream)
    {
        uint8_t portIndex = group.outputPortNumber;
        if (portIndex >= subOutputs.size() || !subOutputs[portIndex]->isSignal())
            continue;

        const float* src = subOutputs[portIndex]->getAudioBuffer();

        for (const auto& conn : group.downstreamConnections)
        {
            float* dst = conn.dst;

            // We cache the size of the buffer, but I made buffers larger so they could work with different
            // sizes - which is needed for plugin mode
            // This WILL break FFT buffers. BUT we should make FFT buffers use the event audio data
            for (size_t s = 0; s < frames; ++s)
                dst[s] += src[s];
        }
    }

    for (size_t i = 0; i < subGraph->subOutputs.size(); ++i)
    {
        auto* innerPort = subGraph->subOutputs[i];

        //if (innerPort->isSignal())
        //    continue;

        auto* outerPort = subGraph->subOutputOuterPorts[i];
        for (auto& ev : innerPort->getEvents())
            outerPort->addEvent(ev);
    }

    pushOutputEventsFromPointers(subGraph->subOutputOuterPorts, g, index);

    for (auto* port : subGraph->subOutputOuterPorts) {
        if (!port) continue;
        if (!port->isSignal())
            port->clearEvents(); // required to avoid crosstalk
    }

    // Clear subgraph ports
    for (auto* port : subInputs)
    {
        if (port->isSignal())
            port->clear(frames);
        port->clearEvents();
    }

    for (auto* port : subOutputs)
    {
        if (port->isSignal())
            port->clear(frames);
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
