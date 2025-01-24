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

void Port::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta) {
    if (auto cnv = findParentOfClass<Canvas>())
    {
        if (cnv->newConnection)
        {
            cnv->newConnection->setConnectionDest(currentPosition + getAbsolutePosition());

            auto c = rootComponent->findComponentAt(currentPosition.x, currentPosition.y);
            if (auto* port = dynamic_cast<Port*>(c))
            {
                if (port != foundPort)
                {
                    foundPort = port;
                    foundPort->isHoveredFromCable = true;
                    std::cout << "found PORT!" << port->portNum << std::endl;
                }
            }
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

void Port::render(NVGcontext* nvg)
{
    nvgBeginPath(nvg);

    auto radius = getWidth() / 2;
    nvgCircle(nvg, x + radius, y + radius, radius);
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
        nvgCircle(nvg, x + 5, y + 5, 10);
        auto alphaCol = portCol;
        portCol.a = 120;
        nvgFillColor(nvg, portCol);
        nvgFill(nvg);
    }
}
