#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../UI_ToolKit/Component.h"
#include "Canvas.h"

class LeftPanel : public pptk::Component
{
public:
    LeftPanel(Component* parent, Canvas* canvas);

    void updateCanvasObjectList();

    void render(NVGcontext* nvg) override;

    void mouseMove(const pptk::Point& position) override;

    void mouseLeave(SDL_Event& e) override;

    void mouseButtonDown(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta) override;

private:
    Canvas* cnv;
    std::vector<std::tuple<std::string, bool>> objectList;
    bool isResizingPanel = false;
};