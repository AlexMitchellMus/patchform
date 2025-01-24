/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Canvas.h"
#include "Object.h"

#include <glaze/reflection/get_name.hpp>

Object::Object(Component* parent, const std::string& name) : Component(parent), name(name)
{
    int width = 120;
    int height = 40;

    int portDiam = 10;

    setBounds(0, 0, width, 40);

    width -= 2.0f;

    int numInputs = 2;

    int totalDiam = portDiam * numInputs;

    float spacing = (width - totalDiam)  / (numInputs - 1);

    for (int i = 0; i < numInputs; ++i)
    {
        auto port = std::make_unique<Port>(this, i);
        port->setBounds((i * spacing) + (i * portDiam) + 1.0f, 1.0f, portDiam, portDiam);
        addComponent(port.get());
        inPorts.push_back(std::move(port));
    }

    auto port = std::make_unique<Port>(this, 0);
    port->setBounds(1.0f, height - portDiam - 1.0f, portDiam, portDiam);
    addComponent(port.get());
    outPorts.push_back(std::move(port));
}

void Object::mouseButtonDown(SDL_Event& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        if (!isSelected)
        {
            cnv->setSelected(this);
            return;
        }
        multiSelected = cnv->areMultiObjectsSelected();

        if (!multiSelected)
            cnv->setSelected(this);
    }
}

void Object::keyPressed(SDL_Event& e)
{
    if (e.key.key == SDLK_DELETE || e.key.key == SDLK_BACKSPACE)
    {
        if (auto cnv = findParentOfClass<Canvas>())
            cnv->deleteSelectedObjects();
    }
}

void Object::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta)
{
    if (multiSelected)
    {
        if (auto cnv = findParentOfClass<Canvas>())
        {
            cnv->setMultiObjectPosition(delta);
        }
    }
    else
        setPosition(getPosition() + delta);
}

void Object::render(NVGcontext* nvg)
{
    nvgBeginPath(nvg);
    auto bgCol = nvgRGB(33, 33, 33);
    auto outLineCol = nvgRGB(45, 45, 45);
    if (isHovered) bgCol = outLineCol;
    if (isSelected) outLineCol = nvgRGB(28, 73, 119);
    nvgDrawRoundedRect(nvg, x, y, width, height, bgCol, outLineCol, 6.0f);

    nvgFontSize(nvg, 18.0f);
    nvgFontFace(nvg, "sans");
    nvgFillColor(nvg, nvgRGB(190, 190, 190));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    nvgText(nvg, x + 10, y + height / 2, name.c_str(), nullptr);
}
