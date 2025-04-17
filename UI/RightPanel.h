#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/Resizer.h"

class AudioNode;
class Parameter;
class Canvas;
class Object;

// ParamItem: Displays a parameter name and editor component
class ParamItem : public pptk::Component {
private:
    std::string paramName;
    Parameter* param = nullptr;
    std::unique_ptr<pptk::Component> editor;

public:
    ParamItem(const std::string& name, Parameter* itemParam);

    void resized() override;

    void render(NVGcontext* vg, const pptk::Theme& theme) override {
        nvgDrawRoundedRect(vg, 0, 3, width, height - 6, nvgRGB(43, 43, 43), nvgRGB(43, 43, 43), 6.0f);

        nvgFillColor(vg, nvgRGB(220, 220, 220));
        nvgFontFace(vg, "Regular");
        nvgFontSize(vg, 14.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, 16, height / 2, paramName.c_str(), nullptr);
    }
};

class RightPanel : public pptk::ResizableComponent {
public:
    RightPanel(Canvas* cnv);
    void setSelectedObjects(std::vector<Object*> objs);
    void updateUI();
    void resized() override;

    void render(NVGcontext* vg, const pptk::Theme& theme) override;

private:
    AudioNode* selectedNode = nullptr;
    int numSelected = 0;
    std::string parameterName;
    std::vector<std::unique_ptr<ParamItem>> paramItems;
};
