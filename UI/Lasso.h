/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"

class Lasso : public pptk::Component
{
public:
    explicit Lasso(pptk::Point startPoint);

    ~Lasso() override;

    // Update the lasso as the mouse moves
    void update(const pptk::Point& currentPoint);

    // Stop lasso from blocking mouse events from clicking on the canvas
    // This can happen if multiple mouse buttons are clicked at the same time while dragging
    bool hitTest(float px, float py) override
    {
        return false;
    }

    pptk::Rect getLassoBounds() const;

    // Render the lasso rectangle
    void render(NVGcontext* nvg, const pptk::Theme& theme) override;

    // Check if an object is inside the lasso bounds
    bool isInside(const pptk::Point& objectPosition) const;

private:
    pptk::Point startPoint;
    pptk::Point endPoint;
};