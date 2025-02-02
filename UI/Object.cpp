/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Canvas.h"
#include "Object.h"
#include "../Nodes/AudioNodeBase.h"

#include <glaze/reflection/get_name.hpp>

Object::Object(AudioNode* node) : nodeID(node->nodeID), audioNode(node), name(node->getName())
{
    setBounds(0, 0, 120, 40);

    auto convertPortType = [](const AudioPort::PortType& type) {
        return (type == AudioPort::Signal) ? Port::PortType::Audio :
               (type == AudioPort::Data) ? Port::PortType::Event : Port::PortType::None;
    };

    for (int i = 0; i < node->inputPortBuffers.size(); ++i)
    {
        inPorts.push_back(std::make_unique<Port>(i, convertPortType(node->inputPortBuffers[i]->getPortType())));
        addComponent(inPorts.back().get());
    }

    if (convertPortType(node->outputPort.getPortType()) != Port::PortType::None)
    {
        outPorts.push_back(std::make_unique<Port>(0, convertPortType(node->outputPort.getPortType()), Port::Direction::Output));
        addComponent(outPorts.back().get());
    }
    Object::resized();
}

Object::Object(std::string name) : name(name)
{
    setBounds(0, 0, 120, 40);

    for (int i = 0; i < 2; ++i)
    {
        inPorts.push_back(std::make_unique<Port>(i, Port::PortType::Audio));
        addComponent(inPorts.back().get());
    }

    outPorts.push_back(std::make_unique<Port>(0, Port::PortType::Audio, Port::Direction::Output));
    addComponent(outPorts.back().get());
    Object::resized();
}

Object::~Object()
{
    std::cout << "object deleting: " << std::endl;
    if (auto* cnv = findParentOfClass<Canvas>())
    {
        cnv->removeConnectionsFor(this);
    }
}

void Object::resized()
{
    int portDiam = 10;
    int numInputs = static_cast<int>(inPorts.size());
    float spacing = (getWidth() - 2 - (numInputs * portDiam)) / std::max(1, numInputs - 1);

    for (int i = 0; i < inPorts.size(); ++i)
    {
        inPorts[i]->setBounds(i * (spacing + portDiam) + 1, 1, portDiam, portDiam);
    }

    if (!outPorts.empty())
    {
        outPorts[0]->setBounds(1, getHeight() - portDiam - 1, portDiam, portDiam);
    }
}

void Object::mouseEnter(SDL_Event& e)
{
    isHovered = true;
    repaint();
}

void Object::mouseLeave(SDL_Event& e)
{
    isHovered = false;
    repaint();
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
        {
            cnv->setSelected(this);
        }
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

void Object::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button button)
{
    if (button == pptk::Button::LEFT)
    {
        if (auto cnv = findParentOfClass<Canvas>())
        {
            cnv->setMultiObjectPosition(delta);
        }
    }
}

void Object::render(NVGcontext* nvg)
{
    nvgBeginPath(nvg);
    auto bgCol = nvgRGB(33, 33, 33);
    auto outLineCol = nvgRGB(45, 45, 45);
    if (isHovered) bgCol = outLineCol;
    if (isSelected) outLineCol = nvgRGB(28, 73, 119);
    nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);

    nvgFontSize(nvg, 18.0f);
    nvgFontFace(nvg, "Regular");
    nvgFillColor(nvg, nvgRGB(190, 190, 190));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    nvgText(nvg, 10, height / 2, name.c_str(), nullptr);
}
