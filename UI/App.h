#pragma once

#include "../UI_ToolKit/Component.h"
#include <memory>
#include <vector>
#include <string>
#include <iostream>

using namespace pptk;

class Connection : public Component {
public:
    Connection() : dest(x, y) {}

    void setConnectionDest(const Point& p) {
        std::cout << "Setting dest: " << p.toString() << std::endl;
        dest = p;
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x, y);
        auto gPos = getAbsolutePosition();
        nvgLineTo(nvg, dest.x - gPos.x, dest.y - gPos.y);

        nvgStrokeColor(nvg, nvgRGB(255, 255, 255)); // Set stroke color
        nvgStrokeWidth(nvg, 2.0f);   // Set line width
        nvgStroke(nvg);
    }

private:
    Point dest;
};

class Port : public Component {
public:
    explicit Port(int portNum) : portNum(portNum) {
        setBounds(portNum * 30, 0, 10, 10);
    }

    void mouseButtonDown(SDL_Event& e) override {
        auto con = std::make_unique<Connection>();
        connectionBeingMade = con.get();
        addComponent(con.get());
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

class Object : public Component {
public:
    explicit Object(const std::string& name) : name(name) {
        setBounds(0, 0, 120, 45);

        for (int i = 0; i < 4; ++i) {
            auto port = std::make_unique<Port>(i);
            addComponent(port.get());
            inPorts.push_back(std::move(port));
        }
    }

    void mouseDrag(const Point& currentPosition, const Point& delta) override {
        setPosition(getPosition() + delta);
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, x, y, width, height, nvgRGB(33, 33, 33), nvgRGB(5, 5, 5), 6.0f);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "sans");
        nvgFillColor(nvg, nvgRGB(255, 255, 255));
        nvgTextAlign(nvg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        nvgText(nvg, x + width / 2, y + height / 2, name.c_str(), nullptr);
    }

private:
    std::string name;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;
};

class Canvas : public Component {
public:
    Canvas() {
        for (int i = 0; i < 10; ++i) {
            auto obj = std::make_unique<Object>("obj_" + std::to_string(i));
            addComponent(obj.get());
            objects.push_back(std::move(obj));
        }
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(33, 33, 33));
        nvgFillRect(nvg, x, y, width, height);
    }

private:
    std::vector<std::unique_ptr<Object>> objects;
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
        topBar = std::make_unique<TopBar>();
        addComponent(topBar.get());
        topBar->setBounds(0, 0, 1920, 30);

        canvas = std::make_unique<Canvas>();
        addComponent(canvas.get());
        canvas->setBounds(0, 30, 1920, 1080 - 30);
    }

private:
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<Canvas> canvas;
};