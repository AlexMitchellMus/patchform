#pragma once

#include "Component.h"

namespace pptk
{
    class ComponentViewport : public pptk::Component
    {
    public:
        ComponentViewport()
        {
            setWantsFocus(true);
        }

        void setViewport(Component* child)
        {
            viewportChild = child;
            if (child)
            {
                addComponent(child);
                setContentHeight(child->getHeight());
            }
        }

        virtual void renderViewportBackground(NVGcontext* nvg) {};

        void renderAll(NVGcontext* nvg) override
        {
            nvgSave(nvg);
            // Apply translation for this component's position
            nvgTranslate(nvg, x, y);
            nvgScale(nvg, scale, scale);

            nvgScissor(nvg, 0, 0, getWidth(), getHeight());
            renderViewportBackground(nvg);

            nvgSave(nvg);

            // Offset rendering for scrolling
            nvgTranslate(nvg, 0, -scrollOffset);

            if (viewportChild)
                viewportChild->renderAll(nvg);

            nvgRestore(nvg);

            renderScrollbar(nvg);

            nvgResetScissor(nvg);
            nvgRestore(nvg);
        }

        void mouseWheel(CompEvent& e) override
        {
            if (!viewportChild) return;

            scrollOffset -= e.sdlEvent.wheel.y * 30.0f; // Adjust scrolling speed
            clampScroll();
            repaint();
        }

        void setContentHeight(float contentHeight)
        {
            maxScroll = std::max(0.0f, contentHeight - getHeight());
            std::cout << "maxScroll: " << maxScroll << " child height: " << contentHeight <<  std::endl;
            clampScroll();
        }

    private:
        void clampScroll()
        {
            scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);
            std::cout << "clapping scroll: " << scrollOffset << std::endl;
        }

        void renderScrollbar(NVGcontext* nvg)
        {
            if (!viewportChild || maxScroll <= 0.0f) return; // No scrolling needed

            float contentHeight = viewportChild->getHeight();
            float viewportHeight = getHeight();

            // Calculate scrollbar height based on visible portion
            float scrollbarHeight = (viewportHeight / contentHeight) * viewportHeight;
            scrollbarHeight = std::max(scrollbarHeight, 20.0f); // Minimum size for visibility

            // Calculate scrollbar position
            float scrollbarPos = (scrollOffset / maxScroll) * (viewportHeight - scrollbarHeight);

            nvgBeginPath(nvg);
            nvgDrawRoundedRect(nvg, getWidth() - 10, scrollbarPos, 8, scrollbarHeight, bg, bg, 4);
        }

        Component* viewportChild = nullptr;
        float scrollOffset = 0.0f;
        float maxScroll = 0.0f;

        NVGcolor bg = nvgRGBA(200, 200, 200, 180);
    };
} // namespace pptk
