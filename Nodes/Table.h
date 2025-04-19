#pragma once

#include "AudioNodeBase.h"
#include "readerwriterqueue.h"

class Table final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Table", "table", false);
    DEFINE_NODE_ALIASES("table");

    int numValues = defaultTableSize;  // hardcoded for now
    std::vector<float> bufferA;
    std::vector<float> bufferB;

    std::atomic<bool> bufferReady{false};

    SampleHandle waveformData;

    std::atomic<bool> isDirty;

    BoolParameter* emitOnLoadParam = nullptr;
    BoolParameter* saveTableOnClose = nullptr;

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
        explicit UI(AudioNode* node) : AudioNode::UI(node) {
            setSize(300, 100);
            auto* tableNode = reinterpret_cast<Table*>(audioNode);
            values = tableNode->bufferA;
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

        void updateGraphValues() override {
            auto* tableNode = reinterpret_cast<Table*>(audioNode);
            if (tableNode->bufferReady.exchange(false)) {
                tableNode->bufferB = tableNode->bufferA;  // snapshot
                values = tableNode->bufferB;
                repaint();
            } else if (tableNode->isDirty.exchange(false)) {
                values = tableNode->bufferB;
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
                } else {
                    AudioNode::UI::mouseButtonDown(e);
                }
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

    Table(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::Data, objParams)
    {
        bufferA.resize(numValues, 0.0f);
        bufferB.resize(numValues, 0.0f);

        auto bufferFromFile = objParams.value("data", bufferA);

        if (bufferA.size() != bufferFromFile.size()) {
            std::vector<float> resampled(defaultTableSize);
            for (size_t i = 0; i < defaultTableSize; ++i) {
                float phase = (float)i / defaultTableSize;
                float srcIndex = phase * bufferA.size();
                int idx0 = (int)std::floor(srcIndex) % bufferA.size();
                int idx1 = (idx0 + 1) % bufferA.size();
                float frac = srcIndex - idx0;
                resampled[i] = bufferA[idx0] * (1.0f - frac) + bufferA[idx1] * frac;
            }
            bufferA = std::move(resampled);
        } else
            bufferA = bufferFromFile;

        addInputPort("control", AudioPort::Data);

        waveformData = SampleHandle::makeSampleHandle(numValues);

        // Populate the waveform buffer with samples from bufferA
        // FIXME: Are we sure we can do this from the UI thread!?
        auto& samples = waveformData.sample->samples;
        for (unsigned long i = 0; i < bufferA.size(); ++i) {
            samples[i] = bufferA[i];
        }
        bufferReady.store(true, std::memory_order_release);

        bool emitOnLoad = objParams.value("emitOnLoad", false);
        emitOnLoadParam   = addParameter<BoolParameter>("emitOnLoad", emitOnLoad);

        bool saveOnClose = objParams.value("saveOnClose", false);
        saveTableOnClose = addParameter<BoolParameter>("saveTableOnClose", saveOnClose);

        eventOnLoad = emitOnLoad;
    }

    void processAudio(const float*, float*, unsigned long, std::vector<MidiMessage>&) override
    {
        auto& events = inputPortBuffers[0]->getEvents();

        for (auto* event : events)
        {
            if (event->data && event->data->type == DataAtom::DataType::Sample) {
                auto incoming = event->data->data.sample;
                if (incoming.isValid()) {
                    const auto& incomingSamples = incoming.sample->samples;
                    bufferA = incomingSamples;
                    bufferReady.store(true, std::memory_order_release);
                    waveformData = incoming;  // reuse handle
                    isDirty.store(true);
                }
            }
        }
#ifdef PATCHFORM_WITH_GUI
        std::pair<int, float> msg;
        while (eventQueue.try_dequeue(msg)) {
            int index = msg.first;
            float val = msg.second;
            if (index >= 0 && index < (int)bufferA.size())
                bufferA[index] = val;
        }
        bufferReady.store(true, std::memory_order_release);
#endif

        if (!waveformData.isValid())
            return;

        auto& samples = waveformData.sample->samples;

        for (unsigned long i = 0; i < bufferA.size(); ++i) {
            samples[i] = bufferA[i];
        }

        if (auto* e = context->eventPool.getFreeEvent())
        {
            auto* dataAtom = context->eventPool.allocateDataAtom();
            dataAtom->type = DataAtom::DataType::Sample;
            // Placement new
            new(&dataAtom->data.sample) SampleHandle(waveformData);
            e->data = dataAtom;
            addEvent(0, e);
        }
    }

    json getSerializedNode() override
    {
        auto saveData = saveTableOnClose->getValue();
        nodeCreationData["saveOnClose"] = saveTableOnClose->getValue();
        if (saveData)
        {
            if (bufferReady.exchange(false, std::memory_order_acquire))
            {
                bufferB = bufferA;
            }
            nodeCreationData["size"] = static_cast<int>(bufferB.size());
            nodeCreationData["data"] = bufferB;
        }

        nodeCreationData["emitOnLoad"] = emitOnLoadParam->getValue();
        return nodeCreationData;
    }
};

REGISTER(Table);
