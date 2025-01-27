#pragma once

#include <utility>

#include "../UI_ToolKit/Component.h"

class ToggleButton : public pptk::Component {
public:
    std::function<void(bool)> onToggle = [](bool){};

    explicit ToggleButton(std::string off, std::string on, std::string font = "icons")
        : offCharacter(std::move(off))
        , onCharacter(std::move(on))
        , font(std::move(font))
    {};

    void mouseButtonDown(SDL_Event& e) override
    {
        state = !state;
        onToggle(state);
    }

    void mouseEnter(SDL_Event& e) override { hovered = true; };

    void mouseLeave(SDL_Event& e) override { hovered = false; };

    void render(NVGcontext* nvg) override
    {
        if (hovered)
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
    bool state = false;
};