#pragma once

#include "Component.h"
#include "nanovg.h"

namespace pptk {

    class Label : public Component
    {
    public:
        Label(const std::string& labelText = "")
            : text(labelText)
        {
        }

        void setText(const std::string& newText)
        {
            text = newText;
            repaint();
        }

        std::string getText() const
        {
            return text;
        }

        void render(NVGcontext* vg) override
        {
            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgText(vg, 0, height * 0.5f, text.c_str(), nullptr);
        }

    private:
        std::string text;
    };

} // namespace pptk
