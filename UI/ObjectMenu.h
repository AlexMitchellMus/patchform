//
// Created by alexw on 30/01/2025.
//

#pragma once

#include "../UI_ToolKit/PopupComponent.h"

#include "json.hpp"
using json = nlohmann::json;

using namespace pptk;

class Object;
class Canvas;

struct ObjectDef
{
    json definition;
    std::string_view icon;
};

class Item : public pptk::Component
{
    public:

    std::function<void(Point, std::string, Point)> onMouseDrag = [](Point, std::string, Point){};
    std::function<void(Point)> onMouseUp = [](Point){};


    Item(ObjectDef def) : definition(def.definition), icon(def.icon)
    {
        name = !definition.empty() ? definition.value<std::string>("obj", "empty") : "empty";
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
        onMouseUp(Point(e.button.x, e.button.y));
    }

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        auto col = hovered ? highlight : bg ;
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), col, col, 6.0f);

        nvgFontSize(vg, 34.0f);
        nvgFontFace(vg, "object_icons");
        nvgTextAlign(vg, NVG_ALIGN_CENTER);
        nvgFillColor(vg, nvgRGB(220, 220, 220)); // Text color
        nvgText(vg, getWidth() / 2, 32, icon.c_str(), nullptr);
    }

    json getObjectDefinition()
    {
        return definition;
    }

private:
    bool hovered = false;

    json definition;
    std::string name;
    std::string icon;

    NVGcolor bg = nvgRGB(46, 46, 46);
    NVGcolor highlight = nvgRGB(38, 38, 38);
    NVGcolor outline = nvgRGB(53, 53, 53);
};

class ToolDock;

class ObjectMenu : public pptk::PopupComponent {
public:
    ObjectMenu(Canvas* canvas, ToolDock* toolDock);

    ~ObjectMenu();

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

private:
    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);

    Canvas* cnv;
    ToolDock* td;

    SafePointer<Object> dndObject;
    std::vector<std::unique_ptr<Item>> items;
};
