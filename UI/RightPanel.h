/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"

using namespace pptk;

class RightPanel : public Component
{
public:
    RightPanel() = default;

    void render(NVGcontext* nvg) override
    {
        nvgFillColor(nvg, nvgRGB(33, 33, 33));
        nvgFillRect(nvg, 0, 0, width, height);

        // Vertical edge line (on left)
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0.5f, 0);
        nvgLineTo(nvg, 0.5f, height);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }
};