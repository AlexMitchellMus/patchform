#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/ComponentRegister.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "../Nodes/AudioPort.h"

#include "Canvas.h"
#include "Connection.h"

using namespace pptk;

class TopBar : public Component {
public:
    TopBar(Component* parent) : Component(parent) {}

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, isHit ? nvgRGB(255, 0, 0) : nvgRGB(43, 43, 43));
        nvgFillRect(nvg, x, y, width, height);

        // Horizontal line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x, height - 0.5f);
        nvgLineTo(nvg, x + width, height - 0.5f);
        nvgStrokeColor(nvg, nvgRGB(63, 63, 63));
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
};

class LeftPanel : public Component
{
public:
    LeftPanel(Component* parent) : Component(parent)
    {
        setMinMaxSize(200, 400, 0, 0);
    }

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, nvgRGB(43, 43, 43));
        nvgFillRect(nvg, x, y, width, height);

        // Vertical edge line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, width - 0.5f, y);
        nvgLineTo(nvg, width - 0.5f, y + height);
        nvgStrokeColor(nvg, nvgRGB(63, 63, 63));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }

    void mouseMove(const Point& position) override
    {
        if (position.x > getWidth() - 10 && position.x < getWidth())
        {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_W_RESIZE));
        }
        else
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    }

    void mouseLeave(SDL_Event& e) override
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    }

    void mouseButtonDown(SDL_Event& e) override
    {
        if (e.button.button == SDL_BUTTON_LEFT)
        {
            if (e.button.x > getWidth() - 10 && e.button.x < getWidth())
            {
                isResizingPanel = true;
            }
            else
                isResizingPanel = false;
        }
    }

    void mouseDrag(const Point& position, const Point& delta) override
    {
        auto currBounds = getBounds();
        setBounds(currBounds.x, currBounds.y, position.x, currBounds.h);
    }

    bool isResizingPanel = false;
};

class RightPanel : public Component
{
public:
    RightPanel(Component* parent) : Component(parent)
    {
    }

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, nvgRGB(43, 43, 43));
        nvgFillRect(nvg, x, y, width, height);
    }
};

class App : public ComponentRegister {
public:
    App() {

        canvas = std::make_unique<Canvas>(this);
        addComponent(canvas.get());
        canvas->setBounds(0, 45, 1920, 1080 - 45);

        topBar = std::make_unique<TopBar>(this);
        addComponent(topBar.get());
        topBar->setBounds(0, 0, 1920, 45);

        leftPanal = std::make_unique<LeftPanel>(this);
        addComponent(leftPanal.get());
        leftPanal->setBounds(0, 45, 200, 1080 - 45);

        rightPanel = std::make_unique<RightPanel>(this);
        addComponent(rightPanel.get());
        rightPanel->setBounds(1920 - 200, 45, 200, 1080 - 45);
    }
    std::unique_ptr<Canvas> canvas;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<LeftPanel> leftPanal;
    std::unique_ptr<RightPanel> rightPanel;
};