/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/RootComponent.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>

class Port : public pptk::Component {
public:
    enum class Direction {Input, Output};

    explicit Port(int portNum, Direction dir = Direction::Input)
        : portNum(portNum)
        , direction(dir) { };

    void mouseButtonDown(SDL_Event& e) override;

    void mouseButtonUp(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button) override;

    void mouseEnter(SDL_Event& e) override
    {
        isHovered = true;
        repaint();
    };

    void mouseLeave(SDL_Event& e) override
    {
        isHovered = false;
        repaint();
    };

    void render(NVGcontext* nvg) override;

    [[nodiscard]] bool isOutput() const { return direction == Direction::Output; };

    int getPortNum() const { return portNum; };

private:
    Direction direction;
    int portNum;

    pptk::SafePointer<Port> foundPort;

    bool isHovered = false;
    bool isHoveredFromCable = false;
};