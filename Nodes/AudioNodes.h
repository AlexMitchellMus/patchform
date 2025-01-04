#include <vector>
#include "../AudioPort.h"

#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class EventPool {
public:
    EventPool(std::size_t initialSize = 1024) {
        // Pre-allocate the event vector
        events.reserve(initialSize);
        // Create that many events initially
        growPool(initialSize);
    }

    // Acquire a free Event (returns nullptr if none available)
    Event* getFreeEvent() {
        //std::cout << "freestack size: " << freeStack.size() << std::endl;
        if (freeStack.empty()) {
            std::cout << "event exhaustion, size: " << freeStack.size() << std::endl;
            // No free events left!
            // Do NOT grow here if you're in the audio callback
            // Return nullptr or handle logic as you wish
            return nullptr;
        }
        // Pop index from top of stack
        std::size_t eventIndex = freeStack.back();
        freeStack.pop_back();

        Event* evt = &events[eventIndex];
        evt->setTimeStamp(0);
        return evt;
    }

    // Return an Event to the pool
    void returnFreeEvent(Event* evt) {
        if (!evt) return;

        // Calculate index
        std::size_t index = static_cast<std::size_t>(evt - &events[0]);
        freeStack.push_back(index);
    }

    // Grow the pool by adding N new events (call this OUTSIDE audio callback)
    void growPool(std::size_t count) {
        // Current size
        std::size_t oldSize = events.size();

        // Increase by 'count'
        events.resize(oldSize + count);

        // Push new indices onto freeStack
        for (std::size_t i = 0; i < count; ++i) {
            freeStack.push_back(oldSize + i);
        }
    }

private:
    std::vector<Event> events;       // Actual storage
    std::vector<std::size_t> freeStack;  // Indices of free events
};

// NodeContext to store common properties for nodes like sample rate
class NodeContext {
public:
    float sampleRate;  // Sample rate for the node
    int frameCount;

    EventPool eventPool;

    NodeContext(float sampleRate, int frameCount) : sampleRate(sampleRate), frameCount(frameCount) {}
};

// Abstract AudioNode class
class AudioNode {
protected:
    std::vector<AudioInputPort> inputPorts;
    AudioPort outputPort;
    NodeContext* context;
    std::string name;

public:
    AudioNode(NodeContext* context, std::string nodeName)
        : context(context)
        , name(nodeName)
        , outputPort(this, "output")
    {}

    virtual ~AudioNode()
    {
    }

    std::string getName() { return name; }

    // Add an input port (for dependency)
    void addInputPort(std::string portName)
    {
        inputPorts.emplace_back(portName);
    }

    void linkInputPort(AudioPort* portToLink, int inputPortIndex)
    {
        inputPorts[inputPortIndex].connectedPorts.push_back(portToLink);
    }

    void process(float* buffer, unsigned long frameCount)
    {
        outputPort.clear(frameCount);
        processAudio(buffer, frameCount);
    }

    // Virtual method for processing the audio buffer
    virtual void processAudio(float* buffer, unsigned long frameCount) = 0;

    // Method to get input ports for sorting
    AudioPort* getOutputPort()
    {
        return &outputPort;
    }

    std::vector<AudioInputPort>& getInputPorts() { return inputPorts; };

};