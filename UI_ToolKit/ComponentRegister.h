/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include <memory>
#include <mutex>
#include "ankerl/unordered_dense.h" // Include unordered_dense map

#include "Component.h"

namespace pptk {

class ComponentRegister : public Component {
public:

    void handleTime(uint32_t time)
    {
        for (const auto& [ c, callback ] : timerCallbacks)
        {
            callback();
        }
    }

    void registerTimerCallback(Component* c, std::function<void()> callback)
    {
        timerCallbacks.emplace_back(c, callback);
    }

    void unregisterTimerCallback(Component* component)
    {
        // Remove all tuples whose first element (Component*) equals cPtr
        timerCallbacks.erase(
            std::remove_if(timerCallbacks.begin(), timerCallbacks.end(),
                           [component](auto& tup)
                           {
                               return std::get<0>(tup) == component;
                           }),
            timerCallbacks.end()
        );
    }

    void clearReferencesTo(Component* c)
    {
        for (Component** tracked : { &draggingComponent, &hoveredComponent, &clickedComponent })
        {
            if (*tracked == c)
            {
                *tracked = nullptr;
            }
        }
    }

    Component* getDraggingComponent() const       { return draggingComponent; }
    void       setDraggingComponent(Component* c) { draggingComponent = c;    }

    Component* getHoveredComponent() const        { return hoveredComponent; }
    void       setHoveredComponent(Component* c)  { hoveredComponent = c;     }

    Component* getClickedComponent() const        { return clickedComponent; }
    void       setClickedComponent(Component* c)  { clickedComponent = c;     }

protected:

    Component* draggingComponent = nullptr;
    Component* hoveredComponent = nullptr;
    Component* clickedComponent = nullptr;

    std::vector<std::tuple<Component*, std::function<void()>>> timerCallbacks;
};

}
