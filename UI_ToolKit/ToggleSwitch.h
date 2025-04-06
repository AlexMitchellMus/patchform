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
        constexpr float padding = 2.0f;
        const float knobRadius = radius - padding;
        const float knobX = state ? width - radius : radius;
        const float knobSize = knobRadius * 2.0f;

        // Background
        nvgBeginPath(vg);
        const NVGcolor bgColor = state ? nvgRGB(36, 130, 210) : nvgRGB(100, 100, 100);
        nvgDrawRoundedRect(vg, 0, 0, width, height, bgColor, bgColor, radius);

        // Knob (drawn as rounded rectangle instead of circle)
        nvgBeginPath(vg);
        const NVGcolor knobColor = state ? nvgRGB(255, 255, 255) : nvgRGB(180, 180, 180);
        nvgDrawRoundedRect(vg, knobX - knobRadius, radius - knobRadius, knobSize, knobSize, knobColor, knobColor, knobRadius);
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
