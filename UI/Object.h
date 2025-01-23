//
// Created by alexw on 23/01/2025.
//

#pragma once

#include "../UI_Toolkit/Component.h"

#include "App.h"
#include "Port.h"

class Canvas;
class Object : public pptk::Component {
public:
    explicit Object(Component* parent, const std::string& name);

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta) override {
        setPosition(getPosition() + delta);
    }

    void mouseEnter(SDL_Event& e) override
    {
        isHovered = true;
    }

    void mouseLeave(SDL_Event& e) override
    {
        isHovered = false;
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        auto bgCol = nvgRGB(33, 33, 33);
        auto outLineCol = nvgRGB(45, 45, 45);
        if (isHovered) bgCol = outLineCol;
        nvgDrawRoundedRect(nvg, x, y, width, height, bgCol, outLineCol, 6.0f);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGB(190, 190, 190));
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        nvgText(nvg, x + 10, y + height / 2, name.c_str(), nullptr);
    }

    std::string& getName() { return name; }

private:
    std::string name;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;

    bool isHovered = false;
};

