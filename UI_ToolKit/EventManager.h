/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Component.h"
#include <iostream>

#include "ComponentRegister.h"

namespace pptk {

class MouseEventManager {
public:
    MouseEventManager(Component* reg) : rootComponent(reinterpret_cast<ComponentRegister*>(reg)) {}

    void handleMouseButtonDown(SDL_Event& e) {
        rootComponent->setDraggingComponent(nullptr); // Reset dragging state
        propagateMouseButtonDown(rootComponent, e);
    }

    void handleMouseButtonUp(SDL_Event& e) {
        if (auto draggedComp = rootComponent->getDraggingComponent()) {
            draggedComp->mouseButtonUp(e);
            updateHoveredComponent(rootComponent, e);
            rootComponent->setDraggingComponent(nullptr); // Reset dragging state
        } else {
            propagateMouseButtonUp(rootComponent, e);
        }
    }

    void handleMouseMove(SDL_Event& e) {
        const Point currentPosition(e.motion.x, e.motion.y);
        const Point delta(e.motion.xrel, e.motion.yrel);

        Button buttonPressed;

        switch (e.motion.state)
        {
            case SDL_BUTTON_LMASK:
                buttonPressed = Button::LEFT;
                    break;

            case SDL_BUTTON_RMASK:
                buttonPressed = Button::RIGHT;
                    break;

            case SDL_BUTTON_MMASK:
                buttonPressed = Button::MIDDLE;
                    break;
        }

        if (auto draggedComp = rootComponent->getDraggingComponent())
        {
            draggedComp->mouseDrag(currentPosition, delta, buttonPressed);
            return;
        }

        updateHoveredComponent(rootComponent, e); // Update hovered component
    }

    void handleMouseWheel(SDL_Event& e)
    {
        if (auto hoveredComp = rootComponent->getHoveredComponent())
        {
            hoveredComp->mouseWheel(e);
        }
    }

    void handleKeyDown(SDL_Event& e)
    {
        if (auto clickedComp = rootComponent->getClickedComponent())
        {
            clickedComp->keyPressed(e);
        }
    }

private:

    void updateHoveredComponent(Component* root, SDL_Event& e) {
        Point globalMouse(e.motion.x, e.motion.y);

        // Use findComponentAt with global coordinates
        Component* newHovered = root->findComponentAt(globalMouse.x, globalMouse.y);

        Component* oldHovered = rootComponent->getHoveredComponent();

        if (newHovered != oldHovered) {
            if (oldHovered) {
                oldHovered->mouseLeave(e);
            }
            if (newHovered) {
                newHovered->mouseEnter(e);
            }

            rootComponent->setHoveredComponent(newHovered);
        }

        // Forward the mouse move event to the hovered component
        if (auto hovered = rootComponent->getHoveredComponent()) {
            // Use global-to-local transformation for the hovered component
            Point localMouse = hovered->globalToLocal(globalMouse.x, globalMouse.y);
            e.motion.x = localMouse.x;
            e.motion.y = localMouse.y;

            hovered->handleMouseMove(e);
        }
    }

    void propagateMouseButtonDown(Component* component, SDL_Event& e) {

        if (!component->isVisible()) {
            return;
        }

        Point localPos = component->globalToLocal(e.button.x, e.button.y);

        // Traverse children in reverse order
        auto& children = component->getChildren();
        for (auto it = children.rbegin(); it != children.rend();) {
            auto& child = *it;

            if (child->isVisible())
            {
                propagateMouseButtonDown(child, e);
                if (rootComponent->getDraggingComponent()) {
                    return; // Stop propagation if a component starts dragging
                }
            }

            ++it;
        }

        if (component->hitTest(localPos.x, localPos.y) && !rootComponent->getDraggingComponent()) {
            rootComponent->setDraggingComponent(component);
            rootComponent->setClickedComponent(component); // Store the clicked component

            e.button.x = localPos.x;
            e.button.y = localPos.y;

            component->mouseButtonDown(e);
        }
    }


    void propagateMouseButtonUp(Component* component, SDL_Event& e) {
        // Only handle the clicked component
        if (auto clickedComp = rootComponent->getClickedComponent()) {
            clickedComp->mouseButtonUp(e);
        }

        // Reset the clicked component after handling the event
        rootComponent->setClickedComponent(nullptr);
    }

protected:
    ComponentRegister* rootComponent;
};

} // namespace pptk
