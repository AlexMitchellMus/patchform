/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class Envelope final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Envelope", "env");

    FloatParameter* attackValParam = nullptr;
    FloatParameter* decayValParam = nullptr;

    float attackVal;
    float decayVal;
    float envValue = 0.0f;
    bool isAttack = false;  // Track whether the envelope is in attack phase

    float port1val = 0.0f;

public:
#ifdef PATCHFORM_WITH_GUI
    class UI final : public AudioNode::UI
    {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(100, 100);
        };

        void render(NVGcontext* nvg) override
        {
            auto drawStar = [](NVGcontext* vg, float x, float y, float width, float height) {
                float cx = x + width / 2;
                float cy = y + height / 2;
                float radius = std::min(width, height) / 2;

                constexpr int points = 5;
                constexpr float angleStep = NVG_PI * 2 / points;

                // Compute star points
                nvgBeginPath(vg);

                for (int i = 0; i < points * 2; ++i) {
                    float angle = NVG_PI / 2 + i * angleStep / 2;
                    float r = (i % 2 == 0) ? radius : radius / 2.5f;
                    float px = cx + cos(angle) * r;
                    float py = cy - sin(angle) * r;

                    if (i == 0)
                        nvgMoveTo(vg, px, py);
                    else
                        nvgLineTo(vg, px, py);
                }

                nvgClosePath(vg);
                nvgFillColor(vg, nvgRGBA(255, 215, 0, 255)); // Gold color
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Black outline
                nvgStrokeWidth(vg, 2);
                nvgStroke(vg);
            };

            nvgBeginPath(nvg);
            auto bgCol = nvgRGB(33, 33, 33);
            auto outLineCol = nvgRGB(45, 45, 45);
            if (getIsHovered()) bgCol = outLineCol;
            if (getIsSelected()) outLineCol = nvgRGB(28, 73, 119);

            // Test for REALLY custom drawing! :-)
            drawStar(nvg, 0, 0, width, height);
            //nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);

            nvgFontSize(nvg, 18.0f);
            nvgFontFace(nvg, "Regular");
            nvgFillColor(nvg, nvgRGB(10, 10, 10));
            nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            nvgText(nvg, width/2, height / 2, "env", nullptr);
        }
    };
    /*
    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
    */
#endif
    Envelope(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Events", AudioPort::PortType::Data);
        addInputPort("Signal", AudioPort::PortType::Signal);

        float attackMs = objParams.value("attack", 0.0f);
        float deacyMs = objParams.value("decay", 0.0f);

        attackValParam = addParameter<FloatParameter>("Attack", attackMs, 0.0f, std::numeric_limits<float>::max());
        decayValParam = addParameter<FloatParameter>("Decay", deacyMs, 0.0f, std::numeric_limits<float>::max());
    }

    json getSerializedNode() override
    {
        nodeCreationData["attack"] = attackValParam->getValue();
        nodeCreationData["decay"] = decayValParam->getValue();
        return nodeCreationData;
    }

    void processAudio(float* out, const unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();
        auto signal = inputPortBuffers[1]->getAudioBuffer();
        auto port1Events = inputPortBuffers[1]->getEvents();
        auto useSignalFreq = inputPortBuffers[1]->isAnyConnectedPortSignal;

        auto output = outputPortBuffers[0]->getAudioBuffer();

        unsigned int nextFreqEventIndex = 0;

        attackVal = attackValParam->getValue() * (context->sampleRate / 1000);
        decayVal = decayValParam->getValue() * (context->sampleRate / 1000);

        std::vector<Event*> toRelease;
        unsigned long nextEventIndex = 0;

        for (unsigned long i = 0; i < frameCount; i++)
        {
            while (nextEventIndex < events.size() && events[nextEventIndex]->getTimeStamp() == i) {
                envValue = 0.0f;
                isAttack = true;
                nextEventIndex++;
                //std::cout << "Envelope " << nodeID << " triggered" << std::endl;
            }
            if (!useSignalFreq)
            {
                while (nextFreqEventIndex < port1Events.size() && port1Events[nextFreqEventIndex]->getTimeStamp() == i) {
                    port1val = port1Events[nextFreqEventIndex]->getAtomValue(0);
                    nextFreqEventIndex++;
                }
            }

            if (isAttack)
            {
                // Attack phase: Ramp up from 0 to 1
                envValue += (1.0f / attackVal);
                if (envValue >= 1.0f) {
                    envValue = 1.0f;
                    isAttack = false;  // Switch to decay phase after reaching 1
                }
            }
            else
            {
                // Decay phase: Ramp down from 1 towards 0
                envValue -= (1.0f / decayVal);
                if (envValue <= 0.0f) {
                    envValue = 0.0f;
                }
            }

            if (!useSignalFreq)
                output[i] = port1val * envValue;
            else
                output[i] = signal[i] * envValue;
        }
    }
};
