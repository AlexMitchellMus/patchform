#pragma once

#include "Component.h"
#include "nanovg.h"

namespace pptk {

    class Label : public Component
    {
    public:
        explicit Label(const std::string& labelText = "", bool elide = true) : text(labelText), elide(elide)
        {
        }

        void setText(const std::string& newText)
        {
            text = newText;
            repaint();
        }

        void resized() override
        {
            displayText = text;

            if (!elide || width <= 0)
                return;

            float fullTextWidth = getTextWidthForFont("Regular", 14, text.c_str());
            if (fullTextWidth <= width)
                return;

            float dotsWidth = getTextWidthForFont("Regular", 14, "...");
            if (dotsWidth >= width)
            {
                displayText = "";
                return;
            }

            float maxTextWidth = width - dotsWidth;

            for (int i = static_cast<int>(text.length()); i >= 0; --i)
            {
                std::string temp = text.substr(0, i);
                float w = getTextWidthForFont("Regular", 14, temp.c_str());
                if (w <= maxTextWidth)
                {
                    displayText = temp + "...";
                    break;
                }
            }

            repaint();
        }

        std::string getText() const
        {
            return text;
        }

        void render(NVGcontext* vg, const Theme& theme) override
        {
            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgText(vg, 0, height * 0.5f, displayText.c_str(), nullptr);
        }

    private:
        std::string text;
        std::string displayText;

        bool elide = true;
    };

} // namespace pptk
