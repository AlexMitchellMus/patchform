#pragma once

#include "PopupComponent.h"
#include "CompEvent.h"
#include "nanovg.h"

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace pptk {

class PopupListComponent : public PopupComponent
{
public:

    PopupListComponent(const std::vector<std::string>& listItems) : labels(listItems)
    {
        for (const auto& label : labels)
        {
            auto item = std::make_unique<Item>(label, [this, label]() {
                if (onItemSelected)
                    onItemSelected(label);
            });

            addComponent(item.get());
            items.push_back(std::move(item));
        }
    }

    void resized() override
    {
        float maxTextWidth = 0.0f;
        const float padding = 20.0f;
        const float minWidth = 150.0f;
        const float maxWidth = 400.0f;

        for (const auto& label : labels)
        {
            float width = getTextWidthForFont("Regular", 14.0f, label);
            maxTextWidth = std::max(maxTextWidth, width);
        }

        float popupWidth = std::clamp(maxTextWidth + padding, minWidth, maxWidth);
        int y = 5; // Top margin of inner list

        for (auto& item : items)
        {
            item->setBounds(5, y, popupWidth - 10, 28);
            y += 30;
        }

        y += 4; // Bottom margin

        setSize(popupWidth, y);
    }

    std::function<void(const std::string&)> onItemSelected;

    class Item : public Component
    {
    public:
        Item(const std::string& label, std::function<void()> onClick)
            : name(label), onClickFn(std::move(onClick)) {}

        void render(NVGcontext* vg) override
        {
            if (isHovered)
            {
                nvgBeginPath(vg);
                nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), hover, hover, 4.0f);
            }

            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGB(220, 220, 220));
            nvgText(vg, 10, getHeight() * 0.5f, name.c_str(), nullptr);
        }

        // In your Item:
        void mouseButtonDown(CompEvent&) override
        {
            if (onClickFn)
                onClickFn(); // no longer destroys anything!
        }

        void mouseEnter(CompEvent&) override { isHovered = true; repaint(); }
        void mouseLeave(CompEvent&) override { isHovered = false; repaint(); }

    private:
        std::string name;
        std::function<void()> onClickFn;
        bool isHovered = false;
        NVGcolor hover = nvgRGB(53, 53, 53);
    };

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, -3, -3, getWidth() + 6, getHeight() + 6, shadow, shadow, 10);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 8);
    }

private:
    std::vector<std::string> labels;
    std::vector<std::unique_ptr<Item>> items;
    std::function<void(const std::string&)> onSelectCallback;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor shadow = nvgRGBA(0, 0, 0, 20);
};

} // namespace pptk
