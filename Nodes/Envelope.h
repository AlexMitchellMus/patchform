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

    float attackVal;
    float decayVal;
    float envValue = 0.0f;
    bool isAttack = false;  // Track whether the envelope is in attack phase

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

    UI* createUI_Raw() override
    {
        return new UI(this);
    };
#endif
    Envelope(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Signal, objParams)
    {
        addInputPort("Events", AudioPort::PortType::Data);
        addInputPort("Signal", AudioPort::PortType::Signal);

        attackVal = objParams.value("attack", 0.0f) * (context->sampleRate / 1000);
        decayVal = objParams.value("decay", 0.0f) * (context->sampleRate / 1000);
    }

    void processAudio(float* out, const unsigned long frameCount) override
    {
        auto events = inputPortBuffers[0]->getEvents();
        auto signal = inputPortBuffers[1]->getAudioBuffer();;
        auto output = outputPort.getAudioBuffer();

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

            // Apply envelope to the signal
            output[i] = signal[i] * envValue;
        }
        //for (auto e : toRelease)
        //{
        //    //context->eventPool.releaseEvent(e);
        //}
    }
};
