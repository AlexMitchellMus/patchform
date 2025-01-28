/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Object.h"
#include "Port.h"
#include "Canvas.h"
#include "Connection.h"


void Port::mouseButtonDown(SDL_Event& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        cnv->newConnection = std::make_unique<Connection>(this);
        cnv->addComponent(cnv->newConnection.get());
        std::cout << "adding new connection to cnv" << std::endl;
    }
}

void Port::mouseButtonUp(SDL_Event& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        cnv->newConnection.reset();
    }
    if (foundPort)
    {
        foundPort->isHoveredFromCable = false;

        auto thisObj = findParentOfClass<Object>();
        auto otherObj = foundPort->findParentOfClass<Object>();

        if (thisObj && otherObj && (thisObj != otherObj))
        {
            std::cout << thisObj->getName() << " : " << portNum << " -> " << otherObj->getName() << " : " << foundPort->portNum << std::endl;
        }
    }
}

void Port::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button button) {
    if (button == pptk::Button::LEFT)
    {
        if (auto cnv = findParentOfClass<Canvas>())
        {
            if (cnv->newConnection)
            {
                cnv->newConnection->setConnectionDest(currentPosition);
                //if (!rootComponent)
                //    return;
                auto c = findRootComponent()->findComponentAt(currentPosition.x, currentPosition.y);
                if (auto* port = dynamic_cast<Port*>(c))
                {
                    // Only connect once for a new port, and if the port directions are correct: input->output or output->input
                    if ((direction != port->direction) && (port != foundPort))
                    {
                        foundPort = port;
                        foundPort->isHoveredFromCable = true;
                        std::cout << "found PORT!" << port->portNum << std::endl;
                    }
                }
                // TODO: Make it so a cable dragged over an object will connect to closest port
                //else if (auto* object = dynamic_cast<Object*>(c))
                //{
                //        std::cout << "found object!" << object->getName() << std::endl;
                //}
                else
                {
                    if (foundPort)
                    {
                        foundPort->isHoveredFromCable = false;
                        foundPort = nullptr;
                        std::cout << "not over a port" << std::endl;
                    }
                }
            }
        }
    }
}

void Port::render(NVGcontext* nvg)
{
    nvgBeginPath(nvg);

    auto radius = getWidth() / 2;
    nvgCircle(nvg, radius, radius, radius);
    //https://colorkit.co/color/1c4977/
    //nvgRGB(119, 28, 118)
    auto orange = nvgRGB(120, 74, 28);
    auto blue = nvgRGB(28, 73, 119);
    auto portCol = portNum == 0 ? blue : orange;
    nvgFillColor(nvg, portCol); // Blue fill for ports
    nvgFill(nvg);

    if (isHovered | isHoveredFromCable)
    {
        nvgBeginPath(nvg);
        nvgCircle(nvg, 5, 5, 10);
        auto alphaCol = portCol;
        portCol.a = 120;
        nvgFillColor(nvg, portCol);
        nvgFill(nvg);
    }
}
