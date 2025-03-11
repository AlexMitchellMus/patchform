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

#include "../Glad/gl.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

class ObjectItems;
class Canvas;
class LeftPanel : public pptk::ResizableComponent
{
public:
    explicit LeftPanel(Canvas* canvas);

    void keyPressed(pptk::CompEvent& e) override;

    void updateCanvasObjectList();

    void render(NVGcontext* nvg) override;

    void resized() override;

private:
    pptk::SafePointer<Canvas> cnv;
    std::vector<std::unique_ptr<ObjectItems>> objectListItems;
};
