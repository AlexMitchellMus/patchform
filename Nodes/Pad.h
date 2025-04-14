#pragma once

class Pad final : public AudioNode {
    DEFINE_AND_REGISTER_NODE("Pad", "pad", false);
    DEFINE_NODE_ALIASES("pad");

    float normX = 0.0f, normY = 0.0f;

    std::atomic<float> xMin{-1.0f}, xMax{1.0f};
    std::atomic<float> yMin{-1.0f}, yMax{1.0f};

    FloatParameter* xMinParam = nullptr;
    FloatParameter* xMaxParam = nullptr;
    FloatParameter* yMinParam = nullptr;
    FloatParameter* yMaxParam = nullptr;

    bool isDefaultUI() const override { return false; };

public:
#ifdef PATCHFORM_WITH_GUI
    class UI final : public AudioNode::UI {
    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node) {
            setSize(100, 100);
        }

        void mouseDrag(const pptk::Point& pos, const pptk::Point& delta, pptk::Button bt) override {
            if (const auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    auto pad = reinterpret_cast<Pad*>(audioNode);
                    pad->normX = std::clamp(pad->normX + (delta.x / getWidth()) * 2.0f, -1.0f, 1.0f);
                    pad->normY = std::clamp(pad->normY + (delta.y / getHeight()) * 2.0f, -1.0f, 1.0f);
                    pad->setNodeDirty();
                    repaint();
                }
                else
                    AudioNode::UI::mouseDrag(pos, delta, bt);
            }
        }

        void mouseButtonDown(pptk::CompEvent& e) override {
            if (const auto cnv = findParentOfClass<Canvas>()) {
                if (cnv->isInLockedMode()) {
                    const auto pad = reinterpret_cast<Pad*>(audioNode);
                    if (e.sdlEvent.button.clicks == 2)
                    {
                        pad->normX = 0.0f;
                        pad->normY = 0.0f;
                        pad->setNodeDirty();
                        repaint();
                    }
                }
            }
            AudioNode::UI::mouseButtonDown(e);
        }

        void drawGUI(NVGcontext* vg) override {
            float w = getWidth(), h = getHeight();
            float cx = (reinterpret_cast<Pad*>(audioNode)->normX + 1.0f) * 0.5f * w;
            float cy = (reinterpret_cast<Pad*>(audioNode)->normY + 1.0f) * 0.5f * h;

            float centerX = w * 0.5f;
            float centerY = h * 0.5f;

            nvgBeginPath(vg);
            nvgStrokeColor(vg, nvgRGBA(100, 100, 100, 40));
            nvgStrokeWidth(vg, 1.0f);
            nvgMoveTo(vg, centerX, 0);
            nvgLineTo(vg, centerX, h);
            nvgMoveTo(vg, 0, centerY);
            nvgLineTo(vg, w, centerY);
            nvgLineStyle(vg, NVG_LINE_SOLID);
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgStrokeColor(vg, nvgRGB(200, 200, 200));
            nvgStrokeWidth(vg, 1.5f);
            nvgMoveTo(vg, cx, 0);
            nvgLineTo(vg, cx, h);
            nvgMoveTo(vg, 0, cy);
            nvgLineTo(vg, w, cy);
            nvgLineStyle(vg, NVG_LINE_SOLID);
            nvgStroke(vg);

            nvgBeginPath(vg);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgCircle(vg, cx, cy, 4);
            nvgFill(vg);
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override {
        return std::make_unique<UI>(this);
    }
#endif

    Pad(NodeContext* context, const json& j) : AudioNode(context, AudioPort::PortType::Data, j) {
        eventOnLoad = true;

        float xmin = j.value("x-min", -1.0f);
        float xmax = j.value("x-max",  1.0f);
        float ymin = j.value("y-min", -1.0f);
        float ymax = j.value("y-max",  1.0f);

        xMin = xmin;
        xMax = xmax;
        yMin = ymin;
        yMax = ymax;

        xMinParam = addParameter<FloatParameter>("x-min", xmin, -10000.0f, 10000.0f);
        xMaxParam = addParameter<FloatParameter>("x-max", xmax, -10000.0f, 10000.0f);
        yMinParam = addParameter<FloatParameter>("y-min", ymin, -10000.0f, 10000.0f);
        yMaxParam = addParameter<FloatParameter>("y-max", ymax, -10000.0f, 10000.0f);

        xMinParam->informNodeOfChange = [this]() { xMin.store(xMinParam->getValue()); };
        xMaxParam->informNodeOfChange = [this]() { xMax.store(xMaxParam->getValue()); };
        yMinParam->informNodeOfChange = [this]() { yMin.store(yMinParam->getValue()); };
        yMaxParam->informNodeOfChange = [this]() { yMax.store(yMaxParam->getValue()); };
    }

    json getSerializedNode() override {
        nodeCreationData["x-min"] = xMin.load();
        nodeCreationData["x-max"] = xMax.load();
        nodeCreationData["y-min"] = yMin.load();
        nodeCreationData["y-max"] = yMax.load();
        return nodeCreationData;
    }

    void processAudio(const float*, float*, const unsigned long, std::vector<MidiMessage>&) override {
        float scaledX = xMin + (normX + 1.0f) * 0.5f * (xMax - xMin);
        float scaledY = yMin + (normY + 1.0f) * 0.5f * (yMax - yMin);

        if (Event* e = context->eventPool.getFreeEvent()) {
            context->eventPool.addDataAtomTo(e, scaledX);
            context->eventPool.addDataAtomTo(e, scaledY);
            addEvent(0, e);
        }
    }
};

REGISTER(Pad);
