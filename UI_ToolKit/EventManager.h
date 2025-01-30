/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Component.h"
#include <iostream>

#include "RootComponent.h"

namespace pptk {

class MouseEventManager {
public:
    MouseEventManager(Component* rootComp) : rootComponent(reinterpret_cast<ComponentRegister*>(rootComp)) {}

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

        switch (e.motion.state) {
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

        if (auto draggedComp = rootComponent->getDraggingComponent()) {
            auto posLocal = draggedComp->globalToLocalWithScale(currentPosition.x, currentPosition.y);
            auto accScale = draggedComp->getAccumulatedScale();
            auto localDelta = Point(delta.x / accScale, delta.y / accScale);
            draggedComp->mouseDrag(posLocal, localDelta, buttonPressed);
            return;
        }

        updateHoveredComponent(rootComponent, e); // Update hovered component
    }

    void handleMouseWheel(SDL_Event& e) {
        if (auto hoveredComp = rootComponent->getHoveredComponent()) {
            hoveredComp->mouseWheel(e);
        }
    }

    void handleKeyDown(SDL_Event& e) {
        if (auto clickedComp = rootComponent->getClickedComponent()) {
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
            // Use global-to-local transformation including scaling
            Point localMouse = hovered->globalToLocalWithScale(globalMouse.x, globalMouse.y);
            e.motion.x = static_cast<int>(localMouse.x);
            e.motion.y = static_cast<int>(localMouse.y);

            hovered->handleMouseMove(e);
        }
    }

    void propagateMouseButtonDown(Component* component, SDL_Event& e) {
        if (!component->isVisible()) {
            return;
        }

        // Use scaled coordinate transformation
        Point localPos = component->globalToLocalWithScale(e.button.x, e.button.y);

        // Traverse children in reverse order (for z-order handling)
        auto& children = component->getChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto& child = *it;

            if (child->isVisible()) {
                propagateMouseButtonDown(child, e);
                if (rootComponent->getDraggingComponent()) {
                    return; // Stop propagation if a component is dragging
                }
            }
        }

        if (component->hitTest(localPos.x, localPos.y) && !rootComponent->getDraggingComponent()) {
            rootComponent->setDraggingComponent(component);
            rootComponent->setClickedComponent(component); // Store the clicked component

            // Adjust event coordinates before passing it to the component
            e.button.x = static_cast<int>(localPos.x);
            e.button.y = static_cast<int>(localPos.y);

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
