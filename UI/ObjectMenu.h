//
// Created by alexw on 30/01/2025.
//

#pragma once

#include "../UI_ToolKit/Component.h"

using namespace pptk;

class Object;
class Canvas;

struct ObjectDef
{
    std::string name;
    std::string_view icon;
};

class Item : public pptk::Component
{
    public:

    std::function<void(Point, std::string, Point)> onMouseDrag = [](Point, std::string, Point){};
    std::function<void(Point)> onMouseUp = [](Point){};


    Item(ObjectDef def) : name(def.name), icon(def.icon)
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

    std::string& getObjectDefinition()
    {
        return name;
    }

private:
    bool hovered = false;

    std::string name;
    std::string icon;

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
        nvgDrawRoundedRect(vg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

private:
    NVGcolor bg;
    NVGcolor outline;
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);

    Canvas* cnv;
    ToolDock* td;

    std::unique_ptr<Object> dndObject;
    std::vector<std::unique_ptr<Item>> items;
};
