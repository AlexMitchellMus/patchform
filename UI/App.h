/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/RootComponent.h"

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

    TopBar()
    {
        mainMenu = std::make_unique<ToggleButton>("A", "A");
        addComponent(mainMenu.get());

        undo = std::make_unique<ToggleButton>("B", "B");
        addComponent(undo.get());

        redo = std::make_unique<ToggleButton>("C", "C");
        addComponent(redo.get());

        hideSidePanelsToggle = std::make_unique<ToggleButton>("D", "D");
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

class RightPanel : public Component
{
public:
    RightPanel() = default;

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, nvgRGB(33, 33, 33));
        nvgFillRect(nvg, 0, 0, width, height);

        // Vertical edge line (on left)
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0.5f, 0);
        nvgLineTo(nvg, 0.5f, height);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }
};

class App : public ComponentRegister {
public:
    App() {

        canvas = std::make_unique<Canvas>();
        addComponent(canvas.get());

        topBar = std::make_unique<TopBar>();
        addComponent(topBar.get());

        toolDock = std::make_unique<ToolDock>();
        addComponent(toolDock.get());

        leftPanel = std::make_unique<LeftPanel>(canvas.get());
        addComponent(leftPanel.get());

        rightPanel = std::make_unique<RightPanel>();
        addComponent(rightPanel.get());

        topBar->hideShowPanels = [this](bool state)
        {
            leftPanel->setVisible(!state);
            rightPanel->setVisible(!state);

//#define AUTO_HIDE_DOCK
#ifdef AUTO_HIDE_DOCK
            if (state)
            {
                resizeToolDock(true);
                registerTimer([this]()
                {
                    resizeToolDock(false);
                });
            } else
            {
                unregisterTimerCallback(this);
                resizeToolDock(true);
            }
#endif
        };

        App::resized();
    }

    void mouseMove(const Point& position) override
    {
        // FIXME: Mouse move is not registering ATM, we need to add a listener or think about a better solution
        if (position.y > getHeight() - 100)
        {
            unregisterTimerCallback(this);
            resizeToolDock(true);
        }
    }

    void resizeToolDock(bool reset)
    {
        int toolDockWidth = 300;

        toolDockPosY = reset ? (getHeight() - 60) : toolDockPosY + 0.5f;

        float toolDockOffset = (getWidth() / 2.0f) - (toolDockWidth / 2.0f);
        toolDock->setBounds(toolDockOffset, toolDockPosY, toolDockWidth, 45);
    }

    void resized() override
    {
        topBar->setBounds(0, 0, getWidth(), 45);
        canvas->setBounds(-canvas->canvasOrigin, - canvas->canvasOrigin + 45, canvas->infinteCanvasSize, canvas->infinteCanvasSize);
        leftPanel->setBounds(0, 45, 200, getHeight() - 45);

        resizeToolDock(true);

        rightPanel->setBounds(getWidth() - 200, 45, 200, getHeight() - 45);
    }

    std::unique_ptr<Canvas> canvas;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<ToolDock> toolDock;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<RightPanel> rightPanel;

    float toolDockAnimator = 1.0f;
    bool animateToolDock = false;

    float toolDockPosY;
};