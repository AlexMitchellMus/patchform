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
#include "NodeRegistry.h"

#include "../Graph/Logger.h"

#include "glaze/glaze.hpp"

#include "Parameter.h"

#ifdef PATCHFORM_WITH_GUI
#include "../UI/Object.h"
#endif

#include "json.hpp"
using json = nlohmann::json;

// Helper macro to name and register node (used in derived node class)
#define DEFINE_AND_REGISTER_NODE(nodeName, shortNodeName)                       \
public:                                                                         \
    static inline const std::string name = nodeName;                            \
    static inline const std::string shortName = shortNodeName;                  \
    static inline const bool registered = []() {                                \
        NodeRegistry::getInstance().registerNode(name);                         \
        return true;                                                            \
    }();                                                                        \
    const std::string& getName() const override { return name; }                \
    const std::string& getShortName() const override { return shortName; }      \
    private:                                                                    \

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class AudioGraph;

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
    NodeContext* context;

    json nodeCreationData;

    virtual json getSerializedNode() { return nodeCreationData; };

    AudioNode(NodeContext* context, AudioPort::PortType type, const json& creationData)
        : context(context)
        //, outputPort(this, "output", type)
        , nodeCreationData(std::move(creationData))
    {
        // Needs to be done after member initialization!
        // This happens outside the audio thread
        // TODO: We need to allow ports to define larger than the frameCount - for wavetable / large audio buffer ports
        if (type != AudioPort::None)
        {
            addOutputPort("main output", type);
            //outputPort.setSize(context->frameCount);
        }
    }

    virtual ~AudioNode()
    {
        //std::cout << "destorying audio node: " << nodeID << std::endl;
    }

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

    virtual bool shouldProcess(unsigned int frameCount)
    {
        return true;
    }

    int getNumOutputs() { return 1; };

    int getNumInputs() { return inputPortBuffers.size(); };

    // Add an input port (for dependency)
    void addInputPort(std::string portName, AudioPort::PortType portType)
    {
        inputPortBuffers.push_back(make_unique<AudioPort>(this, portName, portType));
    }

    void addOutputPort(std::string portName, AudioPort::PortType portType)
    {
        outputPortBuffers.push_back(make_unique<AudioPort>(this, portName, portType));

        if (portType == AudioPort::PortType::Signal)
            outputPortBuffers.back()->setSize(context->frameCount);
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) { };

    // Method to get input ports for sorting
    AudioPort* getOutputPort(const int index = 0) const
    {
        if (outputPortBuffers.size())
            return outputPortBuffers[index].get();

        return nullptr;
    }

    AudioPort* getInputPort(const int index = 0) const
    {
        if (inputPortBuffers.size())
            return inputPortBuffers[index].get();

        return nullptr;
    }

    // Push an event to input buffer
    void pushEvent(int inPort, Event* event)
    {
        hasInputEvents = true;
        inputPortBuffers[inPort]->addEvent(event);
    }

    // Push an event to the output buffer
    void addEvent(int port, Event* event)
    {
        hasOutputEvents = true;
        outputPortBuffers[port]->addEvent(event);
    }

    std::function<void(const std::vector<std::unique_ptr<AudioPort>>&, const AudioGraph&, const int)> pushOutputEvents;

    std::function<void(const std::vector<std::unique_ptr<AudioPort>>&, const AudioGraph&, const int)> sumInputBuffers;

    uint32_t nodeID;
    std::string nodeIDString;

    template<typename T, typename... Args>
    T* addParameter(const std::string& name, Args&&... args) {
        static_assert(std::is_base_of_v<Parameter, T>, "T must be a subclass of Parameter");
        auto param = std::make_unique<T>(name, std::forward<Args>(args)...);
        T* ptr = param.get();
        parameters.push_back(std::move(param));
        return ptr;
    }

    std::vector<std::unique_ptr<Parameter>>& getParameters() { return parameters; };

    std::atomic<bool> hasInputEvents = false;
    bool hasOutputEvents = false;

private:

    void process(float* buffer, unsigned long frameCount, const AudioGraph& runningGraph, const int index)
    {
        //if (!shouldProcess(frameCount))
        //{
        //    return;
        //}

        sumInputBuffers(inputPortBuffers, runningGraph, index);

        processAudio(buffer, frameCount);

        pushOutputEvents(outputPortBuffers, runningGraph, index);

        for (int i = 0; i < outputPortBuffers.size(); ++i)
            getOutputPort(i)->clearEvents();

        for (int i = 0; i < inputPortBuffers.size(); ++i)
            getInputPort(i)->clearEvents();

        hasInputEvents.store(false);
        hasOutputEvents = false;
    }
#ifdef PATCHFORM_WITH_GUI
    std::unique_ptr<UI> ui = nullptr;
#endif
    friend class AudioGraph;

protected:
    std::vector<std::unique_ptr<Parameter>> parameters;
};