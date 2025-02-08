/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Component.h"
#include <iostream>

#include "CompEvent.h"
#include "RootComponent.h"

namespace pptk {

class EventManager {
public:
    EventManager(Component* rootComp) : rootComponent(reinterpret_cast<RootComponent*>(rootComp)) {}

    void handleMouseButtonDown(SDL_Event& e) {
        rootComponent->setDraggingComponent(nullptr); // Reset dragging state
        auto wrappedEvent = CompEvent(e, rootComponent);
        if (const auto comp = findDeepestHitComponent(rootComponent, wrappedEvent))
        {
            rootComponent->setDraggingComponent(comp);
            rootComponent->setClickedComponent(comp);

            Point localPos = comp->globalToLocalWithScale(wrappedEvent.sdlEvent.button.x, wrappedEvent.sdlEvent.button.y);

            wrappedEvent.sdlEvent.button.x = localPos.x;
            wrappedEvent.sdlEvent.button.y = localPos.y;

            comp->mouseButtonDown(wrappedEvent);

            for (auto& [c, handler] : rootComponent->globalMouseHandlers)
            {
                handler(comp);
            }
        }
    }

    void handleMouseButtonUp(SDL_Event& e) {
        if (auto draggedComp = rootComponent->getDraggingComponent()) {
            auto wrappedEvent = CompEvent(e, draggedComp);
            draggedComp->mouseButtonUp(wrappedEvent);
            updateHoveredComponent(rootComponent, wrappedEvent);
            rootComponent->setDraggingComponent(nullptr); // Reset dragging state
        } else {
            auto wrappedEvent = CompEvent(e, rootComponent);
            propagateMouseButtonUp(rootComponent, wrappedEvent);
        }
    }

    void handleMouseMove(SDL_Event& e) {
        const Point currentPosition(e.motion.x, e.motion.y);
        const Point delta(e.motion.xrel, e.motion.yrel);

        Button buttonPressed;
        // SDL button state is bitwise, however we only deal with left/right/middle single button press ATM
        // Not left | right etc...
        switch (e.motion.state) {
            case SDL_BUTTON_RMASK:
                buttonPressed = Button::RIGHT;
                break;
            case SDL_BUTTON_MMASK:
                buttonPressed = Button::MIDDLE;
                break;
            default:
                buttonPressed = Button::LEFT;
                break;
        }

        if (auto draggedComp = rootComponent->getDraggingComponent()) {
            auto posLocal = draggedComp->globalToLocalWithScale(currentPosition.x, currentPosition.y);
            auto accScale = draggedComp->getAccumulatedScale();
            auto localDelta = Point(delta.x / accScale, delta.y / accScale);
            draggedComp->mouseDrag(posLocal, localDelta, buttonPressed);
            return;
        }

        auto wrappedEvent = CompEvent(e, rootComponent);
        updateHoveredComponent(rootComponent, wrappedEvent); // Update hovered component
    }

    void handleMouseWheel(SDL_Event& e) {
        if (auto hoveredComp = rootComponent->getHoveredComponent()) {
            auto wrappedEvent = CompEvent(e, hoveredComp);
            hoveredComp->mouseWheel(wrappedEvent);
        }
    }

    void handleKeyDown(SDL_Event& e) {
        if (auto clickedComp = rootComponent->getClickedComponent()) {
            auto wrappedEvent = CompEvent(e, clickedComp);
            clickedComp->keyPressed(wrappedEvent);
        }
    }

private:

    void updateHoveredComponent(Component* root, CompEvent& e) {
        Point globalMouse(e.sdlEvent.motion.x, e.sdlEvent.motion.y);

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
            e.sdlEvent.motion.x = static_cast<int>(localMouse.x);
            e.sdlEvent.motion.y = static_cast<int>(localMouse.y);

            hovered->handleMouseMove(e);
        }
    }

    Component* findDeepestHitComponent(Component* component, CompEvent& e) {
        if (!component->isVisible()) {
            return nullptr;
        }

        // Convert global coordinates to the component’s local space.
        Point localPos = component->globalToLocalWithScale(e.sdlEvent.button.x, e.sdlEvent.button.y);

        // Determine if this component would normally be hit.
        bool isHit = component->hitTest(localPos.x, localPos.y);

        // If this component is not hit and it doesn't allow its children to be hit,
        // then skip it entirely.
        if (!isHit && !component->allowsClicksOnChildComponents()) {
            return nullptr;
        }

        // First, if the component is hit, ask it if it wants to consume the event.
        // This allows a layer to decide "I want this event" (returning true) or "pass it down" (returning false).
        if (isHit && component->consumeEvent(e)) {
            return component;
        }

        // Check children in reverse order (topmost first).
        auto& children = component->getChildren();
        for (size_t i = children.size(); i > 0; --i) {
            Component* child = children.at(i - 1);
            Component* hitChild = findDeepestHitComponent(child, e);
            if (hitChild != nullptr) {
                return hitChild;
            }
        }

        // Only return this component if it was hit AND it intercepts mouse clicks.
        if (isHit && component->interceptsMouseClicks()) {
            return component;
        }

        return nullptr;
    }

    void propagateMouseButtonUp(Component* component, CompEvent& e) {
        // Only handle the clicked component
        if (auto clickedComp = rootComponent->getClickedComponent()) {
            clickedComp->mouseButtonUp(e);
        }

        // Reset the clicked component after handling the event
        rootComponent->setClickedComponent(nullptr);
    }

protected:
    RootComponent* rootComponent;
};

} // namespace pptk
