#pragma once

#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/ComponentRegister.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "../Nodes/AudioPort.h"

#include "Canvas.h"
#include "LeftPanel.h"
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

        leftPanal = std::make_unique<LeftPanel>(this, canvas.get());
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