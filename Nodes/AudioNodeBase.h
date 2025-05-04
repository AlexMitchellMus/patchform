/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include <functional>

#include "AudioPort.h"
#include "../Graph/NodeContext.h"
#include "glaze/glaze.hpp"
#include "Parameter.h"
#include "NodeRegistry.h"
#include "JsonHelpers.h"

#ifdef PATCHFORM_WITH_GUI
#include "../UI/Object.h"
#endif

#include "json.hpp"
using json = nlohmann::json;

// Helper macro to name and register node (used in derived node class)
// <Fullname> <ShortName> <should constantly process>
#define DEFINE_AND_REGISTER_NODE(nodeName, shortNodeName, processAudio)      \
public:                                                                      \
    static inline const std::string name = nodeName;                         \
    static inline const std::string shortName = shortNodeName;               \
    const std::string& getName() const override { return name; }             \
    const std::string& getShortName() const override { return shortName; }   \
    static inline const bool isAudioProcessor = processAudio;                \
    const bool& alwaysProcess() const override { return isAudioProcessor; }  \
    private:                                                                 \

#define DEFINE_NODE_ALIASES(...)                                                            \
public:                                                                                     \
    static inline const std::vector<std::string> aliases = { __VA_ARGS__ };                 \

#define REGISTER(className)                                                                                                 \
    static inline const bool _##className##_registered = [] {                                                               \
        NodeRegistry::getInstance().registerNode(className::name);                                                          \
        NodeRegistry::getInstance().registerAlias(className::aliases, [](std::shared_ptr<NodeContext> ctx, const json& j) { \
            return new className(std::move(ctx), j);                                                                        \
        });                                                                                                                 \
        return true;                                                                                                        \
    }();

#define REGISTER_PLUGIN(className)																			                \
    static AudioNode* create_##className(std::shared_ptr<NodeContext> ctx, const json& j) {									\
        return new className(std::move(ctx), j);																		    \
    }																										                \
    extern "C" __declspec(dllexport) void registerPatchformNodes() {										                \
        NodeRegistry::getInstance().registerNode(className::name);											                \
        NodeRegistry::getInstance().registerAlias(className::aliases, create_##className);					                \
    }										                                                                                \

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Graph;

struct MidiMessage {
    std::vector<unsigned char> message;
    double timestamp;
};

// Abstract AudioNode class
class AudioNode {
public:

    pptk::Point canvasPos;

#ifdef PATCHFORM_WITH_GUI
    class UI : public Object
    {
    public:
        explicit UI(AudioNode* node) : Object(node) {  };
    };
#endif

    std::vector<std::unique_ptr<AudioPort>> inputPortBuffers;
    std::vector<std::unique_ptr<AudioPort>> outputPortBuffers;

    std::shared_ptr<NodeContext> context;

    json nodeCreationData;

    virtual json getSerializedNode() { return nodeCreationData; };

    AudioNode(std::shared_ptr<NodeContext> context, AudioPort::PortType type, const json& creationData)
        : context(std::move(context))
        , nodeCreationData(std::move(creationData))
    {
        // Reset port visibility to fully visible for all ports
        // This is used for subpatch objects so we can limit which ports are visible
        // We need to do this as subpatch can dynamically change it's IO ports
        // So we only ever grow the vector of ports, and reuse them
        // Which means we need to limit the visibility of them to the UI
        // Internally they stick around, but don't get processed
        visibleInputBits.set();
        visibleOutputBits.set();

        // Needs to be done after member initialization!
        // This happens outside the audio thread
        // TODO: We need to allow ports to define larger than the frameCount - for wavetable / large audio buffer ports
        if (type != AudioPort::None)
        {
            addOutputPort("main output", type);
        }
    }

    virtual ~AudioNode()
    {
        //std::cout << "destorying audio node: " << nodeID << std::endl;
    }

    virtual void postCreate() { };

    virtual bool isGuiOnly() const
    {
        return false;
    }

#ifdef PATCHFORM_WITH_GUI
protected:
    virtual std::unique_ptr<UI> makeUI()
    {
        return std::make_unique<UI>(this);
    };
public:

    virtual bool isDefaultUI() const { return true; };

    // Returns the current UI, otherwise nullptr
    UI* getUI()
    {
        return ui.get();
    }

    UI* getOrCreateUI()
    {
        // The default factory method creates a DefaultUI instance.
        if (!ui)
            ui = makeUI();

        return ui.get();
    }

    void destroyUI()
    {
        ui.reset();
    }
#endif

    // Glaze read json as std::string (not connected ATM)
    //template <typename T>
    //constexpr T& parseObjectParams(T& paramData)
    //{
    //    auto result = glz::read<glz::opts{.error_on_unknown_keys = false}>(paramData, nodeCreationData.dump());
    //    if (result) {
    //        std::cerr << "Failed to parse node data: " << format_error(result.ec) << std::endl;
    //    }
    //}

    // Defined by the macro for each derived class
    [[nodiscard]] virtual const std::string& getName() const = 0;
    [[nodiscard]] virtual const std::string& getShortName() const = 0;

    [[nodiscard]] virtual const bool& alwaysProcess() const = 0;

    virtual bool shouldProcess(unsigned int frameCount)
    {
        return true;
    }

    int getNumOutputs() const { return outputPortBuffers.size(); };

    int getNumInputs() const { return inputPortBuffers.size(); };

    // Add an input port (for dependency)
    void addInputPort(std::string portName, AudioPort::PortType portType)
    {
        if (getNumInputs() >= maxPortNumber)
        {
            std::cerr << "Error: Exceeded max input ports (255)." << std::endl;
            return;
        }

        inputPortBuffers.push_back(make_unique<AudioPort>(this, portName, portType, true));
    }

    void addOutputPort(std::string portName, AudioPort::PortType portType)
    {
        if (getNumOutputs() >= maxPortNumber)
        {
            std::cerr << "Error: Exceeded max output ports (255)." << std::endl;
            return;
        }

        outputPortBuffers.push_back(make_unique<AudioPort>(this, portName, portType));
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(const float* inBuffer, float* buffer, unsigned long frameCount, std::vector<MidiMessage>&) { };

    // Method to get input ports for sorting
    AudioPort* getOutputPort(const int index = 0) const
    {
        if (outputPortBuffers.size() > index)
            return outputPortBuffers[index].get();

        return nullptr;
    }

    AudioPort* getInputPort(const int index = 0) const
    {
        if (inputPortBuffers.size() > index)
            return inputPortBuffers[index].get();

        return nullptr;
    }

    // Push an event to input buffer
    void pushEvent(int inPort, Event* event)
    {
        inputPortBuffers[inPort]->addEvent(event);
    }

    // Push an event to the output buffer
    void addEvent(int port, Event* event)
    {
        outputPortBuffers[port]->addEvent(event);
        hasEvents = true;
    }

    std::function<void(const std::vector<std::unique_ptr<AudioPort>>&, Graph&, const int, AudioNode* _this)> pushOutputEvents;
    std::function<void(const std::vector<AudioPort*>&, Graph&, int)> pushOutputEventsFromPointers;

    std::function<void(Graph& graph, int index)> pushOutputAudio;

    // Sets the bit field mask for this node in the context, where the current running graph will
    // then process this in the next skip
    std::function<void()> setNodeDirty = [](){};

    // Process once on load
    bool eventOnLoad = false;

    uint32_t nodeID;
    std::string nodeIDString;

    template <typename T, typename... Args>
    T* addParameter(Args&&... args) {
        auto param = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = param.get();
        parameters.push_back(std::move(param));
        return ptr;
    }

    std::vector<std::unique_ptr<Parameter>>& getParameters() { return parameters; };

    virtual void cleanupAudio(){};

    virtual bool canFeedback()
    {
        return false;
    }

    void setGraphManagerParent(GraphManager* gm)
    {
        graphManagerParent = gm;
    }

    GraphManager* getGraphManagerParent() { return graphManagerParent; };

    // Used to limit visibility of real ports in graph system
    // We don't (ATM) delete ports, we simply add to them so the pointers stay alive
    const std::bitset<256>& inputPortVisibility() const { return visibleInputBits; }
    const std::bitset<256>& outputPortVisibility() const { return visibleOutputBits; }

private:
    bool isClean = false;

    void runCleanup()
    {
        if (isClean)
            return;

        cleanupAudio();
    }

    bool hasEvents = false;

    virtual void process(const float* inBuffer, float* buffer, std::vector<MidiMessage>& midiMessage, unsigned long frameCount, Graph& g, const size_t index)
    {
        if (!shouldProcess(frameCount))
            return;

        processAudio(inBuffer, buffer, frameCount, midiMessage);

        pushOutputAudio(g, index);

        //if (hasEvents)
        //{
            pushOutputEvents(outputPortBuffers, g, index, this);
        //    hasEvents = false;
        //}

        for (auto& port : outputPortBuffers)
            port->clearEvents();

        for (auto& port : inputPortBuffers)
        {
            if (port->isSignal())
                port->clear(frameCount);
            port->clearEvents();
        }
    }
#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<UI> ui = nullptr;
#endif
    friend class Graph;

protected:
    std::bitset<256> visibleInputBits;
    std::bitset<256> visibleOutputBits;

    GraphManager* graphManagerParent = nullptr;

    std::vector<std::unique_ptr<Parameter>> parameters;
};