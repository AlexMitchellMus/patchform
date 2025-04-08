#pragma once

#include <UI_ToolKit/ComponentViewport.h>
#include <UI_ToolKit/PopupComponent.h>
#include "json.hpp"

class Object;
class Canvas;
class ToolDock;
class Item;

using json = nlohmann::json;

struct ObjectDef {
    std::string_view definition;
    std::string_view icon;
    bool useIcon = true;
    std::string_view displayName;

    bool isEmpty() const { return definition.length() > 0; }
    json getObjectDefinition() const { return json::parse(definition); }
    std::string getDisplayName() const { return std::string(displayName); };
};

class Item : public pptk::Component {
public:
    Item(ObjectDef def);
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

    NVGcolor bg = nvgRGB(46, 46, 46);
    NVGcolor highlight = nvgRGB(38, 38, 38);
    NVGcolor outline = nvgRGB(53, 53, 53);
};

class ObjectMenuList : public pptk::Component {
public:
    ObjectMenuList(Canvas* canvas, ToolDock* toolDock);
    ~ObjectMenuList() override = default;

private:
    Canvas* cnv;
    ToolDock* td;
    pptk::SafePointer<Object> dndObject;
    std::vector<std::unique_ptr<Item>> items;

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
        nvgDrawRoundedRect(vg, 0, 0, width, height, bg, bg, 6.0f);
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
};
