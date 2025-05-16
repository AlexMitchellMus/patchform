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
#include "Theme.h"
#include "../UI_ToolKit/FontMetrics.h"
#include "../UI_ToolKit/CommandIDManager.h"
#include "concurrentqueue.h"

namespace pptk
{
    class RootComponent : public Component
    {
    public:
        RootComponent()
        {
            theme.applyDefaults();
        }
        
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

        // Entry point for NanoVG rendering
        void renderFrame(NVGcontext* nvg)
        {
            Component::renderAll(nvg, theme);
            drawDebugTiles(nvg);
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

        void resized() override
        {
            tilesX = (getWidth() + tileSize - 1) / tileSize;
            tilesY = (getHeight() + tileSize - 1) / tileSize;
            dirtyTiles.resize((tilesX * tilesY + 63) / 64);
            std::fill(dirtyTiles.begin(), dirtyTiles.end(), 0);
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
            globalMouseHandlers.erase(
                std::ranges::remove_if(globalMouseHandlers,
                    [component](auto& tup)
                    {
                        const auto& safePtr = std::get<0>(tup);
                        return !safePtr || safePtr == component;
                    }).begin(),
                globalMouseHandlers.end()
            );
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

        void applyTheme(const Theme& theme)
        {
            applyThemeChange(theme);
        }

        CommandIDManager commandIDManager;

        void setStructureDirty()
        {
            structureDirty = true;
        }

        // Process all pending repaints
        // This marks the root components tile map as dirty where it intersects the component
        bool processRepaintQueue()
        {
            bool didRepaint = false;
            SafePointer<Component> repaintComponent;

            std::ranges::fill(dirtyTiles, 0);

            while (repaintQueue.try_dequeue(repaintComponent)) {
                if (!repaintComponent)
                    continue;

                repaintComponent->isDirty = true;
                didRepaint = true;

                auto* root = dynamic_cast<RootComponent*>(repaintComponent->getRootComponent());
                if (!root || root->tilesX == 0 || root->tilesY == 0)
                    continue;

                constexpr auto tileSize = RootComponent::tileSize;
                const int tileCount = root->tilesX * root->tilesY;
                if (tileCount <= 1)
                    continue;

                if (repaintComponent->tileBits.size() * 64 < tileCount)
                    repaintComponent->tileBits.resize((tileCount + 63) / 64, 0);
                else
                    std::ranges::fill(repaintComponent->tileBits, 0);

                const Rect gb = repaintComponent->getGlobalBounds();
                int minX = gb.x / tileSize;
                int maxX = (gb.x + gb.w) / tileSize;
                int minY = gb.y / tileSize;
                int maxY = (gb.y + gb.h) / tileSize;

                for (int y = minY; y <= maxY; ++y)
                {
                    for (int x = minX; x <= maxX; ++x)
                    {
                        if (x < 0 || x >= root->tilesX || y < 0 || y >= root->tilesY)
                            continue;

                        int index = y * root->tilesX + x;
                        repaintComponent->tileBits[index / 64] |= 1ULL << (index % 64);
                        root->dirtyTiles[index / 64] |= 1ULL << (index % 64);
                    }
                }
//#define DEBUG_DIRTY_BITS
#ifdef DEBUG_DIRTY_BITS
                std::cout << "--------- before render all ----------" << std::endl;
                for (int y = 0; y < root->tilesY; ++y) {
                    for (int x = 0; x < root->tilesX; ++x) {
                        int index = y * root->tilesX + x;
                        bool bitSet = (root->dirtyTiles[index / 64] >> (index % 64)) & 1ULL;
                        std::cout << (bitSet ? "#" : ".");
                    }
                    std::cout << "\n";
                }
#endif
            }

            return didRepaint;
        }

        void drawDebugTiles(NVGcontext* vg) {
            constexpr int tileSize = RootComponent::tileSize;

            // First pass: non-active tiles (light grid)
            for (int y = 0; y < tilesY; ++y) {
                for (int x = 0; x < tilesX; ++x) {
                    int index = y * tilesX + x;
                    bool isDirty = (dirtyTiles[index / 64] >> (index % 64)) & 1ULL;
                    if (isDirty) continue;

                    int px = x * tileSize;
                    int py = y * tileSize;

                    nvgBeginPath(vg);
                    nvgRect(vg, px, py, tileSize, tileSize);
                    nvgStrokeColor(vg, nvgRGB(80, 80, 80));
                    nvgStrokeWidth(vg, 1.0f);
                    nvgStroke(vg);
                }
            }

            // Second pass: active (dirty) tiles
            for (int y = 0; y < tilesY; ++y) {
                for (int x = 0; x < tilesX; ++x) {
                    int index = y * tilesX + x;
                    bool isDirty = (dirtyTiles[index / 64] >> (index % 64)) & 1ULL;
                    if (!isDirty) continue;

                    int px = x * tileSize;
                    int py = y * tileSize;

                    nvgBeginPath(vg);
                    nvgRect(vg, px, py, tileSize, tileSize);
                    nvgFillColor(vg, nvgRGBA(255, 0, 0, 40));
                    nvgFill(vg);

                    nvgStrokeColor(vg, nvgRGB(255, 0, 0));
                    nvgStrokeWidth(vg, 1.0f);
                    nvgStroke(vg);
                }
            }
        }

        int tilesX = -1;
        int tilesY = -1;
        static constexpr int tileSize = 64;
        std::vector<uint64_t> dirtyTiles;

        moodycamel::ConcurrentQueue<SafePointer<Component>> repaintQueue;

    private:
        FontMetricsCache fontMetricsCache;

        bool structureDirty = true;

    protected:
        SafePointer<Component> draggingComponent;
        SafePointer<Component> hoveredComponent;
        SafePointer<Component> clickedComponent;
        SafePointer<Component> focusedComponent;
        SafePointer<Component> lastFocusedComponent;

        Theme theme;

        std::vector<std::tuple<SafePointer<Component>, std::function<void(uint32_t, uint32_t)>, int>> timerCallbacks;

        std::vector<std::tuple<SafePointer<Component>, std::function<void(Component*)>>> globalMouseHandlers;
    };
}
