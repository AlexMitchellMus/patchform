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
        // Directly inject the invalid tile coverage of the temp connection that will be deleted
        //if (auto* root = dynamic_cast<pptk::RootComponent*>(getRootComponent()))
        //{
        //    for (const auto& conn : cnv->newConnections)
        //    {
        //        conn->computeTileCoverage(root->tileMaskBuffer.previousTileMask);
        //    }
        //}
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

                if (auto* port = cnv->findPort(globalPos.x, globalPos.y, direction))
                {
                    if (port->getParent() == this->getParent())
                        return; // or continue if inside a loop

                    // Only connect once for a new port, and if the port directions are correct: input->output or output->input
                    if ((direction != port->direction) && (port != foundPort.get()))
                    {
                        // If we have a previous found port, reset it now
                        if (foundPort)
                            foundPort->setHoveredFromCable(false);

                        foundPort = port;
                        foundPort->setHoveredFromCable(true);
                    }
                }
                else
                {
                    if (foundPort)
                    {
                        foundPort->setHoveredFromCable(false);
                        // Reset the found port to nothing
                        foundPort.reset();
                    }
                }
            }
        }
    }
}

void Port::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    float fullWidth = getWidth();
    auto width = !canvasLocked ? fullWidth : 5.0f;
    auto radius = width / 2;
    auto size = radius * 2;
    float offset = (fullWidth - width) / 2.0f;

    auto orange = nvgRGB(120, 74, 28);
    auto blue = nvgRGB(28, 73, 119);
    auto portCol = portType == PortType::Event ? blue : orange;

    // Draw main port as rounded rect (circle)
    nvgDrawRoundedRect(nvg, offset, offset, size, size, portCol, portCol, radius);

    // Hover effect
    if (isHovered || isHoveredFromCable)
    {
        NVGcolor hoverCol = portCol;
        hoverCol.a = 120;
        nvgDrawRoundedRect(nvg, -5, -5, 20, 20, hoverCol, hoverCol, 10);
    }
}

