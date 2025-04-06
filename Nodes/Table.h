#pragma once

#include "AudioNodeBase.h"
#include "Print.h"

class Table final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Table", "sliders", false);

    int numValues = 0;
    std::vector<float> values;

public:
#ifdef PATCHFORM_WITH_GUI
    bool isDefaultUI() const override { return false; }

    moodycamel::ConcurrentQueue<std::pair<int, float>> eventQueue;

    class UI final : public AudioNode::UI {
        pptk::Point lastPos{0, 0};
        bool hasLastPos = false;

        std::vector<float> values;
        int hoveredOver = -1;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node) {
            setSize(300, 100);
            //setGuiIsTransparent(true);

            auto* sliderNode = reinterpret_cast<Table*>(audioNode);

            values = sliderNode->values;
        }

        void drawGUI(NVGcontext* nvg) override
        {
            const int count = values.size();
            const float w = getWidth();
            const float h = getHeight();
            const float spacing = w / std::max(count, 1);

            /*
            // TODO: Hovering doesn't work ATM, mouseMove(e) doesn't get called on objects??
            if (hoveredOver)
            {
                const auto col = nvgRGB(55, 55, 55);
                nvgBeginPath(nvg);
                float x = hoveredOver * spacing + spacing * 0.5f;
                nvgDrawRoundedRect(nvg, x, 0, 1.5, h, col, col, 0);
            }
            */

            nvgBeginPath(nvg);
            const auto col = nvgRGB(68, 68, 68);
            nvgStrokeColor(nvg, col);
            nvgLineStyle(nvg, NVG_LINE_SOLID);

            enum class TableStyle { PolyLine, Bars };
            TableStyle style = TableStyle::PolyLine;

            switch (style)
            {
                case TableStyle::Bars:
                    {
                        nvgStrokeWidth(nvg, 1.5f);
                        for (int i = 0; i < count; ++i) {
                            float x = i * spacing + spacing * 0.5f;
                            float y = h * (1.0f - values[i]);
                            nvgMoveTo(nvg, x, h);
                            nvgLineTo(nvg, x, y);
                        }

                        nvgStroke(nvg);
                    }
                break;
                defaut:
                case TableStyle::PolyLine:
                    {
                        nvgStrokeWidth(nvg, 1.5f);
                        nvgLineJoin(nvg, NVG_ROUND);
                        for (int i = 0; i < count; ++i) {
                            float x = i * spacing + spacing * 0.5f;
                            float y = h * (1.0f - values[i]);
                            if (i == 0)
                                nvgMoveTo(nvg, x, y);
                            else
                                nvgLineTo(nvg, x, y);
                        }
                    }
                break;
            }

            nvgStroke(nvg);
        }

        void mouseMove(const pptk::Point& position) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    const int count = values.size();
                    const float spacing = getWidth() / std::max(count, 1);
                    hoveredOver = std::clamp(static_cast<int>(position.x / spacing), 0, count - 1);
                    std::cout << "hovered over: " << hoveredOver << std::endl;
                    repaint();
                }
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    hoveredOver = -1;

                    const int count = values.size();
                    const float spacing = getWidth() / std::max(count, 1);
                    int index = std::clamp(static_cast<int>(e.sdlEvent.button.x / spacing), 0, count - 1);
                    float value = 1.0f - std::clamp(e.sdlEvent.button.y / getHeight(), 0.0f, 1.0f);
                    values[index] = value;

                    auto* sliderNode = reinterpret_cast<Table*>(audioNode);
                    sliderNode->eventQueue.enqueue({index, value});
                    sliderNode->setNodeDirty();

                    repaint();
                }
                else AudioNode::UI::mouseButtonDown(e);
            }
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
        {
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (cnv->isInLockedMode())
                {
                    auto* sliderNode = reinterpret_cast<Table*>(audioNode);
                    const int count = sliderNode->numValues;
                    const float spacing = getWidth() / std::max(count, 1);

                    if (!hasLastPos) {
                        lastPos = position;
                        hasLastPos = true;
                    }

                    int lastIndex = std::clamp(static_cast<int>(lastPos.x / spacing), 0, count - 1);
                    int currIndex = std::clamp(static_cast<int>(position.x / spacing), 0, count - 1);

                    float lastVal = 1.0f - std::clamp(lastPos.y / getHeight(), 0.0f, 1.0f);
                    float currVal = 1.0f - std::clamp(position.y / getHeight(), 0.0f, 1.0f);

                    if (lastIndex > currIndex) {
                        std::swap(lastIndex, currIndex);
                        std::swap(lastVal, currVal);
                    }

                    for (int i = lastIndex; i <= currIndex; ++i) {
                        float t = (currIndex == lastIndex) ? 1.0f : (float)(i - lastIndex) / (currIndex - lastIndex);
                        float interpolated = (1.0f - t) * lastVal + t * currVal;
                        values[i] = interpolated;
                        sliderNode->eventQueue.enqueue({i, interpolated});
                    }

                    lastPos = position;
                    sliderNode->setNodeDirty();
                    repaint();
                }
                else AudioNode::UI::mouseDrag(position, delta, button);
            }
        }

        void mouseLeave(pptk::CompEvent& e) override
        {
            hoveredOver = -1;

            AudioNode::UI::mouseLeave(e);
        }

        void mouseButtonUp(pptk::CompEvent& e) override
        {
            hasLastPos = false;
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override {
        return std::make_unique<UI>(this);
    }
#endif

    Table(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Spectral, objParams)
    {
        numValues = objParams.value("size", 8);
        values.resize(numValues, 0.0f);

        addInputPort("control", AudioPort::Data);
    }

    void processAudio(const float*, float*, unsigned long frameCount, std::vector<MidiMessage>&) override
    {
#ifdef PATCHFORM_WITH_GUI
        std::pair<int, float> msg;
        while (eventQueue.try_dequeue(msg)) {
            int index = msg.first;
            float val = msg.second;
            values[index] = val;
        }
#endif

        // Push the waveform table out to the audio buffer
        // TODO: Audio buffers need to resize to what they are holding
        float* out = outputPortBuffers[0]->getAudioBuffer();
        for (unsigned long i = 0; i < values.size(); ++i) {
            out[i] = values[i] * 2.0f - 1.0f;
        }
    }

    json getSerializedNode() override {
        nodeCreationData["size"] = numValues;
        return nodeCreationData;
    }
};
