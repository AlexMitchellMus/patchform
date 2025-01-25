/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/ComponentRegister.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "../Nodes/AudioPort.h"

#include "Canvas.h"
#include "ToolDock.h"
#include "LeftPanel.h"
#include "Connection.h"

#include "../UI_ToolKit/ToggleButton.h"

using namespace pptk;

class TopBar : public Component {
public:
    std::function<void(bool)> hideShowPanels = [](bool){};

    TopBar(Component* parent) : Component(parent)
    {
        mainMenu = std::make_unique<ToggleButton>(this, "A", "A");
        addComponent(mainMenu.get());

        undo = std::make_unique<ToggleButton>(this, "B", "B");
        addComponent(undo.get());

        redo = std::make_unique<ToggleButton>(this, "C", "C");
        addComponent(redo.get());

        hideSidePanelsToggle = std::make_unique<ToggleButton>(this, "D", "D");
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
        int offset = 10;
        mainMenu->setBounds(offset, centreY, 35, 35);
        offset += 50;

        undo->setBounds(offset, centreY, 35, 35);
        offset += 50;
        redo->setBounds(offset, centreY, 35, 35);
        offset += 50;

        hideSidePanelsToggle->setBounds(getWidth() - 45, centreY, 35, 35);

    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(43, 43, 43));
        nvgFillRect(nvg, x, y, width, height);

        // Horizontal line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x, height - 0.5f);
        nvgLineTo(nvg, x + width, height - 0.5f);
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

class RightPanel : public Component
{
public:
    RightPanel(Component* parent) : Component(parent)
    {
    }

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, nvgRGB(33, 33, 33));
        nvgFillRect(nvg, x, y, width, height);

        // Vertical edge line (on left)
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x + 0.5f, y);
        nvgLineTo(nvg, x + 0.5f, y + height);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }
};

class App : public ComponentRegister {
public:
    App() {

        canvas = std::make_unique<Canvas>(this);
        addComponent(canvas.get());

        topBar = std::make_unique<TopBar>(this);
        addComponent(topBar.get());

        toolDock = std::make_unique<ToolDock>(this);
        addComponent(toolDock.get());

        leftPanel = std::make_unique<LeftPanel>(this, canvas.get());
        addComponent(leftPanel.get());

        rightPanel = std::make_unique<RightPanel>(this);
        addComponent(rightPanel.get());

        topBar->hideShowPanels = [this](bool state)
        {
            leftPanel->setVisible(!state);
            rightPanel->setVisible(!state);
        };

        App::resized();
    }

    void resized() override
    {
        topBar->setBounds(0, 0, getWidth(), 45);
        canvas->setBounds(0, 45, getWidth(), getHeight() - 45);
        leftPanel->setBounds(0, 45, 200, getWidth() - 45);

        int toolDockWidth = 400;
        float toolDockOffset = (canvas->getWidth() / 2.0f) - (toolDockWidth / 2.0f);
        toolDock->setBounds(toolDockOffset, getHeight() - 60, toolDockWidth, 50);

        rightPanel->setBounds(getWidth() - 200, 45, 200, getHeight() - 45);
    }

    std::unique_ptr<Canvas> canvas;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<ToolDock> toolDock;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<RightPanel> rightPanel;
};