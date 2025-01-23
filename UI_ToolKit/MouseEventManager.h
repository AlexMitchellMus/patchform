#pragma once

#include "Component.h"
#include <iostream>
#include "ComponentRegister.h"

namespace pptk {

class MouseEventManager {
public:
    MouseEventManager(Component* reg) : registry(reinterpret_cast<ComponentRegister*>(reg)) {}

    void handleMouseButtonDown(Component* root, SDL_Event& e) {
        draggingComponent = nullptr; // Reset dragging state
        propagateMouseButtonDown(root, e);
    }

    void handleMouseButtonUp(Component* root, SDL_Event& e) {
        if (draggingComponent) {
            draggingComponent->mouseButtonUp(e);
            updateHoveredComponent(root, e);
            draggingComponent = nullptr; // Reset dragging state
        } else {
            propagateMouseButtonUp(root, e);
        }
    }

    void handleMouseMove(Component* root, SDL_Event& e) {
        const Point currentPosition(e.motion.x, e.motion.y);
        const Point delta(e.motion.xrel, e.motion.yrel);

        if (e.motion.state & SDL_BUTTON_LMASK) {
            if (draggingComponent) {
                draggingComponent->mouseDrag(currentPosition, delta);
                return;
            }
        }

        updateHoveredComponent(root, e); // Update hovered component
    }

private:
    Component* draggingComponent = nullptr;
    Component* hoveredComponent = nullptr; // Track currently hovered component

    void updateHoveredComponent(Component* root, SDL_Event& e) {
        Component* newHovered = root->findComponentAt(e.motion.x, e.motion.y);

        // Update hover state if the hovered component changes
        if (newHovered != hoveredComponent) {
            if (hoveredComponent) {
                hoveredComponent->mouseLeave(e); // Notify old hovered component
            }
            if (newHovered) {
                newHovered->mouseEnter(e); // Notify new hovered component
            }
            hoveredComponent = newHovered; // Update hovered component
        }

        // Forward mouse move to the currently hovered component
        if (hoveredComponent) {
            hoveredComponent->handleMouseMove(e);
        }
    }

    void propagateMouseButtonDown(Component* component, SDL_Event& e) {
        if (!isComponentValid(component)) return;

        // Traverse children in reverse order
        auto& children = component->getChildren();
        for (auto it = children.rbegin(); it != children.rend();) {
            auto& child = *it;

            if (!isComponentValid(child)) {
                it = std::make_reverse_iterator(children.erase((it + 1).base()));
                continue;
            }

            propagateMouseButtonDown(child, e);
            if (draggingComponent) {
                return; // Stop propagation if a component starts dragging
            }

            ++it;
        }

        if (component->hitTest(e.button.x, e.button.y) && !draggingComponent) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                draggingComponent = component;
                component->mouseButtonDown(e);
            }
        }
    }

    void propagateMouseButtonUp(Component* component, SDL_Event& e) {
        if (!isComponentValid(component)) return;

        for (auto it = component->getChildren().begin(); it != component->getChildren().end();) {
            auto& child = *it;

            if (!isComponentValid(child)) {
                it = component->getChildren().erase(it); // Remove invalid child
                continue;
            }

            propagateMouseButtonUp(child, e);
            ++it;
        }

        component->mouseButtonUp(e);
    }

    bool isComponentValid(Component* component) const {
        return component == registry ? true : registry->exists(component);
    }

protected:
    ComponentRegister* registry;
};

} // namespace pptk
