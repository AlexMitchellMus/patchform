#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/Resizer.h"
#include "../UI_ToolKit/TextEditor.h"

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
        nvgFillColor(vg, nvgRGB(220, 220, 220));
        nvgFontFace(vg, "Regular");
        nvgFontSize(vg, 14.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, 0, height / 2, paramName.c_str(), nullptr);
    }
};

// RightPanel: Displays a list of ParamItems for selected AudioNode
class RightPanel : public pptk::ResizableComponent {
public:
    RightPanel(Canvas* cnv);

    void canvasReloaded(Canvas* cnv);

    void setSelectedNode(AudioNode* node);

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

        float textX = 24; // Padding from the left edge
        float textY = 40; // Starting Y position with padding from the top

        nvgFillColor(vg, nvgRGB(220, 220, 220));
        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "SemiBold");
        nvgTextAlign(vg, NVG_ALIGN_LEFT);
        nvgText(vg, textX, textY, parameterName.c_str(), nullptr);
    }

private:
    AudioNode* selectedNode = nullptr;
    std::string parameterName;
    std::vector<std::unique_ptr<ParamItem>> paramItems; // Holds param UI elements
};
