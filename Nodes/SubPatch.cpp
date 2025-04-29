// SubpatchNode.cpp

#include "Subpatch.h"
#include "../Graph/GraphHolder.h"
#include "../Graph/GraphManager.h"

Subpatch::Subpatch(std::shared_ptr<NodeContext> context, const json& creationData)
    : AudioNode(context, AudioPort::None, creationData)
{
    subManager = std::make_shared<GraphManager>(context->sampleRate, context->frameCount);

    static const json emptySubpatch = {{"nodes", json::array()}, {"connections", json::array()}};

    setupSubgraph(creationData.value("subpatch", emptySubpatch));
}

void Subpatch::setupSubgraph(const json& subpatchJson)
{
    auto [objects, edges] = subManager->loadGraph("subpatch", subpatchJson, false, [this](std::shared_ptr<GraphHolder>& g)
        {
            rebuildPortsFromGraph(*g);
        }
    );
}

void Subpatch::rebuildPortsFromGraph(const GraphHolder& graph) {
    inputPortBuffers.clear();
    outputPortBuffers.clear();
    subInputs.clear();
    subOutputs.clear();

    int inputCounter = 0;
    int outputCounter = 0;

    for (auto* node : graph.getObjects())
    {
        if (auto* inlet = dynamic_cast<Inlet*>(node))
        {
            subInputs.push_back(inlet->getOutputPort(0));
            addInputPort("in_" + std::to_string(inputCounter++), AudioPort::PortType::Signal);
        }
        else if (auto* outlet = dynamic_cast<Outlet*>(node))
        {
            subOutputs.push_back(outlet->getInputPort(0));
            addOutputPort("out_" + std::to_string(outputCounter++), AudioPort::PortType::Signal);
        }
    }
}

void Subpatch::process(const float* inBuffer, float* outBuffer, std::vector<MidiMessage>& midi, unsigned long frames, Graph& g, int index)
{
    if (!subManager)
    {
        return;
    }

    // Push input buffers
    for (size_t i = 0; i < subInputs.size(); ++i) {
        if (i < inputPortBuffers.size()) {
            std::memcpy(subInputs[i]->getAudioBuffer(), inputPortBuffers[i]->getAudioBuffer(), frames * sizeof(float));
        }
    }

    // Process internal graph
    subManager->process(inBuffer, outBuffer, frames, midi);

    // Pull output buffers
    for (size_t i = 0; i < subOutputs.size(); ++i) {
        if (i < outputPortBuffers.size()) {
            std::memcpy(outputPortBuffers[i]->getAudioBuffer(), subOutputs[i]->getAudioBuffer(), frames * sizeof(float));
        }
    }

    pushOutputAudio(g, index);

//#define DEBUG_SUB_PORTS
#ifdef DEBUG_SUB_PORTS
    if (subInputs.size() > 0) {
        std::cout << "Inlet output sample: " << subInputs[0]->getAudioBuffer()[0] << std::endl;
    }
    if (subOutputs.size() > 0) {
        std::cout << "Outlet input sample: " << subOutputs[0]->getAudioBuffer()[0] << std::endl;
    }
#endif

    for (auto& port : outputPortBuffers)
        port->clearEvents();

    for (auto& port : inputPortBuffers)
    {
        if (port->isSignal())
            port->clear(frames);
        port->clearEvents();
    }

    // ALSO clear subOutputs (Outlet inputPorts we pulled from)
    for (auto* port : subOutputs)
    {
        if (port && port->isSignal())
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
