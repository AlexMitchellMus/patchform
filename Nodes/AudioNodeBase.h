/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <vector>
#include "AudioPort.h"
#include "../Graph/NodeContext.h"
#include "NodeRegistry.h"

#include "../Graph/Logger.h"

#define DEFINE_AND_REGISTER_NODE(nodeName)                    \
public:                                                       \
    static inline const std::string name = nodeName;          \
    static inline const bool registered = []() {              \
        NodeRegistry::getInstance().registerNode(name);       \
        return true;                                          \
    }();                                                      \
    std::string getName() { return name; }                    \

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Abstract AudioNode class
class AudioNode {
public:
    struct StateBase
    {
        virtual ~StateBase() = default;
        virtual std::unique_ptr<StateBase> clone() const = 0;
    };
    // Default NullState for nodes without state
    struct NullState : public StateBase {
        std::unique_ptr<StateBase> clone() const override {
            return std::make_unique<NullState>();
        }
    };

protected:
    std::vector<AudioInputPort> inputPorts;
    AudioPort outputPort;
    NodeContext* context;

    std::unique_ptr<StateBase> stateA; // Active state
    std::unique_ptr<StateBase> stateB; // Inactive state
    StateBase* activeState;                  // Pointer to the inactive state
    bool dirty = false;                // Marks if a state swap is needed

public:
    AudioNode(std::unique_ptr<StateBase> initialState, NodeContext* context, AudioPort::PortType type)
        : stateA(std::move(initialState))
        , stateB(stateA->clone())
        , activeState(stateB.get())
        , context(context)
        , outputPort(this, "output", type)
    {}

    virtual ~AudioNode()
    {
    }

    // Defined by the macro for each derived class
    virtual std::string getName() = 0;

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

    enum class State { Unvisited, Visiting, Visited };
    State state = State::Unvisited;

    // Add an input port (for dependency)
    void addInputPort(std::string portName)
    {
        inputPorts.emplace_back(portName);
    }

    void linkInputPort(AudioPort* portToLink, int inputPortIndex)
    {
        inputPorts[inputPortIndex].connectedPorts.push_back(portToLink);
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) = 0;

    // Method to get input ports for sorting
    AudioPort* getOutputPort()
    {
        return &outputPort;
    }

    std::vector<AudioInputPort>& getInputPorts() { return inputPorts; };

protected:
    void process(float* buffer, unsigned long frameCount)
    {
        //swapStatesIfDirty();
        outputPort.clear(frameCount);
        processAudio(buffer, frameCount);
    }

    friend class AudioGraph;
};