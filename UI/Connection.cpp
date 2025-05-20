/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include "../UI_ToolKit/Component.h"

#include "Port.h"
#include "Connection.h"
#include "Object.h"

Connection::Connection(Port* port, Port* dest, uint64_t connEdgeHash) : originPort(port), destPort(dest), connectionBeingCreated(!dest), edgeHash(connEdgeHash)
{
    auto centre = port->getWidth() / 2;

    destPos = { centre, centre };

    if (dest && originPort->isSignal() && destPort->isSignal())
        cableType = CableType::Signal;

    auto cnv = port->findParentOfClass<Canvas>();
    auto inputPortPos = originPort->getPositionInParent(cnv);

    setPosition(inputPortPos);
}

Connection::~Connection()
{
    repaint();
}

void Connection::computeTileCoverage(TileMask& tileMaskBuffer)
{
    const auto tilesX = tileMaskBuffer.getX();
    const auto tilesY = tileMaskBuffer.getY();

    if (tileBits.getX() != tilesX || tileBits.getY() != tilesY)
        tileBits.resize(tilesX, tilesY);
    else
        tileBits.currentTileMask.clear();

    auto& cur = tileBits.currentTileMask;
    const auto tileSize = TileMask::tileSize;
    const auto globalPos = localToGlobal(0, 0);
    const float offsetX = globalPos.x;
    const float offsetY = globalPos.y;
    const float scale = getAccumulatedScale();

    constexpr int segments = 32;
    const float halfThickness = 2.0f * scale;


    pptk::Point prevPt;

    for (int i = 0; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / segments;
        float u = 1.0f - t;
        float tt = t * t, uu = u * u;
        float uuu = uu * u, ttt = tt * t;

        pptk::Point pt = {
            uuu * startPoint.x + 3 * uu * t * controlPoint2.x + 3 * u * tt * controlPoint1.x + ttt * endPoint.x,
            uuu * startPoint.y + 3 * uu * t * controlPoint2.y + 3 * u * tt * controlPoint1.y + ttt * endPoint.y
        };

        pt.x = offsetX + pt.x * scale;
        pt.y = offsetY + pt.y * scale;

        if (i > 0)
        {
            float minXf = std::min(pt.x, prevPt.x) - halfThickness;
            float maxXf = std::max(pt.x, prevPt.x) + halfThickness;
            float minYf = std::min(pt.y, prevPt.y) - halfThickness;
            float maxYf = std::max(pt.y, prevPt.y) + halfThickness;

            int minX = std::max(0, static_cast<int>(minXf / tileSize));
            int maxX = std::min(tilesX - 1, static_cast<int>(maxXf / tileSize));
            int minY = std::max(0, static_cast<int>(minYf / tileSize));
            int maxY = std::min(tilesY - 1, static_cast<int>(maxYf / tileSize));

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    int index = y * tilesX + x;
                    cur.setIndex(index);
                }
            }
        }
        prevPt = pt;
    }

    // Calculate the tile region covering the ball at the end of the curve
    const float ballRadius = 6.0f * scale;
    const float ballMinX = prevPt.x - ballRadius;
    const float ballMaxX = prevPt.x + ballRadius;
    const float ballMinY = prevPt.y - ballRadius;
    const float ballMaxY = prevPt.y + ballRadius;

    int ballTileMinX = std::max(0, static_cast<int>(ballMinX / tileSize));
    int ballTileMaxX = std::min(tilesX - 1, static_cast<int>(ballMaxX / tileSize));
    int ballTileMinY = std::max(0, static_cast<int>(ballMinY / tileSize));
    int ballTileMaxY = std::min(tilesY - 1, static_cast<int>(ballMaxY / tileSize));

    for (int y = ballTileMinY; y <= ballTileMaxY; ++y)
    {
        for (int x = ballTileMinX; x <= ballTileMaxX; ++x)
            cur.setIndex(y * tilesX + x);
    }

    auto& current = tileBits.currentTileMask.raw();
    auto& previous = tileBits.previousTileMask.raw();
    auto& merged = tileBits.mergedTileMask.raw();
    auto& out = tileMaskBuffer.raw();

    assert(current.size() == previous.size() && current.size() == merged.size() && current.size() == out.size());

    for (size_t i = 0; i < current.size(); ++i)
    {
        merged[i] = current[i] | previous[i];
        out[i] |= merged[i];
        previous[i] = current[i];
    }
}


float Connection::pointToSegmentDistance(const pptk::Point& p,
                             const pptk::Point& a,
                             const pptk::Point& b)
{
    float abX = b.x - a.x, abY = b.y - a.y;
    float apX = p.x - a.x, apY = p.y - a.y;

    float abLengthSq = abX * abX + abY * abY; // Avoid computing sqrt
    if (abLengthSq == 0.0f) return std::hypot(apX, apY); // A and B are the same point

    float t = (apX * abX + apY * abY) / abLengthSq;
    t = std::clamp(t, 0.0f, 1.0f); // Clamping

    // Compute closest point coordinates
    float closestX = a.x + t * abX;
    float closestY = a.y + t * abY;

    return std::hypot(p.x - closestX, p.y - closestY); // Compute Euclidean distance
}

bool Connection::isPointNearBezier(const pptk::Point& p,
                                   const pptk::Point& start,
                                   const pptk::Point& c1,
                                   const pptk::Point& c2,
                                   const pptk::Point& end,
                                   float threshold)
{
    // Compute bounding box with padding
    const float padding = std::max(5.0f, threshold * 0.5f);
    float minX = std::min({start.x, c1.x, c2.x, end.x}) - padding;
    float maxX = std::max({start.x, c1.x, c2.x, end.x}) + padding;
    float minY = std::min({start.y, c1.y, c2.y, end.y}) - padding;
    float maxY = std::max({start.y, c1.y, c2.y, end.y}) + padding;

    // Quick bounding box test
    if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY)
    {
        return false;
    }

    // Check if control points are nearly on the line segment using point-to-line distance
    float lineDist1 = pointToSegmentDistance(c1, start, end);
    float lineDist2 = pointToSegmentDistance(c2, start, end);

    if (lineDist1 < threshold && lineDist2 < threshold)
    {
        return pointToSegmentDistance(p, start, end) < threshold;
    }

    // Estimate curve length (approximation using control points)
    float lengthEstimate =
        std::hypot(c1.x - start.x, c1.y - start.y) +
        std::hypot(c2.x - c1.x, c2.y - c1.y) +
        std::hypot(end.x - c2.x, end.y - c2.y);

    // Determine segment count dynamically
    int segments = std::max(5, static_cast<int>(lengthEstimate / 100.0f));

    // Fall back to full Bézier hit test
    pptk::Point prevPoint = start;
    float invSegments = 1.0f / segments;

    for (int i = 1; i <= segments; ++i)
    {
        float t = i * invSegments;
        float u = 1.0f - t;

        // Compute cubic Bézier point
        float tt = t * t, uu = u * u;
        float uuu = uu * u, ttt = tt * t;

        pptk::Point bezierPoint = {
            uuu * start.x + 3 * uu * t * c2.x + 3 * u * tt * c1.x + ttt * end.x,
            uuu * start.y + 3 * uu * t * c2.y + 3 * u * tt * c1.y + ttt * end.y
        };

        if (pointToSegmentDistance(p, prevPoint, bezierPoint) < threshold)
            return true;

        prevPoint = bezierPoint;
    }

    return false;
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
            auto outputPortPos = destPort->getPositionInParent(cnv) + pptk::Point(centre, centre);
            destPos = outputPortPos - inputPortPos;
        }

        setPosition(inputPortPos);

        // Store final points
        auto centre = originPort->getWidth() / 2;
        startPoint = { centre, centre }; // Offset the start position slightly
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
    repaint();
}

void Connection::setConnectionDest(const pptk::Point& globalPos)
{
    destPos = globalToLocal(globalPos.x, globalPos.y);
    setSize(abs(destPos.x), abs(destPos.y));
    updateConnectionGeometry();
}

bool Connection::hitTest(float px, float py)
{
    constexpr float exclusionSize = 5.0f;

    // Define start and end exclusion rectangles
    // We use this so the connection does not block the port mouse interaction
    pptk::Point startMin = { startPoint.x - exclusionSize, startPoint.y - exclusionSize };
    pptk::Point startMax = { startPoint.x + exclusionSize, startPoint.y + exclusionSize };

    pptk::Point endMin = { endPoint.x - exclusionSize, endPoint.y - exclusionSize };
    pptk::Point endMax = { endPoint.x + exclusionSize, endPoint.y + exclusionSize };

    // If mouse is inside start or end exclusion zones, return false
    if ((px >= startMin.x && px <= startMax.x && py >= startMin.y && py <= startMax.y) ||
        (px >= endMin.x && px <= endMax.x && py >= endMin.y && py <= endMax.y))
    {
        return false;
    }

    return isPointNearBezier(pptk::Point(px, py), startPoint, controlPoint1, controlPoint2, endPoint);
}

void Connection::mouseEnter(pptk::CompEvent& e)
{
    if (!isHovered)
    {
        isHovered = true;
        repaint();
    }
}

void Connection::mouseLeave(pptk::CompEvent& e)
{
    if (isHovered)
    {
        isHovered = false;
        repaint();
    }
}

void Connection::mouseButtonDown(pptk::CompEvent& e)
{
    findParentOfClass<Canvas>()->setSelected(this);
}

void Connection::keyPressed(pptk::CompEvent& e)
{
    if (e.sdlEvent.key.key == SDLK_DELETE || e.sdlEvent.key.key == SDLK_BACKSPACE)
    {
        if (auto cnv = findParentOfClass<Canvas>())
            cnv->deleteSelectedObjects();
    }
}

void Connection::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    nvgSave(nvg);

    nvgBeginPath(nvg);
    nvgMoveTo(nvg, endPoint.x, endPoint.y);

    const auto straightCon = false;

    if (straightCon)
        nvgLineTo(nvg, startPoint.x, startPoint.y);
    else
        nvgBezierTo(nvg, controlPoint1.x, controlPoint1.y, controlPoint2.x, controlPoint2.y, startPoint.x, startPoint.y);

    const auto bgCol = nvgRGBA(30, 30, 30, 180);

    nvgStrokeWidth(nvg, 6.0f);   // Set line width
    if (connectionBeingCreated)
        nvgStrokePaint(nvg, nvgDoubleStroke(nvg, theme.app.general_accent, bgCol, bgCol, 0, false, false, 0.0f));
    else if (cableType == CableType::Signal)
        nvgStrokePaint(nvg, nvgDoubleStroke(nvg, isHovered || getIsSelected() ? theme.app.general_accent : conCol, bgCol, bgCol, 0, false, false, 0.0f));
    else
        nvgStrokePaint(nvg, nvgDoubleStroke(nvg, bgCol, bgCol, isHovered || getIsSelected() ? theme.app.general_accent : conCol, 3, false, false, 0.0f));
    nvgStroke(nvg);

//#define DEBUG_PATH_HIT_TEST
#ifdef DEBUG_PATH_HIT_TEST
    for (int i = 0; i <= 30; ++i) {
        float t = i / static_cast<float>(30);
        float u = 1.0f - t;

        float bx = u * u * u * startPoint.x + 3 * u * u * t * controlPoint2.x + 3 * u * t * t * controlPoint1.x + t * t * t * endPoint.x;
        float by = u * u * u * startPoint.y + 3 * u * u * t * controlPoint2.y + 3 * u * t * t * controlPoint1.y + t * t * t * endPoint.y;

        // Draw small circles along the computed Bezier curve
        nvgBeginPath(nvg);
        nvgCircle(nvg, bx, by, 2.0f);
        nvgFillColor(nvg, nvgRGB(255, 0, 0)); // Red: computed Bezier points
        nvgFill(nvg);
    }

#endif

//#define DEBUG_CONTROL_POINTS
#ifdef DEBUG_CONTROL_POINTS
    nvgBeginPath(nvg);
    nvgCircle(nvg, startPoint.x, startPoint.y, 5);
    nvgFillColor(nvg, nvgRGB(255, 255, 0)); // Yellow: Start
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, controlPoint1.x, controlPoint1.y, 5);
    nvgFillColor(nvg, nvgRGB(255, 0, 0)); // Red: Control Point 1
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, controlPoint2.x, controlPoint2.y, 5);
    nvgFillColor(nvg, nvgRGB(0, 255, 0)); // Green: Control Point 2
    nvgFill(nvg);

    nvgBeginPath(nvg);
    nvgCircle(nvg, endPoint.x, endPoint.y, 5);
    nvgFillColor(nvg, nvgRGB(0, 0, 255)); // Blue: End
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