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
        if (SDL_GetModState() & SDL_KMOD_SHIFT)
        {
            // Any object that user drags out while also in multi-drag mode becomes selected
            if (auto parentObj = dynamic_cast<Object*>(getParent()))
            {
                if (!parentObj->getIsSelected())
                {
                    parentAddedToMultiConnect = true;
                    cnv->addToSelection(parentObj);
                }
            }

            for (auto obj : cnv->getSelectedObjects())
            {
                auto portToDragFrom = isOutput() ? obj->getOutPort(portNum) : obj->getInPort(portNum);
                // If the dragging port doesn't exist in the other selected objects, skip that object
                if (portToDragFrom)
                {
                    cnv->newConnections.emplace_back(std::make_unique<Connection>(isOutput() ? obj->getOutPort(portNum) : obj->getInPort(portNum)));
                    cnv->addComponent(cnv->newConnections.back().get());
                }
            }
        } else
        {
            cnv->newConnections.emplace_back(std::make_unique<Connection>(this));
            cnv->addComponent(cnv->newConnections.back().get());
        }
    }
}

void Port::mouseButtonUp(pptk::CompEvent& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        if (foundPort)
        {
            foundPort->isHoveredFromCable = false;

            if (cnv->newConnections.size())
            {
                auto dest = foundPort.get();

                std::vector<std::tuple<Port*, Port*>> portCons;

                for (auto& con : cnv->newConnections)
                {
                    auto origin = con->getOriginPort();
                    portCons.emplace_back(origin, dest);
                }
                cnv->addMultipleConnections(portCons);
            }
        }
        cnv->newConnections.clear();
        foundPort.reset();

        if (parentAddedToMultiConnect)
        {
            cnv->removeFromSelection(reinterpret_cast<Object*>(getParent()));
            parentAddedToMultiConnect = false;
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

            if (cnv->newConnections.size())
            {
                auto globalPos = localToGlobal(currentPosition.x, currentPosition.y);

                for (auto& conn : cnv->newConnections)
                {
                    conn->setConnectionDest(globalPos);
                }

                if (auto* port = cnv->findPort(globalPos.x, globalPos.y))
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

    if (canvasLocked)
        portCol.a *= 0.3f;

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
