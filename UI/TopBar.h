/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/ToggleButton.h"

using namespace pptk;

class TopBar : public Component {
public:
    std::function<void(bool)> hideShowPanels = [](bool){};

    TopBar()
    {
        mainMenu = std::make_unique<ToggleButton>("A", "A");
        mainMenu->setName("MainMenu");
        mainMenu->onClick = [this]()
        {
            std::cout << "main menu clicked" << std::endl;
        };

        addComponent(mainMenu.get());

        undo = std::make_unique<ToggleButton>("B", "B");
        undo->setName("Undo");
        addComponent(undo.get());

        redo = std::make_unique<ToggleButton>("C", "C");
        redo->setName("Redo");
        addComponent(redo.get());

        hideSidePanelsToggle = std::make_unique<ToggleButton>("D", "D");
        hideSidePanelsToggle->setName("HidePanels");
        addComponent(hideSidePanelsToggle.get());

        hideSidePanelsToggle->onToggle = [this](const bool state)
        {
            hideShowPanels(state);
        };

        TopBar::resized();
    }

    void resized() override
    {
        auto centreY = (getHeight() / 2) - (35 / 2);
        int offset = 16;
        mainMenu->setBounds(offset, centreY, 35, 35);
        offset += 50;

        undo->setBounds(offset, centreY, 35, 35);
        offset += 50;
        redo->setBounds(offset, centreY, 35, 35);

        hideSidePanelsToggle->setBounds(getWidth() - 50, centreY, 35, 35);

    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(43, 43, 43));
        nvgFillRect(nvg, 0, 0, width, height);

        // Horizontal line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0, height - 0.5f);
        nvgLineTo(nvg, 0 + width, height - 0.5f);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }

    bool hitTest(float x, float y) const override {
        return getBounds().contains(x, y);
    }

    void mouseButtonDown(SDL_Event& e) override {
        if (e.button.button == SDL_BUTTON_LEFT) {
            isHit = true;
        }
    }

    void mouseButtonUp(SDL_Event& e) override {
        if (e.button.button == SDL_BUTTON_LEFT) {
            isHit = false;
        }
    }

private:
    bool isHit = false;

    std::unique_ptr<ToggleButton> mainMenu;

    std::unique_ptr<ToggleButton> undo;
    std::unique_ptr<ToggleButton> redo;

    std::unique_ptr<ToggleButton> hideSidePanelsToggle;
};
