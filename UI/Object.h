/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "CanvasItem.h"

#include "App.h"
#include "Port.h"

class AudioNode;
class Canvas;
class Object : public CanvasItem {
public:
    explicit Object(const AudioNode* node);

    ~Object();

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button) override;
    void mouseButtonDown(SDL_Event& e) override;
    void mouseEnter(SDL_Event& e) override;
    void mouseLeave(SDL_Event& e) override;
    void keyPressed(SDL_Event& e) override;

    void resized() override;

    void render(NVGcontext* nvg) override;

    [[nodiscard]] const std::string& getName() const { return name; }

    [[nodiscard]] uint8_t getNumInputs() const { return inPorts.size(); };

    [[nodiscard]] uint8_t getNumOutputs() const { return outPorts.size(); };

    json getObjectDefinition()
    {
        return definition;
    };

    bool getIsHovered() const { return isHovered; };
    bool getIsSelected() const { return isSelected; };

    void setObjectDefinition(json j)
    {
        definition = j;
    }

    int nodeID = -1;

private:

    std::string name;
    json definition;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;

    bool isHovered = false;

    friend class Canvas;

    bool multiSelected = false;
};

