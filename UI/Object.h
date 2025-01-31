/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "CanvasItem.h"

#include "App.h"
#include "Port.h"

class Canvas;
class Object : public CanvasItem {
public:
    explicit Object(const std::string& name);

    ~Object();

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button) override;

    void mouseButtonDown(SDL_Event& e) override;

    void mouseEnter(SDL_Event& e) override
    {
        isHovered = true;
        repaint();
    }

    void mouseLeave(SDL_Event& e) override
    {
        isHovered = false;
        repaint();
    }

    void keyPressed(SDL_Event& e) override;

    void render(NVGcontext* nvg) override;

    [[nodiscard]] const std::string& getName() const { return name; }

    [[nodiscard]] uint8_t getNumInputs() const { return inPorts.size(); };

    [[nodiscard]] uint8_t getNumOutputs() const { return outPorts.size(); };

    std::string& getObjectDefinition()
    {
        return definition;
    };

private:

    std::string name;
    std::string definition;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;

    bool isHovered = false;

    friend class Canvas;

    bool multiSelected = false;
};

