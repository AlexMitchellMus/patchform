//
// Created by alexw on 30/01/2025.
//

#pragma once

#include "../UI_ToolKit/PopupComponent.h"

#include "json.hpp"
#include "../UI_ToolKit/CompEvent.h"
using json = nlohmann::json;

class Object;
class Canvas;

struct ObjectDef
{
    json definition;
    std::string_view icon;
    bool useIcon = true;
};

class Item : public pptk::Component
{
    public:

    std::function<void(pptk::Point, std::string, pptk::Point)> onMouseDrag = [](pptk::Point, std::string, pptk::Point){};
    std::function<void(pptk::Point)> onMouseUp = [](pptk::Point){};


    Item(ObjectDef def) : definition(def.definition), icon(def.icon)
    {
        name = !definition.empty() ? definition.value<std::string>("obj", "empty") : "empty";
        useIcon = def.useIcon;
    };

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
    {
        onMouseDrag(position, name, getPositionInParent());
    }

    void mouseEnter(pptk::CompEvent& e) override
    {
        hovered = true;
        repaint();
    }

    void mouseLeave(pptk::CompEvent& e) override
    {
        hovered = false;
        repaint();
    }

    void mouseButtonUp(pptk::CompEvent& e) override
    {
        onMouseUp(pptk::Point(e.sdlEvent.button.x, e.sdlEvent.button.y));
    }

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        auto col = hovered ? highlight : bg ;
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), col, col, 6.0f);

        nvgFontSize(vg, useIcon ? 34.0f : 18.0f);
        nvgFontFace(vg, useIcon ? "object_icons" : "Regular");
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

    bool useIcon = true;

    json definition;
    std::string icon;

    NVGcolor bg = nvgRGB(46, 46, 46);
    NVGcolor highlight = nvgRGB(38, 38, 38);
    NVGcolor outline = nvgRGB(53, 53, 53);
};

class ToolDock;

class ObjectMenu : public pptk::PopupComponent {
public:
    ObjectMenu(Canvas* canvas, ToolDock* toolDock);

    ~ObjectMenu() override;

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

    pptk::SafePointer<Object> dndObject;
    std::vector<std::unique_ptr<Item>> items;
};
