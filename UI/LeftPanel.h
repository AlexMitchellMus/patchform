/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../UI_ToolKit/Component.h"
#include "Canvas.h"

class LeftPanel : public pptk::Component
{
public:
    LeftPanel(Canvas* canvas);

    void updateCanvasObjectList();

    void render(NVGcontext* nvg) override;

    void mouseMove(const pptk::Point& position) override;

    void mouseLeave(SDL_Event& e) override;

    void mouseButtonDown(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button) override;

private:
    Canvas* cnv;
    std::vector<std::tuple<std::string, bool>> objectList;
    bool isResizingPanel = false;
};