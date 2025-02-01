/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include <memory>

#include "PopupComponent.h"

namespace pptk {

class RootComponent : public Component {
public:

    void handleTime(uint32_t time)
    {
        for (const auto& [ c, callback ] : timerCallbacks)
        {
            callback();
        }
    }

    void registerTimerCallback(Component* c, const std::function<void()>& callback)
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

    Component* getDraggingComponent() const       { return draggingComponent.get(); }
    void       setDraggingComponent(Component* c) { draggingComponent = makeSafePointer(c);    }

    Component* getHoveredComponent() const        { return hoveredComponent.get(); }
    void       setHoveredComponent(Component* c)  { hoveredComponent = makeSafePointer(c);     }

    Component* getClickedComponent() const        { return clickedComponent.get(); }
    void       setClickedComponent(Component* c)  { clickedComponent = makeSafePointer(c);     }

    void registerGlobalMouse(Component* c, const std::function<void(pptk::Component*)>& callback)
    {
        globalMouseHandlers.emplace_back(c, callback);
    }

    void unregisterGlobalMouse(Component* component)
    {
        {
            // Remove all tuples whose first element (Component*) equals cPtr
            globalMouseHandlers.erase(
                std::remove_if(globalMouseHandlers.begin(), globalMouseHandlers.end(),
                               [component](auto& tup)
                               {
                                   return std::get<0>(tup) == component;
                               }),
                globalMouseHandlers.end()
            );
        }
    }

    std::vector<std::tuple<Component*, std::function<void(Component*)>>> globalMouseHandlers;

    std::unique_ptr<PopupComponent> popupWindow;

protected:

    SafePointer<Component> draggingComponent;
    SafePointer<Component> hoveredComponent;
    SafePointer<Component> clickedComponent;

    std::vector<std::tuple<Component*, std::function<void()>>> timerCallbacks;
};

}
