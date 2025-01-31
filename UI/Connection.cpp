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

Connection::Connection(Port* port, Port* dest) : originPort(port), destPort(dest), connectionBeingCreated(!dest)
{
    auto centre = port->getWidth() / 2;

    destPos = { centre, centre };
}

Connection::~Connection()
{
    repaint();
}

void Connection::updateConnectionGeometry()
{
    if (!originPort)
        return;

    if (auto cnv = findParentOfClass<Canvas>())
    {
        auto inputPortPos = originPort->getPositionInParent(cnv);

        if (destPort)
        {
            auto centre = destPort->getWidth() / 2;
            // Get the positions of the ports in the canvas
            auto outputPortPos = destPort->getPositionInParent(cnv) + Point(centre, centre);
            destPos = outputPortPos - inputPortPos;
        }

        setPosition(inputPortPos);

        // Store final points
        startPoint = {4.5f, 6.5f}; // Offset the start position slightly
        endPoint = destPos;

        if (!originPort->isOutput())
            std::swap(startPoint, endPoint);

        // Compute control points
        float xDifference = fabsf(startPoint.x - endPoint.x);
        float factor = fminf(xDifference / 100.0f, 1.0f);
        float controlOffset = abs(endPoint.y - startPoint.y) / 2 * factor;

        if (endPoint.y < startPoint.y)
        {
            // Destination is above the origin; create an S shape with transition to straight
            controlPoint1 = {
                endPoint.x + (startPoint.x - endPoint.x) * 0.5f * (1 - factor),
                // Bring closer to the center horizontally
                endPoint.y - controlOffset // Hook above
            };

            controlPoint2 = {
                startPoint.x - (startPoint.x - endPoint.x) * 0.5f * (1 - factor),
                // Bring closer to the center horizontally
                startPoint.y + controlOffset // Hook below
            };
        }
        else
        {
            // Destination is below the origin; create a downward curve with transition to straight
            controlPoint1 = {endPoint.x, (endPoint.y + startPoint.y) / 2};
            controlPoint2 = {startPoint.x, (endPoint.y + startPoint.y) / 2};
        }
    }
}

void Connection::setConnectionDest(const pptk::Point& p)
{
    destPos = p;
    setSize(abs(p.x), abs(p.y));
    updateConnectionGeometry();
}

bool Connection::hitTest(float px, float py) const
{
    std::cout << "hit testing connection: " <<  px << ", " << py << std::endl;
    return false;
}

void Connection::mouseEnter(SDL_Event& e)
{
    std::cout << "mouse enter connection: " << std::endl;
}

void Connection::mouseLeave(SDL_Event& e)
{
    std::cout << "mouse leave connection: " << std::endl;
}

void Connection::render(NVGcontext* nvg) {
    nvgSave(nvg);

    nvgBeginPath(nvg);

    nvgMoveTo(nvg, endPoint.x, endPoint.y);
    nvgBezierTo(nvg, controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, startPoint.x, startPoint.y);

    // Move to the origin position
    //nvgLineTo(nvg, originPos.x, originPos.y);

    nvgStrokeColor(nvg, nvgRGB(100, 100, 100)); // Set stroke color
    nvgStrokeWidth(nvg, 6.0f);   // Set line width
    nvgStrokePaint(nvg, nvgDoubleStroke(nvg, nvgRGBA(90, 90, 90, 30), nvgRGBA(90, 90, 90, 30), nvgRGB(90, 90, 90), 3, false, false, 0.0f));
    nvgStroke(nvg);

//#define DEBUG_PATH
#ifdef DEBUG_PATH1
    nvgBeginPath(nvg);
    nvgCircle(nvg, control1.x, control1.y, 5.0f);  // Circle at first control point
    nvgFillColor(nvg, nvgRGBA(255, 0, 0, 150));  // Red for control1
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, control2.x, control2.y, 5.0f);  // Circle at second control point
    nvgFillColor(nvg, nvgRGBA(0, 0, 255, 150));  // Blue for control2
    nvgFill(nvg);
#endif

//#define DEBUG_PATH_BOUNDING_BOX
#ifdef DEBUG_PATH_BOUNDING_BOX
    nvgBeginPath(nvg);
    nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(),nvgRGBA(0,0,0,0), nvgRGBA(255,0,0, 255), 0);
#endif

    // Ball at the end of a new connection
    if (connectionBeingCreated)
    {
        nvgBeginPath(nvg);
        nvgCircle(nvg, destPos.x, destPos.y, 5.0f);
        nvgFillColor(nvg, nvgRGBA(90, 90, 90, 100));
        nvgFill(nvg);
    }

    nvgRestore(nvg);
}