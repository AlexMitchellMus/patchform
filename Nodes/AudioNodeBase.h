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

// Helper macro to populate state copy for state management (used in derived node's state class)
#define ENABLE_COPY(Derived)                                                    \
std::unique_ptr<StateBase> copy() const override {                              \
    return std::make_unique<Derived>(*this);                                    \
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class AudioGraph;

// Abstract AudioNode class
class AudioNode {
public:
    class StateBase {
    public:
        virtual ~StateBase() = default;

        virtual std::unique_ptr<StateBase> copy() const = 0;
    };

    class NullState : public StateBase {
    public:
        std::unique_ptr<StateBase> copy() const override {
            return std::make_unique<NullState>();
        }
    };

    std::unique_ptr<StateBase> stateA; // Active state
    std::unique_ptr<StateBase> stateB; // Inactive state
    StateBase* activeState;            // Pointer to the inactive state
    bool dirty = false;                // Marks if a state swap is needed

    // old input buffer system (fixed pointer)
    //std::vector<AudioInputPort> inputPorts;

    // new input buffer system (dynamic - lookup table)
    std::vector<std::unique_ptr<AudioPort>> inputPortBuffers;

    AudioPort outputPort;
    NodeContext* context;

    json nodeCreationData;

    AudioNode(std::unique_ptr<StateBase> initialState, NodeContext* context, AudioPort::PortType type, const json& creationData)
        : stateA(std::move(initialState))
        , stateB(stateA->copy())
        , activeState(stateB.get())
        , context(context)
        , outputPort(this, "output", type)
        , nodeCreationData(std::move(creationData))
    {
    }

    virtual ~AudioNode()
    {
        std::cout << "destorying audio node: " << nodeID << std::endl;
    }

    // Defined by the macro for each derived class
    virtual const std::string& getName() const = 0;
    virtual const std::string& getShortName() const = 0;

    int getNumOutputs() { return 1; };

    int getNumInputs() { return inputPortBuffers.size(); };

    // Mark the node as dirty to trigger a state swap
    void setDirty() { dirty = true; }

    // Swap the active and inactive states if dirty
    void swapStatesIfDirty() {
        if (dirty) {
            std::cout << "swapping state" << std::endl;
            stateA.swap(stateB);        // Swap active and inactive states
            activeState = stateB.get(); // Update the pointer to the inactive state
            dirty = false;
        }
    }

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

    int runCount = 0;

private:
    void process(float* buffer, unsigned long frameCount, const AudioGraph& runningGraph, const int index)
    {
        //swapStatesIfDirty();
        sumInputBuffers(inputPortBuffers, runningGraph, index);
        outputPort.clear(frameCount);
        processAudio(buffer, frameCount);
    }

    friend class AudioGraph;
};