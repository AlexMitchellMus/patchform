/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"

#include "../UI_ToolKit/ToggleButton.h"

class ZoomSlider : public pptk::Component
{
public:
    explicit ZoomSlider(Component* parent) : Component(parent)
    {
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color
        nvgText(nvg, x + 5, y + 24, "100 %", nullptr);
    }
};

class ToolDock : public pptk::Component {
public:
    explicit ToolDock(Component* parent) : Component(parent)
    {
        editButton = std::make_unique<ToggleButton>(parent, "E", "F", "icons");
        addComponent(editButton.get());

        addObjectButton = std::make_unique<ToggleButton>(parent, "G", "G", "icons");
        addComponent(addObjectButton.get());

        viewButton = std::make_unique<ToggleButton>(parent, "H", "H", "icons");
        addComponent(viewButton.get());

        resizeToFit = std::make_unique<ToggleButton>(parent, "I", "I", "icons");
        addComponent(resizeToFit.get());

        zoomSlider = std::make_unique<ZoomSlider>(parent);
        addComponent(zoomSlider.get());

        ToolDock::resized();
    };

    void resized() override
    {
        int offset = 15;
        editButton->setBounds(x + offset, y + 5, 35, 35);
        offset += 50;
        addObjectButton->setBounds(x + offset, y + 5, 35, 35);
        offset += 50;
        viewButton->setBounds(x + offset, y + 5, 35, 35);
        offset += 50;
        resizeToFit->setBounds(x + offset, y + 5, 35, 35);
        offset += 50;
        zoomSlider->setBounds(x + offset, y + 5, 35, 60);
    }

    void render(NVGcontext* nvg) override
    {
        auto dropShadowCol = nvgRGBA(0, 0, 0, 30);
        auto dropShadowCornerRadius = (getHeight() + 6) / 2;

        auto bgCol = nvgRGB(43, 43, 43);
        auto outLineCol = nvgRGB(53, 53, 53);
        auto cornerRadius = getHeight() / 2;

        nvgDrawRoundedRect(nvg, x - 3, y - 3, width + 6, height + 6, dropShadowCol, dropShadowCol, dropShadowCornerRadius);
        nvgDrawRoundedRect(nvg, x, y, width, height, bgCol, outLineCol, cornerRadius);
    }

    std::unique_ptr<ToggleButton> editButton;
    std::unique_ptr<ToggleButton> addObjectButton;
    std::unique_ptr<ToggleButton> viewButton;
    std::unique_ptr<ToggleButton> resizeToFit;
    std::unique_ptr<ZoomSlider> zoomSlider;
};
