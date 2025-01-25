/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Component.h"
#include "ComponentRegister.h"

namespace pptk {
Component::Component(Component* parentComp)
    : parent(parentComp)
    , rootComponent(parent->rootComponent)
{
}

Component::~Component()
{
    //std::cout << "removing: " << typeid(*this).name() << std::endl;
    reinterpret_cast<ComponentRegister*>(rootComponent)->unregisterComponent(this);
}

void Component::setVisible(bool shouldBeVisible)
{
    visible = shouldBeVisible;
};

bool Component::isComponentValid(Component* c)
{
    return reinterpret_cast<ComponentRegister*>(rootComponent)->exists(c);
}

void Component::addComponent(Component* child)
{
    reinterpret_cast<ComponentRegister*>(rootComponent)->registerComponent(child);

    children.push_back(child);
}

}