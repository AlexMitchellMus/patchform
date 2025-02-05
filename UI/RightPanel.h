#pragma once

#include "../UI_ToolKit/Component.h"

namespace pptk
{
    class TextEditor;
}

class AudioNode;
class Parameter;
class Canvas;

// ParamItem: Displays a parameter name and an editable TextBox
class ParamItem : public pptk::Component {
private:
    std::string paramName;
    Parameter* param = nullptr;
    std::unique_ptr<pptk::TextEditor> textBox;

public:
    ParamItem(const std::string& name, Parameter* itemParam);

    void resized() override;

    void render(NVGcontext* vg) override {
        // Draw parameter name on the left
        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgFontFace(vg, "Regular");
        nvgFontSize(vg, 14.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, 10, height / 2, paramName.c_str(), nullptr);
    }
};

// RightPanel: Displays a list of ParamItems for selected AudioNode
class RightPanel : public pptk::Component {
public:
    RightPanel(Canvas* cnv);

    void setSelectedNode(AudioNode* node) {
        if (node == nullptr)
        {
            selectedNode = nullptr;
            paramItems.clear();
            repaint();
        }
        selectedNode = node;
        updateUI();
    }

    void updateUI();

    void resized() override;

    void render(NVGcontext* vg) override {
        nvgFillColor(vg, nvgRGB(33, 33, 33));
        nvgFillRect(vg, 0, 0, width, height);

        // Vertical edge line (on left)
        nvgBeginPath(vg);
        nvgMoveTo(vg, 0.5f, 0);
        nvgLineTo(vg, 0.5f, height);
        nvgStrokeColor(vg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(vg, 1.0f);
        nvgStroke(vg);

        nvgFillColor(vg, nvgRGB(255, 255, 255));
        nvgFontSize(vg, 16.0f);
        nvgFontFace(vg, "SemiBold");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, 20, 30, "Parameters", nullptr);
    }

private:
    AudioNode* selectedNode = nullptr;
    std::vector<std::unique_ptr<ParamItem>> paramItems; // Holds param UI elements
};
