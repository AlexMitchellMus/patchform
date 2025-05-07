#pragma once

#include "../AudioNodeBase.h"
#include "readerwriterqueue.h"
#include "Utility/ScopedTrippleBuffer.h"

class Table final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Table", "table", false);
    DEFINE_NODE_ALIASES("table");

    int numValues = defaultTableSize;  // hardcoded for now
    std::vector<float> mainBuffer;  // persistent audio buffer
    ScopedTrippleBuffer scopedSwap;  // audio->UI buffer

    SampleHandle waveformData;

    std::atomic<bool> isDirty;

    BoolParameter* emitOnLoadParam = nullptr;
    BoolParameter* saveTableOnClose = nullptr;

    bool saveContents = false;

public:
#ifdef PATCHFORM_WITH_GUI
    bool isDefaultUI() const override { return false; }

    moodycamel::ReaderWriterQueue<std::pair<int, float>> eventQueue;

    class UI final : public AudioNode::UI {
        pptk::Point lastPos{0, 0};
        bool hasLastPos = false;

        std::vector<float> values;
        int hoveredOver = -1;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setSize(300, 100);
            auto* tableNode = reinterpret_cast<Table*>(audioNode);
            tableNode->isDirty.store(true);
        }

        void drawGUI(NVGcontext* nvg) override {
            const int count = values.size();
            const float w = getWidth();
            const float h = getHeight();
            const float spacing = w / std::max(count, 1);

            nvgBeginPath(nvg);
            const auto col = nvgRGB(220, 220, 220);
            nvgStrokeColor(nvg, col);
            nvgLineStyle(nvg, NVG_LINE_SOLID);

            enum class TableStyle { PolyLine, Bars };
            TableStyle style = TableStyle::PolyLine;

            switch (style) {
                case TableStyle::Bars:
                    nvgStrokeWidth(nvg, 1.5f);
                    for (int i = 0; i < count; ++i) {
                        float x = i * spacing + spacing * 0.5f;
                        float y = h * (1.0f - values[i]);
                        nvgMoveTo(nvg, x, h);
                        nvgLineTo(nvg, x, y);
                    }
                    nvgStroke(nvg);
                    break;
                default:
                case TableStyle::PolyLine:
                    nvgStrokeWidth(nvg, 1.5f);
                    nvgLineJoin(nvg, NVG_ROUND);
                    nvgLineCap(nvg, NVG_ROUND);
                    for (int i = 0; i < count; ++i) {
                        float x = i * spacing + spacing * 0.5f;
                        float y = h * (0.5f - 0.5f * values[i]);
                        if (i == 0)
                            nvgMoveTo(nvg, x, y);
                        else
                            nvgLineTo(nvg, x, y);
                    }
                    nvgStroke(nvg);
                    break;
            }
        }

        void updateGraphValues() override
        {
            auto* tableNode = reinterpret_cast<Table*>(audioNode);
            if (tableNode->isDirty.exchange(false))
            {
                {
                    const auto reader = tableNode->scopedSwap.acquireScopedReader();
                    values.assign(reader.data, reader.data + tableNode->numValues);
                }
                repaint();
            }
        }

        void mouseMove(const pptk::Point& position) override {
            if (auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    const int count = values.size();
                    const float spacing = getWidth() / std::max(count, 1);
                    hoveredOver = std::clamp(static_cast<int>(position.x / spacing), 0, count - 1);
                    repaint();
                }
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override {
            if (auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    hoveredOver = -1;

                    const int count = values.size();
                    const float spacing = getWidth() / std::max(count, 1);
                    int index = std::clamp(static_cast<int>(e.sdlEvent.button.x / spacing), 0, count - 1);
                    float norm = 1.0f - std::clamp(e.sdlEvent.button.y / getHeight(), 0.0f, 1.0f);
                    float value = 2.0f * norm - 1.0f;  // remap [0,1] to [-1,1]
                    values[index] = value;

                    auto* tableNode = reinterpret_cast<Table*>(audioNode);
                    tableNode->eventQueue.enqueue({index, value});
                    tableNode->setNodeDirty();

                    repaint();
                }
                AudioNode::UI::mouseButtonDown(e);
            }
        }

        void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override {
            if (auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    auto* tableNode = reinterpret_cast<Table*>(audioNode);
                    const int count = tableNode->numValues;
                    const float spacing = getWidth() / std::max(count, 1);

                    if (!hasLastPos) {
                        lastPos = position;
                        hasLastPos = true;
                    }

                    int lastIndex = std::clamp(static_cast<int>(lastPos.x / spacing), 0, count - 1);
                    int currIndex = std::clamp(static_cast<int>(position.x / spacing), 0, count - 1);

                    float lastVal = (1.0f - std::clamp(lastPos.y / getHeight(), 0.0f, 1.0f)) * 2.0f - 1.0f;
                    float currVal = (1.0f - std::clamp(position.y / getHeight(), 0.0f, 1.0f)) * 2.0f - 1.0f;

                    if (lastIndex > currIndex) {
                        std::swap(lastIndex, currIndex);
                        std::swap(lastVal, currVal);
                    }

                    for (int i = lastIndex; i <= currIndex; ++i) {
                        float t = (currIndex == lastIndex) ? 1.0f : (float)(i - lastIndex) / (currIndex - lastIndex);
                        float interpolated = (1.0f - t) * lastVal + t * currVal;
                        values[i] = interpolated;
                        tableNode->eventQueue.enqueue({i, interpolated});
                    }

                    lastPos = position;
                    tableNode->setNodeDirty();
                    repaint();
                } else {
                    AudioNode::UI::mouseDrag(position, delta, button);
                }
            }
        }

        void mouseLeave(pptk::CompEvent& e) override {
            hoveredOver = -1;
            AudioNode::UI::mouseLeave(e);
        }

        void mouseButtonUp(pptk::CompEvent& e) override {
            hasLastPos = false;
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override {
        return std::make_unique<UI>(this);
    }
#endif

    Table(std::shared_ptr<NodeContext> context, const json& objParams)
        : AudioNode(context, AudioPort::PortType::Data, objParams) {
        mainBuffer.resize(numValues, 0.0f);
        scopedSwap.resize(numValues);

        auto bufferFromFile = objParams.value("data", mainBuffer);

        if (mainBuffer.size() != bufferFromFile.size())
        {
            std::vector<float> resampled(defaultTableSize);
            for (size_t i = 0; i < defaultTableSize; ++i) {
                float phase = (float)i / defaultTableSize;
                float srcIndex = phase * mainBuffer.size();
                int idx0 = (int)std::floor(srcIndex) % mainBuffer.size();
                int idx1 = (idx0 + 1) % mainBuffer.size();
                float frac = srcIndex - idx0;
                resampled[i] = mainBuffer[idx0] * (1.0f - frac) + mainBuffer[idx1] * frac;
            }
            mainBuffer = std::move(resampled);
        } else {
            mainBuffer = bufferFromFile;
        }

        {
             auto writer = scopedSwap.acquireScopedWriter();
             std::ranges::copy(mainBuffer, writer.data);
             isDirty.store(true);
        }

        addInputPort("control", AudioPort::Data);
        waveformData = SampleHandle::makeSampleHandle(numValues);
        std::copy(mainBuffer.begin(), mainBuffer.end(), waveformData.sample->samples.begin());

        emitOnLoadParam = addParameter<BoolParameter>("emitOnLoad", objParams.value("emitOnLoad", false));
        saveContents = objParams.value("saveContents", false);
        saveTableOnClose = addParameter<BoolParameter>("saveContents", saveContents);
        saveTableOnClose->onParameterChanged = [this]() {
            saveContents = saveTableOnClose->getValue();
        };
        eventOnLoad = emitOnLoadParam->getValue();
    }


    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override {
        auto& events = inputPortBuffers[0]->getEvents();
        bool newData = false;

        for (auto* event : events) {
            if (event->data && event->data->type == DataAtom::DataType::Sample) {
                auto incoming = event->data->data.sample;
                if (incoming.isValid()) {
                    const auto& incomingSamples = incoming.sample->samples;
                    mainBuffer = incomingSamples;
                    waveformData = incoming;
                    newData = true;
                }
            }
        }

#ifdef PATCHFORM_WITH_GUI
        std::pair<int, float> msg;
        while (eventQueue.try_dequeue(msg)) {
            if (msg.first >= 0 && msg.first < (int)mainBuffer.size())
                mainBuffer[msg.first] = msg.second;
            newData = true;
        }
#endif

        if (newData) {
            const auto writer = scopedSwap.acquireScopedWriter();
            std::ranges::copy(mainBuffer, writer.data);
            isDirty.store(true);
        }

        if (!waveformData.isValid()) return;
        auto& samples = waveformData.sample->samples;
        std::ranges::copy(mainBuffer, samples.begin());

        if (auto* e = context->eventPool.getFreeEvent()) {
            auto* dataAtom = context->eventPool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Sample;
            new(&dataAtom->data.sample) SampleHandle(waveformData);
            e->data = dataAtom;
            addEvent(0, e);
        }
    }

    json getSerializedNode() override {
        nodeCreationData["saveContents"] = saveContents;
        if (saveContents)
        {
            auto reader = scopedSwap.acquireScopedReader();
            std::vector<float> values(reader.data, reader.data + scopedSwap.size());
            nodeCreationData["data"] = values;
        } else
        {
            nodeCreationData.erase("data");
        }
        nodeCreationData["emitOnLoad"] = emitOnLoadParam->getValue();
        return nodeCreationData;
    }
};

REGISTER(Table);
