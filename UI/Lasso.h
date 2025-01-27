/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"
#include <vector>
#include <utility>

class Lasso : public pptk::Component
{
public:
    Lasso();

    // Start the lasso at a specific position
    void start(const pptk::Point& startPoint);

    void end();

    // Update the lasso as the mouse moves
    void update(const pptk::Point& currentPoint);

    pptk::Rect getLassoBounds() const;

    // Finish the lasso and return the selection bounds
    std::pair<pptk::Point, pptk::Point> finish();

    // Render the lasso rectangle
    void render(NVGcontext* nvg) override;

    // Check if an object is inside the lasso bounds
    bool isInside(const pptk::Point& objectPosition) const;

private:
    pptk::Point startPoint;
    pptk::Point endPoint;
    bool active;
};