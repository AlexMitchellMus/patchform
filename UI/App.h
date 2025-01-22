#pragma once

#include "../UI_ToolKit/Component.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

#include "../Nodes/AudioPort.h"

#include "Canvas.h"
#include "Connection.h"

using namespace pptk;

class Port : public Component {
public:
    explicit Port(int portNum) : portNum(portNum) {
    }

    void mouseButtonDown(SDL_Event& e) override {
        auto con = std::make_unique<Connection>();
        connectionBeingMade = con.get();
        addComponent<Connection>(std::move(con));
        connections.push_back(std::move(con));
    }

    void mouseButtonUp(SDL_Event& e) override {
        //connections.clear();
        //connectionBeingMade.reset();
    }

    void mouseDrag(const Point& currentPosition, const Point& delta) override {
        if (connectionBeingMade) {
            connectionBeingMade->setConnectionDest(currentPosition);
        }
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgCircle(nvg, x + 5, y + 5, 5);
        nvgFillColor(nvg, nvgRGB(0, 0, 255)); // Blue fill for ports
        nvgFill(nvg);
    }

private:
    int portNum;
    std::vector<std::unique_ptr<Connection>> connections;
    Connection* connectionBeingMade;
};

class TopBar : public Component {
public:
    TopBar() = default;

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, isHit ? nvgRGB(255, 0, 0) : nvgRGB(25, 25, 25));
        nvgFillRect(nvg, x, y, width, height);
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

class App : public Component {
public:
    App() {
        auto canvas = addComponent(std::make_unique<Canvas>());
        canvas->setBounds(0, 0, 1920, 1080);

        auto topBar = addComponent(std::make_unique<TopBar>());
        topBar->setBounds(0, 0, 1920, 30);

    }
};