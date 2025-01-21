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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class AudioGraph;

// Abstract AudioNode class
class AudioNode {
public:
    std::vector<std::unique_ptr<AudioPort>> inputPortBuffers;

    AudioPort outputPort;
    NodeContext* context;

    json nodeCreationData;

    AudioNode(NodeContext* context, AudioPort::PortType type, const json& creationData)
        : context(context)
        , outputPort(this, "output", type)
        , nodeCreationData(std::move(creationData))
    {
    }

    virtual ~AudioNode()
    {
        //std::cout << "destorying audio node: " << nodeID << std::endl;
    }

    // Glaze read json as std::string (not connected ATM)
    template <typename T>
    constexpr T& parseObjectParams(T& paramData)
    {
        auto result = glz::read<glz::opts{.error_on_unknown_keys = false}>(paramData, nodeCreationData.dump());
        if (result) {
            std::cerr << "Failed to parse node data: " << format_error(result.ec) << std::endl;
        }
    }

    // Defined by the macro for each derived class
    virtual const std::string& getName() const = 0;
    virtual const std::string& getShortName() const = 0;

    int getNumOutputs() { return 1; };

    int getNumInputs() { return inputPortBuffers.size(); };

    // Add an input port (for dependency)
    void addInputPort(std::string portName, AudioPort::PortType portType)
    {
        inputPortBuffers.push_back(make_unique<AudioPort>(this, portName, portType));
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) = 0;

    // Method to get input ports for sorting
    AudioPort* getOutputPort()
    {
        return &outputPort;
    }

    std::function<void(const std::vector<std::unique_ptr<AudioPort>>&, const AudioGraph&, const int)> sumInputBuffers;

    uint32_t nodeID;

private:
    void process(float* buffer, unsigned long frameCount, const AudioGraph& runningGraph, const int index)
    {
        sumInputBuffers(inputPortBuffers, runningGraph, index);
        outputPort.clear(frameCount);
        processAudio(buffer, frameCount);
    }

    friend class AudioGraph;
};