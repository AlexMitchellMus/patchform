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
    using Registry = ankerl::unordered_dense::set<Component*>;

    // Register a component
    void registerComponent(Component* component) {
        if (component)
        {
            registry.insert(component);
        }
    }

    // Unregister a component
    void unregisterComponent(Component* component) {
        if (component)
        {
            registry.erase(component);
        }
    }

    // Check if a component exists in the registry
    bool exists(Component* component) const {
        return registry.contains(component);
    }

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

protected:
    Registry registry;

    std::vector<std::tuple<Component*, std::function<void()>>> timerCallbacks;
};

}
