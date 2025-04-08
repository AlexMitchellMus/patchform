#pragma once

#include <UI_ToolKit/ComponentViewport.h>
#include <UI_ToolKit/PopupComponent.h>
#include "json.hpp"

#include "ObjectMenuLists.h"

class Object;
class Canvas;
class ToolDock;
class Item;

using json = nlohmann::json;

class Item : public pptk::Component {
public:
    Item(ObjectMenuDefs::ObjectDef def);
    void render(NVGcontext* vg) override;
    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override;
    void mouseEnter(pptk::CompEvent& e) override;
    void mouseLeave(pptk::CompEvent& e) override;
    void mouseButtonUp(pptk::CompEvent& e) override;

    std::function<void(pptk::Point, std::string, pptk::Point)> onMouseDrag = [](pptk::Point, std::string, pptk::Point) {};
    std::function<void(pptk::Point)> onMouseUp = [](pptk::Point) {};

    bool isInvalid() const { return definition.empty(); }
    json getObjectDefinition() const { return json::parse(definition); }

private:
    bool hovered = false;
    bool useIcon = true;

    std::string definition;
    std::string icon;
    std::string name;
    NVGcolor tint;

    NVGcolor bg = nvgRGB(46, 46, 46);
    NVGcolor highlight = nvgRGB(38, 38, 38);
    NVGcolor outline = nvgRGB(53, 53, 53);
};

class ObjectMenuList : public pptk::Component {
public:
    ObjectMenuList(Canvas* canvas, ToolDock* toolDock);
    ~ObjectMenuList() override = default;

private:
    void render(NVGcontext* vg) override;

    Canvas* cnv;
    ToolDock* td;
    pptk::SafePointer<Object> dndObject;
    std::vector<std::unique_ptr<Item>> items;

    struct CategoryHeader {
        const char* name;
        pptk::Point position;
        NVGcolor tint;
    };

    std::vector<CategoryHeader> categoryHeaders;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);
};

class ObjectMenuView : public pptk::ComponentViewport {
public:
    ObjectMenuView(Canvas* canvas, ToolDock* toolDock);
};

class ObjectMenu : public pptk::PopupComponent {
public:
    ObjectMenu(Canvas* canvas, ToolDock* toolDock);

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, 0, 0, width, height, bg, outline, 6.0f);
    }

    void resized() override
    {
        viewport->setBounds(0, 0, width, height);
    }

private:
    Canvas* cnv;
    ToolDock* td;
    std::unique_ptr<ObjectMenuView> viewport;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(55, 55, 55);
};
