#pragma once

#include "PopupListComponent.h"

namespace pptk
{
    class DropdownSelector : public Component
    {
    public:
        DropdownSelector(const std::vector<std::string>& opts)
            : options(opts)
        {
            if (!options.empty())
                selected = options[0];
        }

        void updateOptions(const std::vector<std::string>& newOptions) {
            options = newOptions;

            // Auto-select first item if available
            if (!options.empty())
                selected = options[0];
            else
                selected.clear();

            repaint();
        }

        void setOnSelect(std::function<void(const std::string&)> cb) { onSelect = std::move(cb); }

        void render(NVGcontext* vg) override
        {
            //Background
            auto bgCol = nvgRGB(40, 40, 40);
            nvgDrawRoundedRect(vg, 0, 0, width, height, bgCol, bgCol, 3);

            nvgBeginPath(vg);
            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgText(vg, 10, getHeight() * 0.5f, selected.c_str(), nullptr);

            // Define parameters for the downward chevron
            float chevronSize = 8.0f;     // Adjust the size of the chevron
            float rightPadding = 10.0f;   // Distance from the right edge
            float xStart = getWidth() - rightPadding - chevronSize;
            float yCenter = getHeight() * 0.5f;

            // Draw a downward-pointing chevron using stroked lines
            nvgBeginPath(vg);
            // Top left vertex of the chevron
            nvgMoveTo(vg, xStart, yCenter - chevronSize / 2);
            // Bottom vertex (tip) of the chevron
            nvgLineTo(vg, xStart + chevronSize / 2, yCenter);
            // Top right vertex of the chevron
            nvgLineTo(vg, xStart + chevronSize, yCenter - chevronSize / 2);

            // Set stroke properties and render the chevron
            nvgStrokeColor(vg, nvgRGB(220, 220, 220));
            nvgStrokeWidth(vg, 1.0f);
            nvgLineStyle(vg, NVG_SOLID);
            nvgStroke(vg);
        }

        void mouseButtonDown(pptk::CompEvent&) override
        {
            // Dismiss if already open
            if (popupMenu && popupMenu->isVisible())
            {
                popupMenu->close();
                popupMenu.reset();
                setPopupComponent(nullptr);
                return;
            }

            auto* self = this;

            auto popup = std::make_unique<pptk::PopupListComponent>(options);
            popup->onItemSelected = [self](const std::string& choice)
            {
                self->selected = choice;
                if (self->onSelect)
                    self->onSelect(choice);

                self->repaint();

                if (self->popupMenu)
                {
                    self->popupMenu->close();
                }
            };

            popupMenu = pptk::SafePointer(popup.get());
            setPopupComponent(std::move(popup));
            getRootComponent()->addComponent(popupMenu.get());

            // Position below dropdown
            auto globalPos = localToGlobal(0, getHeight());
            popupMenu->setPosition(globalPos.x, globalPos.y);
            popupMenu->registerMouseListener(this);
        }

        void setSelected(const std::string& item) {
            if (std::find(options.begin(), options.end(), item) != options.end()) {
                selected = item;
                repaint();
            }
        }

    private:
        std::vector<std::string> options;
        std::string selected;
        std::function<void(const std::string&)> onSelect;
        pptk::SafePointer<pptk::PopupListComponent> popupMenu;
    };
} // namespace pptk
