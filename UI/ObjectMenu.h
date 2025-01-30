//
// Created by alexw on 30/01/2025.
//

#pragma once

#include "../UI_ToolKit/Component.h"

using namespace pptk;

class Object;
class Canvas;

class Item : public pptk::Component
{
    public:

    std::function<void(Point, std::string, Point)> onMouseDrag = [](Point, std::string, Point){};
    std::function<void()> onMouseUp = [](){};


    Item(std::string& name) : name(std::move(name))
    {
        bg = nvgRGB(48, 48, 48);
        highlight = nvgRGB(38, 38, 38);
        outline = nvgRGB(53, 53, 53);
        repaint();
    };

    void mouseDrag(const Point& position, const Point& delta, Button button) override
    {
        onMouseDrag(position, name, getPositionInParent());
    }

    void mouseEnter(SDL_Event& e) override
    {
        hovered = true;
        repaint();
    }

    void mouseLeave(SDL_Event& e) override
    {
        hovered = false;
        repaint();
    }

    void mouseButtonUp(SDL_Event& e) override
    {
        onMouseUp();
    }

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        auto col = hovered ? highlight : bg ;
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), col, col, 6.0f);

        nvgFontSize(vg, 24.0f);
        nvgFontFace(vg, "icons");
        nvgFillColor(vg, nvgRGB(220, 220, 220)); // Text color
        nvgText(vg, 5, 24, "G", nullptr);
    }

private:
    bool hovered = false;

    std::string name;

    NVGcolor bg;
    NVGcolor highlight;
    NVGcolor outline;
};

class ToolDock;

class ObjectMenu : public pptk::Component {
public:
    ObjectMenu(Canvas* canvas, ToolDock* toolDock);

    ~ObjectMenu();

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

private:
    NVGcolor bg;
    NVGcolor outline;

    Canvas* cnv;
    ToolDock* td;

    std::unique_ptr<Component> dndObject;
    std::vector<std::unique_ptr<Item>> items;
};
