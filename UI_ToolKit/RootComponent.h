/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include <memory>

#include "PopupComponent.h"
#include "../UI_ToolKit/FontMetrics.h"

namespace pptk
{
    class RootComponent : public Component
    {
    public:
        void handleTime(uint32_t time)
        {
            auto callbacksCopy = timerCallbacks; // Copy to avoid iterator invalidation

            for (auto& [componentPtr, callback] : callbacksCopy)
            {
                if (componentPtr) // Ensure component still exists
                {
                    callback(time); // Execute the callback (even if the original vector changed)
                }
            }
        }


        void registerTimerCallback(Component* c, const std::function<void(uint32_t)>& callback)
        {
            unregisterTimerCallback(c);

            timerCallbacks.emplace_back(c, callback);
        }

        void unregisterTimerCallback(Component* component)
        {
            for (auto& [c, callback] : timerCallbacks)
            {
                if (c == component)
                    c = nullptr; // Mark for deletion
            }

            // Remove all marked entries after iteration is complete
            timerCallbacks.erase(
                std::ranges::remove_if(timerCallbacks,
                                       [](auto& tup) { return std::get<0>(tup) == nullptr; }).begin(),
                timerCallbacks.end()
            );
        }

        Component* getDraggingComponent() const { return draggingComponent.get(); }
        void setDraggingComponent(Component* c) { draggingComponent = makeSafePointer(c); }

        Component* getHoveredComponent() const { return hoveredComponent.get(); }
        void setHoveredComponent(Component* c) { hoveredComponent = makeSafePointer(c); }

        Component* getClickedComponent() const { return clickedComponent.get(); }
        void setClickedComponent(Component* c) { clickedComponent = makeSafePointer(c); }

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

        std::vector<std::tuple<Component*, std::function<void(uint32_t)>>> timerCallbacks;
    };
}
