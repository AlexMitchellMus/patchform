#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class Dial final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Dial", "dial", false);

    bool firstRun = true;

    float dialValue = 0.0f; // normalized (0-1)

    FloatParameter* defaultValueParam = nullptr;
    FloatParameter* minValueParam = nullptr;
    FloatParameter* maxValueParam = nullptr;

    BoolParameter* emitOnLoadParam = nullptr;

    std::atomic<float> minValue = 0.0f;
    std::atomic<float> maxValue = 1.0f;

public:
#ifdef PATCHFORM_WITH_GUI

    bool isDefaultUI() const override { return false; }

    moodycamel::ConcurrentQueue<float> eventQueue;

    class UI final : public AudioNode::UI {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node) {
            setSize(100, 100);
            setGuiIsTransparent(true);

            auto dial = reinterpret_cast<Dial*>(audioNode);

            float minV = dial->minValue;
            float maxV = dial->maxValue;
            float actualValue = dial->dialValue * (maxV - minV) + minV;

            value = (maxV != minV) ? (actualValue - minV) / (maxV - minV) : 0.0f;
        }

        float valueToAngle(float value) {
            return (minAngle - NVG_PI * 0.5f) + (maxAngle - minAngle) * value;
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override {
            if (auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    value -= delta.y * 0.005f * cnv->scale;
                    value = std::clamp(value, 0.0f, 1.0f);

                    auto dial = reinterpret_cast<Dial*>(audioNode);
                    dial->eventQueue.enqueue(value);
                    dial->setNodeDirty();

                    repaint();
                } else {
                    AudioNode::UI::mouseDrag(position, delta, button);
                }
            }
        }

        void drawGUI(NVGcontext* nvg) override {
            const auto centre = getWidth() * 0.5f;
            const auto radius = getWidth() * 0.4f;

            nvgBeginPath(nvg);
            nvgCircle(nvg, centre, centre, radius);
            nvgFillColor(nvg, nvgRGBA(50, 50, 50, 255));
            nvgFill(nvg);

            float dotRadius = radius * 0.2f;
            auto angle = valueToAngle(value);
            float dotX = centre + (radius - dotRadius - 10) * cos(angle);
            float dotY = centre + (radius - dotRadius - 10) * sin(angle);

            nvgBeginPath(nvg);
            nvgCircle(nvg, dotX, dotY, dotRadius);
            nvgFillColor(nvg, nvgRGBA(28, 28, 28, 255));
            nvgFill(nvg);
        }

    private:
        float value = 0.0f;
        const float minAngle = -NVG_PI * 0.75f;
        const float maxAngle =  NVG_PI * 0.75f;
    };

    std::unique_ptr<AudioNode::UI> makeUI() override {
        return std::make_unique<UI>(this);
    }

#endif

    Dial(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        float minV = objParams.value("min", 0.0f);
        float maxV = objParams.value("max", 1.0f);
        float value = objParams.value("default", minV); // actual value
        bool emitOnLoad = objParams.value("emitOnLoad", false);

        minValue = minV;
        maxValue = maxV;

        // Convert actual value to normalized dial position
        if (maxV != minV)
            dialValue = (value - minV) / (maxV - minV);
        else
            dialValue = 0.0f;

        defaultValueParam = addParameter<FloatParameter>("default", value, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        minValueParam     = addParameter<FloatParameter>("Min", minV, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        maxValueParam     = addParameter<FloatParameter>("Max", maxV, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        emitOnLoadParam   = addParameter<BoolParameter>("emitOnLoad", emitOnLoad);

        minValueParam->informNodeOfChange = [this]() {
            minValue.store(minValueParam->getValue());
        };

        maxValueParam->informNodeOfChange = [this]() {
            maxValue.store(maxValueParam->getValue());
        };

        eventOnLoad = true;

        firstRun = emitOnLoad;
    }

    json getSerializedNode() override {
        float minV = minValue.load();
        float maxV = maxValue.load();
        float actualValue = dialValue * (maxV - minV) + minV;

        nodeCreationData["min"] = minV;
        nodeCreationData["max"] = maxV;
        nodeCreationData["default"] = actualValue;
        nodeCreationData["emitOnLoad"] = emitOnLoadParam->getValue();
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float* in, float* out, const unsigned long frameCount, std::vector<MidiMessage>& midiMessage) override
    {
        const float minV = minValue.load();
        const float maxV = maxValue.load();

        while (eventQueue.try_dequeue(dialValue))
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                float actual = dialValue * (maxV - minV) + minV;
                context->eventPool.addDataAtomTo(e, actual);
                addEvent(0, e);
            }
        }

        if (firstRun)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                float actual = dialValue * (maxV - minV) + minV;
                context->eventPool.addDataAtomTo(e, actual);
                addEvent(0, e);
            }
            firstRun = false;
        }
    }
#endif
};
