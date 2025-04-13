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

#include "../Glad/gl.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

class Port : public pptk::Component {
public:
    enum class Direction { Input, Output };
    enum class PortType { None, Audio, Event, ImEvent };

    explicit Port(int portNum, PortType type = PortType::Audio, Direction dir = Direction::Input)
        : portNum(portNum)
        , direction(dir)
        , portType(type)
    {};

    void mouseButtonDown(pptk::CompEvent& e) override;

    void mouseButtonUp(pptk::CompEvent& e) override;

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button) override;

    void mouseEnter(pptk::CompEvent& e) override
    {
        isHovered = true;
        repaint();
    };

    void mouseLeave(pptk::CompEvent& e) override
    {
        isHovered = false;
        repaint();
    };

    void render(NVGcontext* nvg) override;

    [[nodiscard]] bool isOutput() const { return direction == Direction::Output; };

    int getPortNum() const { return portNum; };

    bool isSignal() const { return portType == PortType::Audio; };

    void setCanvasMode(bool lockedMode)
    {
        canvasLocked = lockedMode;
        // This will be called from canvas, which will repaint everything
        // No need to repaint per port here
    }

    bool hitTest(float x, float y) override
    {
        // If the canvas is in locked mode we don't want the port to be interactive
        // But we still want to show it semi-transparent (maybe)
        if (canvasLocked)
            return false;

        return Component::hitTest(x, y);
    }

    void setHoveredFromCable(const bool shouldBeHovered)
    {
        if (isHoveredFromCable != shouldBeHovered)
        {
            isHoveredFromCable = shouldBeHovered;
            repaint();
        }
    }

private:
    Direction direction;
    int portNum;
    PortType portType;

    pptk::SafePointer<Port> foundPort;

    bool canvasLocked = false;

    bool isHovered = false;
    bool isHoveredFromCable = false;

    bool parentAddedToMultiConnect = false;
};