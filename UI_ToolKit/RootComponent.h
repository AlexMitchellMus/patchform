/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <chrono>
#include <iostream>
#include <memory>

#include "PopupComponent.h"
#include "../UI_ToolKit/FontMetrics.h"

namespace pptk
{
    class RootComponent : public Component
    {
    public:
        
        void handleTime(uint32_t time, uint32_t deltaTime)
        {
            std::vector<decltype(timerCallbacks)::value_type> callbacksCopy;

            for (const auto& tup : timerCallbacks)
            {
                if (std::get<0>(tup)) // SafePointer is valid
                    callbacksCopy.push_back(tup);
            }

            for (const auto& [componentPtr, callback, id] : callbacksCopy)
            {
                callback(time, deltaTime);
            }

            // Remove expired callbacks
            std::erase_if(timerCallbacks, [](const auto& tup) {
                return !std::get<0>(tup);
            });
        }

        void registerTimerCallback(Component* c, const std::function<void(uint32_t, uint32_t)>& callback, int timerID = 0)
        {
            unregisterTimerCallback(c, timerID);

            timerCallbacks.emplace_back(c, callback, timerID);
        }

        void unregisterTimerCallback(const Component* component, int timerID = 0)
        {
            for (auto& [c, callback, id] : timerCallbacks)
            {
                if (c.get() == component && id == timerID)
                    c = nullptr; // Mark for deletion
            }

            // Remove all marked entries after iteration is complete
            timerCallbacks.erase(
                std::ranges::remove_if(timerCallbacks,
                                       [](auto& tup) { return std::get<0>(tup).get() == nullptr; }).begin(),
                timerCallbacks.end()
            );
        }

        [[nodiscard]] Component* getDraggingComponent() const { return draggingComponent.get(); }
        void setDraggingComponent(Component* c) { draggingComponent = makeSafePointer(c); }

        [[nodiscard]] Component* getHoveredComponent() const { return hoveredComponent.get(); }
        void setHoveredComponent(Component* c) { hoveredComponent = makeSafePointer(c); }

        [[nodiscard]] Component* getClickedComponent() const { return clickedComponent.get(); }
        void setClickedComponent(Component* c) { clickedComponent = makeSafePointer(c); }

        [[nodiscard]] Component* getFocusedComponent() const { return focusedComponent.get(); }
        void setFocusedComponent(Component* c)
        {
            if (!c && lastFocusedComponent && focusedComponent)
            {
                focusedComponent->focusLost();
                std::swap(lastFocusedComponent, focusedComponent);
                focusedComponent->focusGained();
            }
            if (c && c != focusedComponent.get())
            {
                if (focusedComponent)
                    focusedComponent->focusLost();

                focusedComponent = makeSafePointer(c);
                focusedComponent->focusGained();
                if (!lastFocusedComponent)
                    lastFocusedComponent = makeSafePointer(c);
            }
        }

        void callGlobalMouseHandlersOn(Component* comp)
        {
            std::erase_if(globalMouseHandlers, [](auto& tup) {
                return !std::get<0>(tup); // remove if SafePointer is expired
            });

            // Make a copy of the global handlers, as the call lambda's could register new global handlers!
            auto handlersCopy = globalMouseHandlers;
            for (auto& [c, handler] : handlersCopy)
            {
                if (c)
                    handler(comp);
            }
        }

        void registerGlobalMouse(Component* c, const std::function<void(pptk::Component*)>& callback)
        {
            globalMouseHandlers.emplace_back(c, callback);
        }

        void unregisterGlobalMouse(Component* component)
        {
            {
                // Remove all tuples whose first element (Component*) equals cPtr
                globalMouseHandlers.erase(
                    std::ranges::remove_if(globalMouseHandlers,
                                           [component](auto& tup)
                                           {
                                               return std::get<0>(tup).get() == component;
                                           }).begin(),
                    globalMouseHandlers.end()
                );
            }
        }

        std::unique_ptr<PopupComponent> popupWindow;

        void cacheFontMetrics(NVGcontext* vg, const std::vector<std::string>& fonts, const std::vector<float>& sizes)
        {
            // Use nanovg state once to cache font metrics
            nvgBeginFrame(vg, 0, 0, 1.0f);
            fontMetricsCache.cacheFontMetrics(vg, fonts);
            nvgEndFrame(vg);
        }

        float getTextWidth(const std::string& fontName, const float size, const std::string& text)
        {
            return fontMetricsCache.getTextWidth(fontName, size, text);
        }

    private:
        FontMetricsCache fontMetricsCache;

    protected:
        SafePointer<Component> draggingComponent;
        SafePointer<Component> hoveredComponent;
        SafePointer<Component> clickedComponent;
        SafePointer<Component> focusedComponent;
        SafePointer<Component> lastFocusedComponent;

        std::vector<std::tuple<SafePointer<Component>, std::function<void(uint32_t, uint32_t)>, int>> timerCallbacks;

        std::vector<std::tuple<SafePointer<Component>, std::function<void(Component*)>>> globalMouseHandlers;
    };
}
