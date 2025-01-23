#include "Canvas.h"
#include "Object.h"

Object::Object(Component* parent, const std::string& name) : Component(parent), name(name)
{
    int width = 120;

    setBounds(0, 0, width, 40);

    int numInputs = 2;

    int diam = 10;

    int totalDiam = diam * numInputs;

    float spacing = (width - totalDiam)  / (numInputs - 1);

    for (int i = 0; i < numInputs; ++i)
    {
        auto port = std::make_unique<Port>(this, i);
        port->setBounds((i * spacing) + (i * diam), 0, diam, diam);
        addComponent(port.get());
        inPorts.push_back(std::move(port));
    }

    auto port = std::make_unique<Port>(this, 0);
    port->setBounds(0, 30, 10, 10);
    addComponent(port.get());
    outPorts.push_back(std::move(port));
}
