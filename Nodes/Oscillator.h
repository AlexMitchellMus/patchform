/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>
#include <random>
#include <cstdlib>
#include <array>
#include "SignalsmithBlep.h"

// Use TABLE_SIZE as the number of intervals; we add one extra sample to close the cycle.
constexpr size_t TABLE_SIZE = 4092;
constexpr size_t FULL_TABLE_SIZE = TABLE_SIZE + 1;

constexpr float pi = 3.14159265358979323846f;
constexpr float twoPi = 6.28318530717958647692f;

// A constexpr sine approximation.
static constexpr float constexpr_sin(float x)
{
    // Normalize x to [-pi, pi]
    while (x > pi) x -= twoPi;
    while (x < -pi) x += twoPi;

    float x2 = x * x;
    float term1 = x; // x
    float term2 = (x * x2) / 6.0f; // x^3/3!
    float term3 = (x * x2 * x2) / 120.0f; // x^5/5!
    float term4 = (x * x2 * x2 * x2) / 5040.0f; // x^7/7!
    float term5 = (x * x2 * x2 * x2 * x2) / 362880.0f; // x^9/9!
    float term6 = (x * x2 * x2 * x2 * x2 * x2) / 39916800.0f; // x^11/11!

    return term1 - term2 + term3 - term4 + term5 - term6;
}

//-----------------------------------------------------------
// Waveform Table Generators (all produce FULL_TABLE_SIZE samples)
//-----------------------------------------------------------

static constexpr std::array<float, FULL_TABLE_SIZE> generateSineWave()
{
    std::array<float, FULL_TABLE_SIZE> table = {};
    // Use TABLE_SIZE as the denominator so that the extra sample is computed at 2*pi.
    for (size_t i = 0; i < FULL_TABLE_SIZE; ++i)
    {
        float angle = 2.0f * pi * static_cast<float>(i) / TABLE_SIZE;
        table[i] = constexpr_sin(angle);
    }
    return table;
}

static constexpr std::array<float, FULL_TABLE_SIZE> generateSawWave()
{
    std::array<float, FULL_TABLE_SIZE> table = {};
    // For TABLE_SIZE intervals, we generate TABLE_SIZE+1 samples.
    // The value at i=TABLE_SIZE is exactly 2*(TABLE_SIZE/TABLE_SIZE)-1 = 1.
    for (size_t i = 0; i < FULL_TABLE_SIZE; ++i)
    {
        float fraction = static_cast<float>(i) / TABLE_SIZE; // note: divide by TABLE_SIZE, not FULL_TABLE_SIZE
        table[i] = 2.0f * fraction - 1.0f;
    }
    return table;
}

static constexpr std::array<float, FULL_TABLE_SIZE> generateSquareWave()
{
    std::array<float, FULL_TABLE_SIZE> table = {};
    for (size_t i = 0; i < TABLE_SIZE; ++i)
    {
        table[i] = (i < (TABLE_SIZE / 2)) ? 1.0f : -1.0f;
    }
    // Close the cycle by making the extra sample equal to the first sample.
    table[TABLE_SIZE] = 1.0f;
    return table;
}

static constexpr std::array<float, FULL_TABLE_SIZE> generateTriangleWave()
{
    std::array<float, FULL_TABLE_SIZE> table = {};
    for (size_t i = 0; i < TABLE_SIZE; ++i)
    {
        float fraction = static_cast<float>(i) / TABLE_SIZE;
        if (fraction < 0.5f)
            table[i] = 4.0f * fraction - 1.0f;
        else
            table[i] = 3.0f - 4.0f * fraction;
    }
    // Close the cycle by matching the first sample.
    table[TABLE_SIZE] = -1.0f;
    return table;
}

// Precomputed tables.
static constexpr auto sineWaveTable = generateSineWave();
static constexpr auto sawWaveTable = generateSawWave();
static constexpr auto squareWaveTable = generateSquareWave();
static constexpr auto triangleWaveTable = generateTriangleWave();

//-----------------------------------------------------------
// Oscillator Class Using the Tables
//-----------------------------------------------------------

class Oscillator : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Oscillator", "osc", true);

protected:
    inline static bool initialized;

    // Phase (in table index units)
    float phase = 0.0f;
    std::atomic<hash32> waveformHash;
    float freq = 0.0f;

#include <cstdint>

    class XorShift {
    public:
        // Constructor: you can optionally provide a seed.
        explicit XorShift(uint32_t seed = 2463534242u) : state(seed) {}

        void setSeed(uint32_t seed) {
            // Mix the seed with a fixed constant to improve bit dispersion.
            state = seed ^ 0xA3C59AC3;  // XOR with a well-chosen constant.
            if (state == 0) {  // Avoid a zero state.
                state = 2463534242u;
            }
        }

        // Generate the next float in the range [-1.0f, 1.0f].
        inline float nextFloat() {
            // Xorshift operations.
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            // Convert the 32-bit integer to a float in [0, 1]
            // 4294967295.0f is the maximum value for a 32-bit unsigned integer.
            float normalized = static_cast<float>(state) * (1.0f / 4294967295.0f);
            // Map to [-1, 1]
            return normalized * 2.0f - 1.0f;
        }

    private:
        uint32_t state;
    };

    XorShift fastRNG;

    StringParameter* waveformParameter;

    // All tables now have FULL_TABLE_SIZE samples.
    // We use a pointer to float and an effective cycle length.
    std::atomic<const float*> waveformTable = nullptr;
    // Effective cycle length remains TABLE_SIZE (the extra sample is for interpolation only)
    size_t tableLength = TABLE_SIZE;

    enum class UseBlep { None, SawBlep };

    UseBlep useBlep;

    signalsmith::EllipticBlep<float> blep;
    signalsmith::EllipticBlepAllpass<float> allpass;

    // Choose waveform table based on the string.
    hash32 updateWaveform(hash32 waveformHash)
    {
        switch (waveformHash)
        {
        case hash("saw"):
            waveformTable.store(sawWaveTable.data());
            break;
        case hash("square"):
            waveformTable.store(squareWaveTable.data());
            break;
        case hash("tri"):
        case hash("triangle"):
            waveformTable.store(triangleWaveTable.data());
            break;
        case hash("noise"):
            waveformTable.store(nullptr);
            break;
        default:
            if (waveformHash != hash("sine"))
            {
                //std::cout << "Error! Unknown waveform: " << waveform << ", using default sine" << std::endl;
                waveformHash = hash("sine");
            }
            waveformTable.store(sineWaveTable.data());
            break;
        }

        return waveformHash;
    }

public:
    Oscillator(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("phase", AudioPort::PortType::Data);
        addInputPort("frequency", AudioPort::PortType::Signal);

        auto waveform = objParams.value("waveform", "sine");
        freq = objParams.value("freq", 440.0f);

        waveformParameter = addParameter<StringParameter>("Waveform", waveform);

        waveformParameter->informNodeOfChange = [this]()
        {
            waveformHash.store(updateWaveform(hash(waveformParameter->getValue())));
        };

        waveformHash.store(updateWaveform(hash(waveformParameter->getValue())));

        context->stringMap.intern("sine", "saw", "square", "triangle", "tri", "noise");

        fastRNG.setSeed(nodeID);
    }

    json getSerializedNode() override
    {
        if (auto* waveformString = context->stringMap.find(waveformHash))
        {
            nodeCreationData["waveform"] = *waveformString;
        }
        return nodeCreationData;
    }

    void processAudio(float* out, unsigned long frameCount) override
    {
        // Get input event buffers and audio buffers.
        const auto& events = inputPortBuffers[0]->getEvents();
        const auto& freqEvents = inputPortBuffers[1]->getEvents();
        bool useSignalFreq = inputPortBuffers[1]->isAnyConnectedPortSignal;
        auto freqIn = inputPortBuffers[1]->getAudioBuffer();
        auto output = outputPortBuffers[0]->getAudioBuffer();

        auto waveformTableToUse = waveformTable.load(std::memory_order_relaxed);
        if (waveformHash.load(std::memory_order_relaxed) == hash("saw"))
            useBlep = UseBlep::SawBlep;
        else
            useBlep = UseBlep::None;

        // These indices track events within the current block.
        unsigned int nextEventIndex = 0;
        unsigned int nextFreqEventIndex = 0;

        // Process each sample.
        // Note: We assume that the member variable 'phase' is normalized to [0,1) for all waveforms.
        for (unsigned int i = 0; i < frameCount; i++)
        {
            if (waveformTableToUse)
            {
                // --- Handle external phase-reset events ---
                while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i)
                {
                    // For a saw wave, apply a BLEP correction at reset.
                    if (useBlep == UseBlep::SawBlep)
                    {
                        blep.step(); // Ensure BLEP state is in sync.
                        // For a saw defined as 2×phase–1 the jump is 2.
                        blep.add(-2.0f, 1, 0.0f);
                    }
                    if (DataAtom* data = events[nextEventIndex]->data; data && data->type == DataAtom::DataType::Float)
                        phase = std::clamp(data->data.atom, 0.0f, 1.0f);
                    else
                        phase = 0.0f;
                    nextEventIndex++;
                }

                // --- Process frequency events ---
                while (!useSignalFreq && nextFreqEventIndex < freqEvents.size() && freqEvents[nextFreqEventIndex]->getTimeStamp() == i)
                {
                    if (auto newFreq = freqEvents[nextFreqEventIndex]->getAtom(0))
                    {
                        if (newFreq->type == DataAtom::DataType::Float)
                            freq = newFreq->data.atom;
                    }
                    nextFreqEventIndex++;
                }

                if (useSignalFreq)
                    freq = freqIn[1];

                // --- Advance phase ---
                // Compute the normalized phase increment (one cycle = 1.0).
                double dPhase = static_cast<double>(freq) / context->sampleRate;
                double phase_d = static_cast<double>(phase) + dPhase;

                // If we're using BLEP (i.e. for saw), handle natural wrap-around with correction.
                if (useBlep == UseBlep::SawBlep)
                {
                    blep.step();
                    if (phase_d >= 1.0)
                    {
                        double overshoot = phase_d - 1.0;
                        double t = overshoot / dPhase; // fractional offset within the current sample
                        phase_d -= 1.0;
                        blep.add(-2.0f, 1, static_cast<float>(t));
                    }
                }
                // Save the updated phase (wrap safety).
                phase = static_cast<float>(phase_d);
                if (phase >= 1.0f)
                    phase -= 1.0f;

                // --- Table lookup ---
                // Convert the normalized phase to a table index.
                double tableIndex = static_cast<double>(phase) * TABLE_SIZE;
                int idx = static_cast<int>(tableIndex);
                // Because our tables have FULL_TABLE_SIZE samples (TABLE_SIZE+1), we use idx+1 directly.
                int nextIdx = idx + 1;
                double frac = tableIndex - idx;
                double value = waveformTableToUse[idx] + frac * (waveformTableToUse[nextIdx] - waveformTableToUse[idx]);

                // --- Apply BLEP correction only for saw ---
                if (useBlep == UseBlep::SawBlep)
                {
                    value += blep.get();
                    // WARNING! Make sure allpass has been reset (either in class or manually)
                    value = allpass(static_cast<float>(value));
                }

                // --- Write the output sample with scaling ---
                output[i] = 0.5f * static_cast<float>(value);
            }
            else
            {
                output[i] = fastRNG.nextFloat();
            }
        }
    }
};
