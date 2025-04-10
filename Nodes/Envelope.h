#pragma once

#include "AudioNodeBase.h"

class Envelope final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("AHR Envelope", "AHRenv", true);

    FloatParameter* attackValParam = nullptr;
    FloatParameter* holdValParam = nullptr;
    FloatParameter* decayValParam = nullptr;
    FloatParameter* attackSlopeParam = nullptr;
    FloatParameter* decaySlopeParam = nullptr;

    std::atomic<float> attackVal;
    std::atomic<float> holdVal{0.0f};
    std::atomic<float> decayVal;
    std::atomic<float> attackSlope{0.0f};
    std::atomic<float> decaySlope{0.0f};

    float envValue = 0.0f;

    enum Phase { Idle, Attack, Hold, Decay };
    Phase phase = Idle;

    int attackSampleCount = 1;
    int holdSampleCount = 1;
    int decaySampleCount = 1;
    int currentPhaseSample = 0;

    float port1val = 0.0f;

public:
    Envelope(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Events", AudioPort::PortType::Data);
        addInputPort("Signal", AudioPort::PortType::Signal);

        float attackMs = objParams.value("attack", 0.0f);
        float holdMs = objParams.value("hold", 0.0f);
        float decayMs = objParams.value("decay", 0.0f);
        float attackSlopeVal = objParams.value("attackSlope", 0.0f);
        float decaySlopeVal = objParams.value("decaySlope", 0.0f);

        attackValParam = addParameter<FloatParameter>("Attack", attackMs, 0.0f, 10000.0f);
        holdValParam = addParameter<FloatParameter>("Hold", holdMs, 0.0f, 10000.0f);
        decayValParam = addParameter<FloatParameter>("Decay", decayMs, 0.0f, 10000.0f);
        attackSlopeParam = addParameter<FloatParameter>("Attack Slope", attackSlopeVal, -1.0f, 1.0f);
        decaySlopeParam = addParameter<FloatParameter>("Decay Slope", decaySlopeVal, -1.0f, 1.0f);

        attackVal.store(attackMs);
        holdVal.store(holdMs);
        decayVal.store(decayMs);
        attackSlope.store(attackSlopeVal);
        decaySlope.store(decaySlopeVal);

        attackValParam->informNodeOfChange = [this]() {
            attackVal.store(attackValParam->getValue());
        };
        holdValParam->informNodeOfChange = [this]() {
            holdVal.store(holdValParam->getValue());
        };
        decayValParam->informNodeOfChange = [this]() {
            decayVal.store(decayValParam->getValue());
        };
        attackSlopeParam->informNodeOfChange = [this]() {
            attackSlope.store(attackSlopeParam->getValue());
        };
        decaySlopeParam->informNodeOfChange = [this]() {
            decaySlope.store(decaySlopeParam->getValue());
        };
    }

    json getSerializedNode() override
    {
        nodeCreationData["attack"] = attackValParam->getValue();
        nodeCreationData["hold"] = holdValParam->getValue();
        nodeCreationData["decay"] = decayValParam->getValue();
        nodeCreationData["attackSlope"] = attackSlopeParam->getValue();
        nodeCreationData["decaySlope"] = decaySlopeParam->getValue();
        return nodeCreationData;
    }

    float applySlope(float normPhase, float slope)
    {
        normPhase = std::clamp(normPhase, 0.0f, 1.0f);
        if (slope == 0.0f) return normPhase;
        if (slope < 0.0f) return 1.0f - std::pow(1.0f - normPhase, 1.0f + (-slope * 4.0f));
        return std::pow(normPhase, 1.0f + (slope * 4.0f));
    }

    void processAudio(const float*, float* out, const unsigned long frameCount, std::vector<MidiMessage>&) override
    {
        const auto& events = inputPortBuffers[0]->getEvents();
        const float* signal = inputPortBuffers[1]->getAudioBuffer();
        const auto& port1Events = inputPortBuffers[1]->getEvents();
        bool useSignalFreq = inputPortBuffers[1]->isAnyConnectedPortSignal;
        float* output = outputPortBuffers[0]->getAudioBuffer();

        unsigned int nextEventIndex = 0;
        unsigned int nextFreqEventIndex = 0;

        float atkMs = attackVal.load();
        float hldMs = holdVal.load();
        float decMs = decayVal.load();
        float atkSlope = attackSlope.load();
        float decSlope = decaySlope.load();

        attackSampleCount = std::max(1, static_cast<int>(atkMs * context->sampleRate / 1000.0f));
        holdSampleCount = std::max(1, static_cast<int>(hldMs * context->sampleRate / 1000.0f));
        decaySampleCount = std::max(1, static_cast<int>(decMs * context->sampleRate / 1000.0f));

        for (unsigned long i = 0; i < frameCount; ++i)
        {
            while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i)
            {
                phase = Attack;
                currentPhaseSample = 0;
                nextEventIndex++;
            }

            if (!useSignalFreq)
            {
                while (nextFreqEventIndex < port1Events.size() && port1Events[nextFreqEventIndex]->getTimeStamp() == i)
                {
                    port1val = port1Events[nextFreqEventIndex]->getAtomValue(0);
                    nextFreqEventIndex++;
                }
            }

            switch (phase)
            {
                case Attack:
                {
                    float norm = float(currentPhaseSample) / float(attackSampleCount);
                    envValue = applySlope(norm, atkSlope);
                    if (++currentPhaseSample >= attackSampleCount) {
                        phase = Hold;
                        currentPhaseSample = 0;
                    }
                    break;
                }
                case Hold:
                {
                    envValue = 1.0f;
                    if (++currentPhaseSample >= holdSampleCount) {
                        phase = Decay;
                        currentPhaseSample = 0;
                    }
                    break;
                }
                case Decay:
                {
                    float norm = float(currentPhaseSample) / float(decaySampleCount);
                    envValue = 1.0f - applySlope(norm, decSlope);
                    if (++currentPhaseSample >= decaySampleCount) {
                        envValue = 0.0f;
                        phase = Idle;
                    }
                    break;
                }
                case Idle:
                default:
                    break;
            }

            output[i] = envValue * (useSignalFreq ? signal[i] : port1val);
        }
    }
};
