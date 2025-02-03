/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Object.h"
#include "Port.h"
#include "Canvas.h"
#include "Connection.h"


void Port::mouseButtonDown(pptk::CompEvent& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        cnv->newConnection = std::make_unique<Connection>(this);
        // We get the position of the port in the canvas
        auto conpos = getPositionInParent(cnv);
        cnv->addComponent(cnv->newConnection.get());
        cnv->newConnection->setPosition(conpos);
    }
}

void Port::mouseButtonUp(pptk::CompEvent& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        cnv->newConnection.reset();

        if (foundPort)
        {
            foundPort->isHoveredFromCable = false;

            auto thisObj = findParentOfClass<Object>();
            auto otherObj = foundPort->findParentOfClass<Object>();

            if (thisObj && otherObj && (thisObj != otherObj))
            {
                cnv->addConnection(this, foundPort.get());
                //std::cout << thisObj->getName() << " : " << portNum << " -> " << otherObj->getName() << " : " << foundPort->portNum << std::endl;
            }
        }
    }
}

void Port::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button button) {
    if (button == pptk::Button::LEFT)
    {
        if (auto cnv = findParentOfClass<Canvas>())
        {
            if (cnv->isInLockedMode())
                return;

            if (cnv->newConnection)
            {
                cnv->newConnection->setConnectionDest(currentPosition);

                auto globalPos = localToGlobal(currentPosition.x, currentPosition.y);

                auto c = cnv->newConnection->findComponentAt(globalPos.x, globalPos.y);
                if (auto* port = dynamic_cast<Port*>(c))
                {
                    // Only connect once for a new port, and if the port directions are correct: input->output or output->input
                    if ((direction != port->direction) && (port != foundPort.get()))
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
                        foundPort.reset();
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
    auto portCol = portType == PortType::Event ? blue : orange;
    nvgFillColor(nvg, portCol); // Blue fill for ports
    nvgFill(nvg);

    if (isHovered | isHoveredFromCable)
    {
        nvgBeginPath(nvg);
        nvgCircle(nvg, 5, 5, 10);
        portCol.a = 120;
        nvgFillColor(nvg, portCol);
        nvgFill(nvg);
    }
}
