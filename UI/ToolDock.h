/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"

#include "../UI_ToolKit/ToggleButton.h"

class ToolDock : public pptk::Component {
public:
    explicit ToolDock(Component* parent) : Component(parent)
    {
        editButton = std::make_unique<ToggleButton>(parent, "E", "F", "icons");
        addComponent(editButton.get());

        ToolDock::resized();
    };

    void resized() override
    {
        editButton->setBounds(x + 15, y + 7, 35, 35);
    }

    void render(NVGcontext* nvg) override
    {
        auto dropShadowCol = nvgRGBA(0, 0, 0, 30);
        auto dropShadowCornerRadius = (getHeight() + 6) / 2;

        auto bgCol = nvgRGB(33, 33, 33);
        auto outLineCol = nvgRGB(45, 45, 45);
        auto cornerRadius = getHeight() / 2;

        nvgDrawRoundedRect(nvg, x - 3, y - 3, width + 6, height + 6, dropShadowCol, dropShadowCol, dropShadowCornerRadius);
        nvgDrawRoundedRect(nvg, x, y, width, height, bgCol, outLineCol, cornerRadius);
    }

    std::unique_ptr<ToggleButton> editButton;
};
