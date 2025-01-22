#pragma once

#include "Component.h"
#include <iostream>

namespace pptk {

class MouseEventManager {
public:
    void handleMouseButtonDown(Component* root, SDL_Event& e) {
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
            auto& child = *it;
            propagateMouseButtonDown(child.get(), e); // Use raw pointer from unique_ptr
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
        for (auto& child : component->getChildren()) {
            propagateMouseButtonUp(child.get(), e); // Use raw pointer from unique_ptr
        }

        component->mouseButtonUp(e);
    }

    void propagateMouseMove(Component* component, SDL_Event& e) {
        auto& children = component->getChildren();

        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            auto& child = *it;
            propagateMouseMove(child.get(), e); // Use raw pointer from unique_ptr
        }

        // Handle the current component's mouse move
        component->handleMouseMove(e);
    }
};

} // namespace pptk
