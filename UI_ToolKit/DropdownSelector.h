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

        void updateOptions(const std::vector<std::string>& newOptions)
        {
            options = newOptions;

            if (!options.empty()) {
                if (std::find(options.begin(), options.end(), selected) == options.end()) {
                    selected = options[0]; // fallback to first item
                }
            }

            resized();
            repaint();
        }

        void setOnSelect(std::function<void(const std::string&)> cb) { onSelect = std::move(cb); }

        void render(NVGcontext* vg) override
        {
            //Background
            auto bgCol = nvgRGB(38, 38, 38);
            nvgDrawRoundedRect(vg, 0, 0, width, height, bgCol, bgCol, 3);

            nvgBeginPath(vg);
            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgText(vg, 10, getHeight() * 0.5f, displayText.c_str(), nullptr);

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

        void resized() override
        {
            auto* root = getRootComponent();
            if (!root)
                return;

            constexpr float chevronSize = 8.0f;
            constexpr float chevronPadding = 10.0f;
            constexpr float textLeftPadding = 10.0f;
            constexpr float textChevronBuffer = 8.0f;

            const std::string ellipsis = "…";
            float ellipsisWidth = root->getTextWidthForFont("Regular", 14.0f, ellipsis);

            // Total available space for visible text *excluding* ellipsis
            float available = getWidth()
                            - textLeftPadding
                            - chevronSize
                            - chevronPadding
                            - textChevronBuffer
                            - ellipsisWidth;

            if (root->getTextWidthForFont("Regular", 14.0f, selected) <= available + ellipsisWidth) {
                displayText = selected;
                return;
            }

            int low = 0;
            int high = static_cast<int>(selected.size());

            while (low < high) {
                int mid = (low + high) / 2;
                std::string test = selected.substr(0, mid);

                if (root->getTextWidthForFont("Regular", 14.0f, test) <= available)
                    low = mid + 1;
                else
                    high = mid;
            }

            displayText = selected.substr(0, std::max(0, low - 1)) + ellipsis;
        }

        void mouseButtonDown(pptk::CompEvent&) override
        {
            // Close existing menu
            if (popupMenu && popupMenu->isVisible())
            {
                popupMenu->close();
                popupMenu.reset();
                setPopupComponent(nullptr);
                return;
            }

            // Clear previous popup safely
            setPopupComponent(nullptr);

            // Create new popup
            auto popup = std::make_unique<PopupListComponent>(options);
            popup->setSelected(selected);
            auto* popupRaw = popup.get();

            popup->onItemSelected = [_this = makeSafePointer(this), this](const std::string& choice)
            {
                if (!_this)
                    return;

                selected = choice;
                resized();

                if (onSelect)
                    onSelect(choice);

                repaint();

                if (popupMenu)
                    popupMenu->close(); // this safely triggers destroy
            };

            setPopupComponent(std::move(popup));            // assigns and deletes old popup
            getRootComponent()->addComponent(popupRaw);     // now safe to add to tree
            popupMenu = SafePointer(popupRaw);        // now safe to track it

            auto pos = clampPopupPositionInsideWindow(popupRaw, this);
            popupRaw->setPosition(pos.x, pos.y);
            popupRaw->registerMouseListener(this);
        }

        void setSelected(const std::string& item) {
            if (std::find(options.begin(), options.end(), item) != options.end())
            {
                selected = item;
                resized();
                repaint();
            }
        }

    private:
        inline Point clampPopupPositionInsideWindow(Component* popup, Component* anchor, float offsetY = 0.0f)
        {
            auto root = anchor->getRootComponent();
            auto popupBounds = popup->getBounds();
            auto globalPos = anchor->localToGlobal(0, anchor->getHeight() + offsetY);
            auto rootBounds = root->getAbsoluteBounds().reduced(8);

            float x = globalPos.x;
            float y = globalPos.y;

            if (x + popupBounds.w > rootBounds.x + rootBounds.w)
                x = rootBounds.x + rootBounds.w - popupBounds.w;

            if (y + popupBounds.h > rootBounds.y + rootBounds.h)
                y = rootBounds.y + rootBounds.h - popupBounds.h;

            x = std::max(x, rootBounds.x);
            y = std::max(y, rootBounds.y);

            return { x, y };
        }

        std::vector<std::string> options;
        std::string selected;
        std::string displayText;
        std::function<void(const std::string&)> onSelect;
        SafePointer<PopupListComponent> popupMenu;
    };

} // namespace pptk
