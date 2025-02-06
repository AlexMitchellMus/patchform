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
#include "../UI_ToolKit/Resizer.h"

class Canvas;
class LeftPanel : public pptk::ResizableComponent
{
public:
    explicit LeftPanel(Canvas* canvas);

    void updateCanvasObjectList();

    void render(NVGcontext* nvg) override;

    void resized() override;

private:
    pptk::SafePointer<Canvas> cnv;
    std::vector<std::tuple<std::string, bool>> objectList;
};
