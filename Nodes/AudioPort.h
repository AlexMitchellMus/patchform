#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include "AudioNodeBase.h"
#include "../Graph/Event.h"
#include "AlignedAllocator.h"

class AudioPort;
class AudioNode;

constexpr int defaultTableSize = 2048;
constexpr int maxPortNumber = 255;

struct PortGroup {
    uint8_t inputPortNumber;
    std::vector<AudioPort*> connectedPorts;
};

struct DownstreamPortGroup {
    uint8_t outputPortNumber;

    struct DownstreamConnection {
        const float* src = nullptr;
        float* dst = nullptr;
        size_t bufferSize = 0;
        AudioNode* node;
        AudioPort* inputPort;
        int inputPortIndex;
        uint32_t targetIndex = 0;
        uint32_t writeCount = 0;
        uint32_t maxWriteCount = 0;
    };

    std::vector<DownstreamConnection> downstreamConnections;
};

using OutputPortMap = std::vector<std::vector<PortGroup>>;
using DownStreamPortMap = std::vector<std::vector<DownstreamPortGroup>>;

class AudioPort {
public:
    enum PortType : uint8_t {
        None      = 0,
        Signal    = 1 << 0,
        Spectral  = 1 << 1,
        Wavetable = 1 << 2,
        Samples   = 1 << 3,
        Data      = 1 << 4
    };

    bool isInput = false;

    AudioPort() = default;

    AudioPort(AudioNode* parent, const std::string& portName, PortType type, bool isInput = false)
        : node(parent)
        , name(std::move(portName))
        , portType(type)
        , isInput(isInput)
    {
        events.reserve(1024);

        if (type == PortType::Signal)
            setSize(bufferSize = 64);
        else if (type == PortType::Spectral)
            setSize(bufferSize = 256);
        else if (type == PortType::Wavetable)
            setSize(bufferSize = defaultTableSize);
    }

    AudioPort(const AudioPort&) = delete;
    AudioPort& operator=(const AudioPort&) = delete;
    AudioPort(AudioPort&&) noexcept = default;
    AudioPort& operator=(AudioPort&&) noexcept = default;

    float* getAudioBuffer() { return bufferPtr; }

    size_t getAudioBufferSize() const { return bufferSize; }

    bool isAnyConnectedPortSignal = false;

    std::vector<Event*>& getEvents() { return events; }

    void clearEvents() { events.clear(); }

    void addEvent(Event* event) { events.push_back(event); }

    void clear(size_t size) {
        if (usingInline)
            std::fill_n(inlineBuffer, size, 0.0f);
        else
            audioBuffer.assign(size, 0.0f);
    }

    void changePortType(PortType t)
    {
        portType = t;
    }

    void zero() { clear(bufferSize); }

    void setSize(size_t size) {
        bufferSize = static_cast<unsigned>(size);

        if (size <= InlineBufferSize) {
            bufferPtr = inlineBuffer;
            usingInline = true;
        } else {
            audioBuffer.resize(size);
            bufferPtr = audioBuffer.data();
            usingInline = false;
        }

        clear(size);
    }

    inline bool isSignal() const {
        return (portType & (Signal | Spectral | Wavetable)) != 0;
    }

    inline bool isWavetable() const { return (portType & Wavetable) != 0; }

    inline bool isSampleBuffer() const { return (portType & Samples) != 0; }

    AudioNode* getParentNode() const { return node; }

    bool operator==(const AudioPort& other) const {
        return this == &other;
    }

    PortType getPortType() const { return portType; }

    Sample sampleBuffer;

protected:
    static constexpr size_t InlineBufferSize = 128;
    alignas(32) float inlineBuffer[InlineBufferSize]{};
    float* bufferPtr = inlineBuffer;
    bool usingInline = true;

    std::vector<float, AlignedAllocator<float, 32>> audioBuffer;

    AudioNode* node;
    unsigned bufferSize = 0;
    std::vector<Event*> events;
    std::string name;
    PortType portType;
};
