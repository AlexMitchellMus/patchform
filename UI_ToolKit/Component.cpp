/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Component.h"
#include "ComponentRegister.h"

namespace pptk {

Component::~Component()
{
    //std::cout << "removing: " << typeid(*this).name() << std::endl;

    // Find the root component FIRST before we remove the parent!
    // We want to remove all saved state (hovered/dragged/etc) pointers to this component
    reinterpret_cast<ComponentRegister*>(findRootComponent())->clearReferencesTo(this);

    // Then! Remove component
    removeFromParent();

    for (auto* child : children)
    {
        child->parent = nullptr;
    }
    children.clear();
}

void Component::addComponent(Component* child)
{
    if (child->parent)
    {
        child->removeFromParent();
    }

    child->parent = this;
    children.push_back(child);
}

void Component::setVisible(bool shouldBeVisible)
{
    visible = shouldBeVisible;
};

void Component::removeFromParent()
{
    if (parent)
    {
        auto& siblings = parent->children;

        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        parent = nullptr;
    }
}

void Component::updateLayout()
{
    finalX = parent->finalX + x;
    finalY = parent->finalY + y;

    for (auto& child : children)
        child->updateLayout();
}

void Component::registerTimer(std::function<void()> callback)
{
    reinterpret_cast<ComponentRegister*>(findRootComponent())->registerTimerCallback(this, callback);
}

}