/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "LeftPanel.h"
#include "SDL3/SDL.h"
#include "nanovg.h"
#include "Object.h"
#include "Canvas.h"

LeftPanel::LeftPanel(Canvas* canvas) : cnv(canvas)
{
    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Right);

    if (cnv)
    {
        cnv->addObjectChangedListener([this]()
        {
            updateCanvasObjectList();
        });
    }

    updateCanvasObjectList();

    repaint();
}

void LeftPanel::updateCanvasObjectList()
{
    objectList.clear();

    if (cnv)
    {
        for (auto obj : cnv->getObjects())
        {
            objectList.emplace_back(obj->getName(), obj->getIsSelected());
        }
    }

    repaint();
}

void LeftPanel::resized()
{
    getResizer().setBounds(getBounds());
}

void LeftPanel::render(NVGcontext* nvg)
{
    auto selectedCol = nvgRGB(43, 43, 43);
    nvgFillColor(nvg, nvgRGB(33, 33, 33));
    nvgFillRect(nvg, 0, 0, width, height);

    // Draw the object list
    float textX = 24; // Padding from the left edge
    float textY = 40; // Starting Y position with padding from the top
    const float lineHeight = 30; // Line spacing

    nvgFontSize(nvg, 14.0f);
    nvgFontFace(nvg, "SemiBold");
    nvgTextAlign(nvg, NVG_ALIGN_LEFT);
    nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color

    nvgText(nvg, textX, textY, "Objects", nullptr);
    textY += 40;

    nvgFontFace(nvg, "Regular");

    for (const auto& [objectName, isSelected]: objectList)
    {
        if (isSelected)
            nvgDrawRoundedRect(nvg, textX - 10, textY - 18, width - 30, 26, selectedCol, selectedCol, 6.0f);

        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color
        nvgText(nvg, textX, textY, objectName.c_str(), nullptr);
        textY += lineHeight; // Move to the next line
    }

    // Vertical edge line
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, width - 0.5f, 0);
    nvgLineTo(nvg, width - 0.5f, height);
    nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
    nvgStrokeWidth(nvg, 1.0f);
    nvgStroke(nvg);
}
