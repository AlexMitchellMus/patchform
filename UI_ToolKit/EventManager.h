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
    MouseEventManager(Component* reg) : registry(reinterpret_cast<ComponentRegister*>(reg)) {}

    void handleMouseButtonDown(SDL_Event& e) {
        registry->setDraggingComponent(nullptr); // Reset dragging state
        propagateMouseButtonDown(registry, e);
    }

    void handleMouseButtonUp(SDL_Event& e) {
        if (auto draggedComp = registry->getDraggingComponent()) {
            draggedComp->mouseButtonUp(e);
            updateHoveredComponent(registry, e);
            registry->setDraggingComponent(nullptr); // Reset dragging state
        } else {
            propagateMouseButtonUp(registry, e);
        }
    }

    void handleMouseMove(SDL_Event& e) {
        const Point currentPosition(e.motion.x, e.motion.y);
        const Point delta(e.motion.xrel, e.motion.yrel);

        if (e.motion.state & SDL_BUTTON_LMASK) {
            if (auto draggedComp = registry->getDraggingComponent()) {
                draggedComp->mouseDrag(currentPosition, delta);
                return;
            }
        }

        updateHoveredComponent(registry, e); // Update hovered component
    }

    void handleKeyDown(SDL_Event& e)
    {
        if (auto clickedComp = registry->getClickedComponent())
        {
            clickedComp->keyPressed(e);
        }
    }

private:

    void updateHoveredComponent(Component* root, SDL_Event& e) {
        // Find which component (if any) is under the mouse
        Component* newHovered = root->findComponentAt(e.motion.x, e.motion.y);

        // Get the currently hovered component from the registry
        Component* oldHovered = registry->getHoveredComponent();

        // If the hovered component changed (including from some component to null, or null to some component)
        if (newHovered != oldHovered)
        {
            // Unhover the old component (if any)
            if (oldHovered)
            {
                oldHovered->mouseLeave(e);
            }

            // Hover the new component (if any)
            if (newHovered)
            {
                newHovered->mouseEnter(e);
            }

            // Update the registry to track the new hovered component (which can be nullptr)
            registry->setHoveredComponent(newHovered);
        }

        // Forward the mouse-move event to the currently hovered component
        if (auto hovered = registry->getHoveredComponent())
        {
            hovered->handleMouseMove(e);
        }
    }

    void propagateMouseButtonDown(Component* component, SDL_Event& e) {

        if (!component->isVisible()) {
            return;
        }

        // Traverse children in reverse order
        auto& children = component->getChildren();
        for (auto it = children.rbegin(); it != children.rend();) {
            auto& child = *it;

            if (child->isVisible())
            {
                propagateMouseButtonDown(child, e);
                if (registry->getDraggingComponent()) {
                    return; // Stop propagation if a component starts dragging
                }
            }

            ++it;
        }

        if (component->hitTest(e.button.x, e.button.y) && !registry->getDraggingComponent()) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                registry->setDraggingComponent(component);
                registry->setClickedComponent(component); // Store the clicked component
                component->mouseButtonDown(e);
            }
        }
    }


    void propagateMouseButtonUp(Component* component, SDL_Event& e) {
        // Only handle the clicked component
        if (auto clickedComp = registry->getClickedComponent()) {
            clickedComp->mouseButtonUp(e);
        }

        // Reset the clicked component after handling the event
        registry->setClickedComponent(nullptr);
    }

protected:
    ComponentRegister* registry;
};

} // namespace pptk
