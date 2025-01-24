/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include "../UI_ToolKit/Component.h"

#include "Connection.h"
#include "Port.h"
#include "Object.h"

Connection::Connection(Component* parent) : Component(parent)
{
    originPort = dynamic_cast<Port*>(parent);

    // FIXME: Horrible hack, we add the originPort postition when first making the connection!
    dest = originPort->getAbsolutePosition() + pptk::Point(5,5) + originPort->findParentOfClass<Port>()->getAbsolutePosition();
}

void Connection::render(NVGcontext* nvg) {
    nvgBeginPath(nvg);

    // Calculate the global position of the origin port
    auto originPos = originPort->getAbsolutePosition() + pptk::Point(5, 5);

    // Convert dest into the same relative coordinate system
    auto relativeDest = dest - originPort->findParentOfClass<Port>()->getAbsolutePosition();
    nvgMoveTo(nvg, relativeDest.x, relativeDest.y);

    // Move to the origin position
    nvgLineTo(nvg, originPos.x, originPos.y);

    nvgStrokeColor(nvg, nvgRGB(100, 100, 100)); // Set stroke color
    nvgStrokeWidth(nvg, 5.0f);   // Set line width
    nvgStrokePaint(nvg, nvgDoubleStroke(nvg, nvgRGBA(90,90,90,90), nvgRGBA(90,90,90,90), nvgRGB(120, 120, 120), 4, true, false, 0.0f));
    nvgStroke(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, relativeDest.x, relativeDest.y, 5.0f);
    nvgFillColor(nvg, nvgRGBA(90, 90, 90, 100));
    nvgFill(nvg);
}