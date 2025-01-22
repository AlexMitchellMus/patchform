//
// Created by alexw on 22/01/2025.
//

#pragma once

#include "Component.h"
#include <iostream>

namespace pptk {

class MouseEventManager {
public:
    void handleMouseButtonDown(Component* root, SDL_Event& e) {
        std::cout << "mouse down event" << std::endl;
        draggingComponent = nullptr; // Reset the dragging state
        propagateMouseButtonDown(root, e);
    }

    void handleMouseButtonUp(Component* root, SDL_Event& e) {
        if (draggingComponent) {
            draggingComponent->mouseButtonUp(e);
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

        propagateMouseMove(root, e);
    }

private:
    Component* draggingComponent = nullptr;

    void propagateMouseButtonDown(Component* component, SDL_Event& e) {
        // Traverse children in reverse order
        for (auto it = component->getChildren().rbegin(); it != component->getChildren().rend(); ++it) {
            Component* child = *it;
            propagateMouseButtonDown(child, e);
            if (draggingComponent) {
                return; // Stop propagation if a component starts dragging
            }
        }

        if (component->hitTest(e.button.x, e.button.y) && !draggingComponent) {
            if (e.button.button == SDL_BUTTON_LEFT) {
                draggingComponent = component;
                component->mouseButtonDown(e);
            }
        }
    }

    void propagateMouseButtonUp(Component* component, SDL_Event& e) {
        for (Component* child : component->getChildren()) {
            propagateMouseButtonUp(child, e);
        }

        component->mouseButtonUp(e);
    }

    void propagateMouseMove(Component* component, SDL_Event& e) {
        // Copy the children vector to avoid potential modification during iteration
        auto children = component->getChildren();

        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            Component* child = *it;
            propagateMouseMove(child, e);
        }

        // Handle the current component's mouse move
        component->handleMouseMove(e);
    }
};

} // namespace pptk
