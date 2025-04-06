#pragma once

#include "../UI_ToolKit/Component.h"
#include <functional>

class ToggleSwitch : public pptk::Component {
public:
    std::function<void(bool)> onToggle = [](bool) {};

    ToggleSwitch() {
        setSize(40, 24);
    }

    void mouseButtonDown(pptk::CompEvent&) override {
        state = !state;
        onToggle(state);
        repaint();
    }

    void render(NVGcontext* vg) override {
        const float radius = height * 0.5f;
        const float padding = 2.0f;
        const float knobRadius = radius - padding;
        const float knobX = state ? width - radius : radius;

        // Background
        NVGcolor bgColor = state ? nvgRGB(36, 130, 210) : nvgRGB(100, 100, 100);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0, 0, width, height, radius);
        nvgFillColor(vg, bgColor);
        nvgFill(vg);

        // Knob
        nvgBeginPath(vg);
        nvgCircle(vg, knobX, radius, knobRadius);
        nvgFillColor(vg, state ? nvgRGB(255, 255, 255) : nvgRGB(180, 180, 180));
        nvgFill(vg);
    }

    void setState(bool newState) {
        if (state != newState) {
            state = newState;
            repaint();
        }
    }

    bool getState() const {
        return state;
    }

private:
    bool state = false;
};
