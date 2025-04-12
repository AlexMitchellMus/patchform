#pragma once

#include "SDL3/SDL.h"
#include "SafePointer.h"

namespace pptk {

    class Component;

    class CompEvent {
    public:
        SDL_Event sdlEvent;
        SafePointer<Component> originalComponent;
        bool stopPropagation = false;

        CompEvent() = default;

        CompEvent(const SDL_Event& evt, Component* source)
            : sdlEvent(evt), originalComponent(source) {}
    };

} // namespace pptk