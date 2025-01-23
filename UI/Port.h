#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/ComponentRegister.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "../Nodes/AudioPort.h"

#include "Canvas.h"

class Port : public pptk::Component {
public:
    explicit Port(Component* parent, int portNum) : Component(parent), portNum(portNum)
    {
    }

    void mouseButtonDown(SDL_Event& e) override;

    void mouseButtonUp(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta) override;

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
        nvgCircle(nvg, x + 5, y + 5, 5);
        //https://colorkit.co/color/1c4977/
        //nvgRGB(119, 28, 118)
        auto pink = nvgRGB(92, 28, 119);
        auto blue =  nvgRGB(28, 73, 119);
        auto portCol = portNum == 0 ? blue : pink;
        nvgFillColor(nvg, portCol); // Blue fill for ports
        nvgFill(nvg);

        if (isHovered | isHoveredFromCable)
        {
            nvgBeginPath(nvg);
            nvgCircle(nvg, x + 5, y + 5, 10);
            auto alphaCol = portCol;
            portCol.a = 120;
            nvgFillColor(nvg, portCol);
            nvgFill(nvg);
        }

    }

private:
    int portNum;

    Port* foundPort = nullptr;

    bool isHovered = false;
    bool isHoveredFromCable = false;
};