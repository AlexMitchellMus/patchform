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

    // FIXME: Horrible hack, we add the originPort position when first making the connection!
    dest = originPort->getAbsolutePosition() + pptk::Point(5,5) + originPort->findParentOfClass<Port>()->getAbsolutePosition();
}

void Connection::render(NVGcontext* nvg) {
    nvgBeginPath(nvg);

    // Calculate the global position of the origin port
    auto originPos = originPort->getAbsolutePosition() + pptk::Point(5, 5);
    // Convert dest into the same relative coordinate system
    auto relativeDest = dest - originPort->findParentOfClass<Port>()->getAbsolutePosition();

    Point start = relativeDest;
    Point end = originPos;

    Point control1;
    Point control2;

    if (originPort->isOutput())
        std::swap(end, start);

    nvgMoveTo(nvg, end.x, end.y);

    float xDifference = fabsf(start.x - end.x);
    float factor = fminf(xDifference / 100.0f, 1.0f);
    float controlOffset = abs(end.y - start.y) / 2 * factor;

    if (end.y < start.y) {
        // Destination is above the origin; create an S shape with transition to straight
        control1 = {
            end.x + (start.x - end.x) * 0.5f * (1 - factor),  // Bring closer to the center horizontally
            end.y - controlOffset                             // Hook above
        };

        control2 = {
            start.x - (start.x - end.x) * 0.5f * (1 - factor), // Bring closer to the center horizontally
            start.y + controlOffset                            // Hook below
        };
        nvgBezierTo(nvg, control1.x, control1.y, control2.x, control2.y, start.x, start.y);
    } else {
        // Destination is below the origin; create a downward curve with transition to straight
        control1 = {end.x, (end.y + start.y) / 2};
        control2 = {start.x, (end.y + start.y) / 2};
        nvgBezierTo(nvg, control1.x, control1.y, control2.x, control2.y, start.x, start.y);
    }

    // Move to the origin position
    //nvgLineTo(nvg, originPos.x, originPos.y);

    nvgStrokeColor(nvg, nvgRGB(100, 100, 100)); // Set stroke color
    nvgStrokeWidth(nvg, 6.0f);   // Set line width
    nvgStrokePaint(nvg, nvgDoubleStroke(nvg, nvgRGBA(120, 120, 120, 20), nvgRGBA(120, 120, 120, 20), nvgRGB(120, 120, 120), 3, false, false, 0.0f));
    nvgStroke(nvg);

//#define DEBUG_PATH
#ifdef DEBUG_PATH
    nvgBeginPath(nvg);
    nvgCircle(nvg, control1.x, control1.y, 5.0f);  // Circle at first control point
    nvgFillColor(nvg, nvgRGBA(255, 0, 0, 150));  // Red for control1
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, control2.x, control2.y, 5.0f);  // Circle at second control point
    nvgFillColor(nvg, nvgRGBA(0, 0, 255, 150));  // Blue for control2
    nvgFill(nvg);
#endif

    // Ball at the end of a new connection
    nvgBeginPath(nvg);
    nvgCircle(nvg, relativeDest.x, relativeDest.y, 5.0f);
    nvgFillColor(nvg, nvgRGBA(90, 90, 90, 100));
    nvgFill(nvg);
}