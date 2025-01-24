/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_Toolkit/Component.h"

#include "App.h"
#include "Port.h"

class Canvas;
class Object : public pptk::Component {
public:
    explicit Object(Component* parent, const std::string& name);

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta) override;

    void mouseButtonDown(SDL_Event& e) override;

    void mouseEnter(SDL_Event& e) override
    {
        isHovered = true;
    }

    void mouseLeave(SDL_Event& e) override
    {
        isHovered = false;
    }

    void keyPressed(SDL_Event& e) override;

    void render(NVGcontext* nvg) override;

    std::string& getName() { return name; }

    bool getIsSelected() { return isSelected; }

private:

    void setSelected(bool shouldBeSelected)
    {
        if (isSelected != shouldBeSelected)
            isSelected = shouldBeSelected;
    }

    std::string name;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;

    bool isSelected = false;

    bool isHovered = false;

    friend class Canvas;

    bool multiSelected = false;
};

