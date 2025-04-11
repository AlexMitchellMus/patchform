#pragma once

#include "../UI_ToolKit/Component.h"
#include <functional>

namespace pptk
{
    class ToggleSwitch : public pptk::Component {
    public:
        std::function<void(bool)> onToggle = [](bool) {};

        ToggleSwitch() {
            setSize(40, 24);
        }

        void mouseButtonUp(pptk::CompEvent& e) override
        {
            const auto pos = globalToLocal(e.sdlEvent.button.x, e.sdlEvent.button.y);
            if (hitTest(pos.x, pos.y))
            {
                state = !state;
                onToggle(state);
                repaint();
            }
        }

        void render(NVGcontext* vg) override {
            const float radius = height * 0.5f;
            constexpr float padding = 3.0f;
            const float knobRadius = radius - padding;
            const float knobX = state ? width - radius : radius;
            const float knobSize = knobRadius * 2.0f;

            // Background
            nvgBeginPath(vg);
            const NVGcolor bgColor = state ? nvgRGB(36, 130, 210) : nvgRGB(100, 100, 100);
            nvgDrawRoundedRect(vg, 0, 0, width, height, bgColor, bgColor, radius);

            // Knob
            nvgBeginPath(vg);
            const NVGcolor knobColor = state ? nvgRGB(255, 255, 255) : nvgRGB(180, 180, 180);
            nvgDrawRoundedRect(vg, knobX - knobRadius, radius - knobRadius, knobSize, knobSize, knobColor, knobColor, knobRadius);

            // Draw check mark if 'on', or cross (X) if 'off', centered within the knob.
            if (getHeight() < 20)
                return;

            nvgStrokeColor(vg, bgColor);
            nvgStrokeWidth(vg, 2.0f);
            nvgLineStyle(vg, NVG_SOLID);
            nvgBeginPath(vg);
            if (state) {
                // Draw a tick (check mark)
                // Coordinates relative to the knob center (knobX, radius)
                // These values can be tweaked for the desired look.
                float tickStartX = knobX - knobRadius * 0.5f;
                float tickStartY = radius;
                float tickMidX   = knobX - knobRadius * 0.1f;
                float tickMidY   = radius + knobRadius * 0.4f;
                float tickEndX   = knobX + knobRadius * 0.6f;
                float tickEndY   = radius - knobRadius * 0.5f;
                nvgMoveTo(vg, tickStartX, tickStartY);
                nvgLineTo(vg, tickMidX, tickMidY);
                nvgLineTo(vg, tickEndX, tickEndY);
            } else {
                // Draw a cross (X)
                float crossOffset = knobRadius * 0.5f;
                nvgMoveTo(vg, knobX - crossOffset, radius - crossOffset);
                nvgLineTo(vg, knobX + crossOffset, radius + crossOffset);
                nvgMoveTo(vg, knobX + crossOffset, radius - crossOffset);
                nvgLineTo(vg, knobX - crossOffset, radius + crossOffset);
            }
            nvgStroke(vg);
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
} // end namespace pptk
