#include "Canvas.h"
#include "Object.h"

Object::Object(Canvas* cnv, const std::string& name) : name(name)
{
    setBounds(0, 0, 120, 45);

    for (int i = 0; i < 4; ++i)
    {
        auto port = std::make_unique<Port>(i);
        port->setBounds(i * 30, 0, 10, 10);
        inPorts.push_back(addComponent(std::move(port)));
    }

    auto port = std::make_unique<Port>(0);
    port->setBounds(0, 35, 10, 10);
    outPorts.push_back(addComponent(std::move(port)));
}
