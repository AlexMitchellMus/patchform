#pragma once

#include <utility>

#include "../UI_ToolKit/Component.h"

class ToggleButton : public pptk::Component {
public:
    std::function<void(bool)> onToggle = [](bool){};
    std::function<void()> onClick = [](){};

    explicit ToggleButton(std::string off, std::string on, std::string font = "icons")
        : offCharacter(std::move(off))
        , onCharacter(std::move(on))
        , font(std::move(font))
    {};

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        state = !state;
        onToggle(state);

        onClick();

        repaint();
    }

    bool getState()
    {
        return state;
    }

    void setActive(bool active)
    {
        isActive = active;
        repaint();
    }

    void mouseEnter(pptk::CompEvent& e) override
    {
        hovered = true;
        repaint();
    };

    void mouseLeave(pptk::CompEvent& e) override
    {
        hovered = false;
        repaint();
    };

    void render(NVGcontext* nvg) override
    {
        if (hovered || isActive)
        {
            auto bgCol = nvgRGBA(0, 0, 0, 30);
            nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, bgCol, 8);
        }

        nvgFontSize(nvg, 24.0f);
        nvgFontFace(nvg, font.c_str());
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color
        nvgText(nvg, 5, 24, !state ? offCharacter.c_str() : onCharacter.c_str(), nullptr);
    }

private:
    std::string onCharacter;
    std::string offCharacter;
    std::string font;
    bool hovered = false;
    bool isActive = false;
    bool state = false;
};