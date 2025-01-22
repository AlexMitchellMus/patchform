//
// Created by alexw on 23/01/2025.
//

#pragma once

#include "../UI_Toolkit/Component.h"

#include "App.h"

class Canvas;
class Object : public pptk::Component {
public:
    explicit Object(Canvas* cnv, const std::string& name);

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta) override {
        setPosition(getPosition() + delta);
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        auto bgCol = nvgRGB(33, 33, 33);
        nvgDrawRoundedRect(nvg, x, y, width, height, bgCol, bgCol, 6.0f);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGB(255, 255, 255));
        nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        nvgText(nvg, x + width / 2, y + height / 2, name.c_str(), nullptr);
    }

private:
    std::string name;
    std::vector<Port*> inPorts;
    std::vector<Port*> outPorts;
};

