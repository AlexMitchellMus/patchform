#pragma once

#include "AudioNodeBase.h"

class Slider final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Slider", "slider", false);
    DEFINE_NODE_ALIASES("slider");

    bool firstRun = true;
    float sliderValue = 0.0f; // normalized (0-1)

    FloatParameter* defaultValueParam = nullptr;
    FloatParameter* minValueParam = nullptr;
    FloatParameter* maxValueParam = nullptr;
    BoolParameter* emitOnLoadParam = nullptr;
    ListParameter* orientationParam = nullptr;

    std::atomic<float> minValue = 0.0f;
    std::atomic<float> maxValue = 1.0f;

    enum class Orientation { Horizontal, Vertical };

    Orientation orientation = Orientation::Vertical;

public:
#ifdef PATCHFORM_WITH_GUI
    bool isDefaultUI() const override { return false; }
    moodycamel::ConcurrentQueue<float> eventQueue;

    class UI final : public AudioNode::UI
    {
        float value = 0.0f;
        Orientation orientation;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setGuiIsTransparent(true);

            auto* s = reinterpret_cast<Slider*>(audioNode);
            float minV = s->minValue;
            float maxV = s->maxValue;
            float actualValue = s->sliderValue * (maxV - minV) + minV;
            value = (maxV != minV) ? (actualValue - minV) / (maxV - minV) : 0.0f;

            setOrientation(s->orientation);

            updateSize();
        }

        void setOrientation(const Orientation o)
        {
            orientation = o;
            updateSize();
        }

        void updateSize()
        {
            if (orientation == Orientation::Horizontal)
            {
                setSize(30 * 6, 30);
            }
            else
                setSize(30, 30 * 6);

            if (const auto cnv = findParentOfClass<Canvas>())
            {
                cnv->updateConnectionsPosition();
            }

            repaint();
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
        {
            if (auto* cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    auto* s = reinterpret_cast<Slider*>(audioNode);
                    const float thumbSize = 12.0f;
                    const float pixelRange = (orientation == Orientation::Vertical ? getHeight() : getWidth()) - thumbSize;
                    float deltaNorm = (orientation == Orientation::Vertical ? delta.y : delta.x) / pixelRange;
                    if  (orientation == Orientation::Vertical)
                        value -= deltaNorm;
                    else
                        value += deltaNorm;

                    value = std::clamp(value, 0.0f, 1.0f);
                    s->eventQueue.enqueue(value);
                    s->setNodeDirty();
                    repaint();
                }
                else
                {
                    AudioNode::UI::mouseDrag(position, delta, button);
                }
            }
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const float width = getWidth(), height = getHeight();
            constexpr float thumbSize = 12.0f;
            constexpr float halfThumb = thumbSize * 0.5f;
            constexpr float trackInset = 7.0f;
            constexpr float trackThickness = 6.0f;

            const float trackSize = (orientation == Orientation::Vertical ? height : width) - trackInset * 2.0f;
            const float thumbTravel = trackSize - thumbSize;
            const float thumbPos =   (orientation == Orientation::Vertical ? 1.0f - value : value) * thumbTravel + trackInset + halfThumb;

            const auto trackCol = nvgRGB(60, 60, 60);
            const auto thumbCol = nvgRGB(200, 200, 200);
            const auto trackActive = nvgRGB(36, 130, 210);

            // Center the track depending on orientation
            const float trackX = (orientation == Orientation::Vertical)
                                     ? (width * 0.5f - trackThickness * 0.5f)
                                     : trackInset;
            const float trackY = (orientation == Orientation::Vertical)
                                     ? trackInset
                                     : (height * 0.5f - trackThickness * 0.5f);

            // Draw background track
            nvgBeginPath(nvg);
            if (orientation == Orientation::Vertical)
            {
                nvgDrawRoundedRect(nvg, trackX, trackY, trackThickness, height - 2 * trackInset, trackCol, trackCol, 3);

                const float activeHeight = (height - 2 * trackInset) - (thumbPos - halfThumb - trackInset);
                nvgDrawRoundedRect(nvg, trackX, thumbPos - halfThumb, trackThickness, activeHeight, trackActive,
                                   trackActive, 3);
            }
            else
            {
                nvgDrawRoundedRect(nvg, trackX, trackY, width - 2 * trackInset, trackThickness, trackCol, trackCol, 3);

                const float activeWidth = thumbPos + halfThumb - trackInset;
                nvgDrawRoundedRect(nvg, trackInset, trackY, activeWidth, trackThickness, trackActive, trackActive, 3);
            }

            // Draw thumb (always centered on track)
            nvgBeginPath(nvg);
            if (orientation == Orientation::Vertical)
            {
                const float thumbX = (width - thumbSize) * 0.5f;
                nvgDrawRoundedRect(nvg, thumbX, thumbPos - halfThumb, thumbSize, thumbSize, thumbCol, thumbCol,
                                   thumbSize * 0.5f);
            }
            else
            {
                const float thumbY = (height - thumbSize) * 0.5f;
                nvgDrawRoundedRect(nvg, thumbPos - halfThumb, thumbY, thumbSize, thumbSize, thumbCol, thumbCol,
                                   thumbSize * 0.5f);
            }
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    }
#endif

    Slider(NodeContext* context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        float minV = objParams.value("min", 0.0f);
        float maxV = objParams.value("max", 1.0f);
        float value = objParams.value("default", minV);
        bool emitOnLoad = objParams.value("emitOnLoad", false);
        std::string orientStr = objParams.value("orientation", "vertical");

        minValue = minV;
        maxValue = maxV;
        orientation = (orientStr == "horizontal") ? Orientation::Horizontal : Orientation::Vertical;

        sliderValue = (maxV != minV) ? (value - minV) / (maxV - minV) : 0.0f;

        defaultValueParam = addParameter<FloatParameter>("default", value, -std::numeric_limits<float>::max(),
                                                         std::numeric_limits<float>::max());
        minValueParam = addParameter<FloatParameter>("Min", minV, -std::numeric_limits<float>::max(),
                                                     std::numeric_limits<float>::max());
        maxValueParam = addParameter<FloatParameter>("Max", maxV, -std::numeric_limits<float>::max(),
                                                     std::numeric_limits<float>::max());
        emitOnLoadParam = addParameter<BoolParameter>("emitOnLoad", emitOnLoad);
        orientationParam = addParameter<ListParameter>("Orientation",
                                                       std::vector<std::string>{"vertical", "horizontal"}, orientStr);

        orientationParam->updateNodeUI = [this](const std::variant<int, float, std::string>& val)
        {
            if (auto orientStr = std::get_if<std::string>(&val))
            {
                orientation = *orientStr == "horizontal" ? Orientation::Horizontal : Orientation::Vertical;
                if (auto* ui = getUI())
                {
                    if (auto sliderUi = dynamic_cast<Slider::UI*>(ui))
                    {
                        sliderUi->setOrientation(orientation);
                    }
                }
            }
        };

        minValueParam->informNodeOfChange = [this]()
        {
            minValue.store(minValueParam->getValue());
        };
        maxValueParam->informNodeOfChange = [this]()
        {
            maxValue.store(maxValueParam->getValue());
        };
        orientationParam->informNodeOfChange = [this]()
        {
            orientation = orientationParam->getAsString() == "horizontal"
                              ? Orientation::Horizontal
                              : Orientation::Vertical;
        };

        eventOnLoad = true;
        firstRun = emitOnLoad;
    }

    json getSerializedNode() override
    {
        float minV = minValue.load();
        float maxV = maxValue.load();
        float actualValue = sliderValue * (maxV - minV) + minV;
        nodeCreationData["min"] = minV;
        nodeCreationData["max"] = maxV;
        nodeCreationData["default"] = actualValue;
        nodeCreationData["emitOnLoad"] = emitOnLoadParam->getValue();
        nodeCreationData["orientation"] = orientationParam->getAsString();
        return nodeCreationData;
    }

#ifdef PATCHFORM_WITH_GUI
    void processAudio(const float* in, float* out, const unsigned long frameCount,
                      std::vector<MidiMessage>& midiMessage) override
    {
        const float minV = minValue.load();
        const float maxV = maxValue.load();

        while (eventQueue.try_dequeue(sliderValue))
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                float actual = sliderValue * (maxV - minV) + minV;
                context->eventPool.addDataAtomTo(e, actual);
                addEvent(0, e);
            }
        }

        if (firstRun)
        {
            if (Event* e = context->eventPool.getFreeEvent())
            {
                float actual = sliderValue * (maxV - minV) + minV;
                context->eventPool.addDataAtomTo(e, actual);
                addEvent(0, e);
            }
            firstRun = false;
        }
    }
#endif
};

REGISTER(Slider);
