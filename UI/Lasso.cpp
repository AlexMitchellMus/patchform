/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Lasso.h"

Lasso::Lasso(Component* parent)
    : Component(parent), active(false)
{
}

void Lasso::start(const pptk::Point& startPoint)
{
    this->startPoint = startPoint;
    this->endPoint = startPoint;
    active = true;
}

void Lasso::end()
{
    active = false;
}

void Lasso::update(const pptk::Point& currentPoint)
{
    if (active)
    {
        endPoint = currentPoint;
    }
}

std::pair<pptk::Point, pptk::Point> Lasso::finish()
{
    active = false;
    return {startPoint, endPoint};
}

void Lasso::render(NVGcontext* nvg)
{
    if (active)
    {
        float x = std::min(startPoint.x, endPoint.x);
        float y = std::min(startPoint.y, endPoint.y);
        float w = std::abs(endPoint.x - startPoint.x);
        float h = std::abs(endPoint.y - startPoint.y);

        auto outerCol = nvgRGB(28, 73, 119);
        auto innerCol = outerCol;
        innerCol.a *= 0.05f;

        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, x, y, w, h, innerCol, outerCol, 0.0f);
    }
}

pptk::Rect Lasso::getLassoBounds() const
{
    float left = std::min(startPoint.x, endPoint.x);
    float top = std::min(startPoint.y, endPoint.y);
    float width = std::max(startPoint.x, endPoint.x) - left;
    float height = std::max(startPoint.y, endPoint.y) - top;

    return {left, top, width, height};
}

bool Lasso::isInside(const pptk::Point& objectPosition) const
{
    float x1 = std::min(startPoint.x, endPoint.x);
    float y1 = std::min(startPoint.y, endPoint.y);
    float x2 = std::max(startPoint.x, endPoint.x);
    float y2 = std::max(startPoint.y, endPoint.y);

    return objectPosition.x >= x1 && objectPosition.x <= x2 &&
           objectPosition.y >= y1 && objectPosition.y <= y2;
}