#pragma once

#include "SDL3/SDL.h"
#include "SafePointer.h"

namespace pptk {

    class Component;

    class CompEvent {
    public:
        SDL_Event sdlEvent;

        // Use SafePointer instead of a raw pointer.
        SafePointer<Component> originalComponent;

        // A flag to indicate if propagation should stop.
        bool stopPropagation = false;
    };

} // namespace pptk