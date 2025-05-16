/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Lasso.h"

Lasso::Lasso(pptk::Point canvasPos)
    : startPoint(canvasPos)
    , endPoint(canvasPos)
{
    setPosition(canvasPos);
    setSize(0, 0);
}

Lasso::~Lasso()
{
}

void Lasso::update(const pptk::Point& currentPoint)
{
    endPoint = currentPoint; // Update endpoint for the lasso

    float x1 = std::min(startPoint.x, endPoint.x);
    float y1 = std::min(startPoint.y, endPoint.y);
    float x2 = std::max(startPoint.x, endPoint.x);
    float y2 = std::max(startPoint.y, endPoint.y);

    setPosition({x1, y1});
    setSize(x2 - x1, y2 - y1);
    repaint();
}

void Lasso::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    auto outerCol = nvgRGB(28, 73, 119);
    auto innerCol = outerCol;
    innerCol.a *= 0.1f;

    nvgBeginPath(nvg);
    nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), innerCol, outerCol, 0.0f);
}

pptk::Rect Lasso::getLassoBounds() const
{
    float x1 = std::min(startPoint.x, endPoint.x);
    float y1 = std::min(startPoint.y, endPoint.y);
    float width = std::abs(endPoint.x - startPoint.x);
    float height = std::abs(endPoint.y - startPoint.y);

    return {x1, y1, width, height};
}

bool Lasso::isInside(const pptk::Point& objectPosition) const
{
    pptk::Rect bounds = getLassoBounds();
    return objectPosition.x >= bounds.x && objectPosition.x <= bounds.x + bounds.w &&
           objectPosition.y >= bounds.y && objectPosition.y <= bounds.y + bounds.h;
}