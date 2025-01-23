//
// Created by alexw on 23/01/2025.
//

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