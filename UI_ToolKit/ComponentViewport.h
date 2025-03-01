#pragma once

#include "Component.h"

namespace pptk
{
    class ComponentViewport : public pptk::Component
    {
    public:
        class Scrollbar : public pptk::Component
        {
            public:

            ~Scrollbar()
            {
                // Manually manage the lifetime of the frametimer!
                // FIXME! we shouldn't need to, this should be handled by the timer!
                stopFrameTimer(0);
                stopFrameTimer(1);
            }

            std::function<void(float)> onScroll = [](float){};

            Scrollbar()
            {
            };

            void setScrollbarHeight(float barHeight)
            {
                scrollbarHeight = barHeight;
                repaint();
            }

            void setScrollPosition(float pos)
            {
                scrollbarPosition = pos;
                repaint();
            }

            void mouseEnter(CompEvent& e) override
            {
                isHovered = true;
                stopFrameTimer(1); // Stop any running shrink timer

                startFrameTimer([this](uint32_t time)
                {
                    growing = true;
                    animatedGrowth += 0.02f;

                    if (animatedGrowth >= 1.0f)
                    {
                        animatedGrowth = 1.0f;
                        growing = false; // Mark as fully grown
                        stopFrameTimer(0);
                    }
                    repaint();
                }, 0);

                repaint();
            }

            void mouseLeave(CompEvent& e) override
            {
                uint32_t leaveTime = SDL_GetTicks(); // Capture leave time

                startFrameTimer([this, leaveTime](uint32_t time)
                {
                    // Wait half a seconds AFTER growth is fully done
                    if (growing || time - leaveTime < 500)
                    {
                        return;
                    }

                    animatedGrowth -= 0.02f;
                    if (animatedGrowth <= 0.0f)
                    {
                        animatedGrowth = 0.0f;
                        isHovered = false;
                        stopFrameTimer(1);
                    }
                    repaint();
                }, 1);

                repaint();
            }


            void mouseButtonDown(CompEvent& e) override {
                // Record the difference between where the mouse is and where the thumb starts.
                dragOffset = e.sdlEvent.button.y - scrollbarPosition;
            }

            void mouseDrag(const Point& position, const Point& delta, Button button) override {
                // Instead of using delta, use the absolute position (adjusted by dragOffset)
                onScroll(position.y - dragOffset);
            }

            void render(NVGcontext* nvg) override
            {
                nvgBeginPath(nvg);
                float finalWidth = 4;
                if (isHovered)
                {
                    finalWidth = finalWidth + (width - finalWidth) * animatedGrowth;
                    auto finalRad = width * 0.5f;
                    nvgRoundedRectVarying(nvg, 0, 0, width, height, 0, finalRad, finalRad, 0);
                    auto bgFinal = bg;
                    bgFinal.a *= animatedGrowth;
                    nvgFillColor(nvg, bgFinal);
                    nvgFill(nvg);
                }
                nvgDrawRoundedRect(nvg, width - finalWidth, scrollbarPosition, finalWidth, scrollbarHeight, fg, fg, finalWidth * 0.5f);
            }
        private:
            NVGcolor bg = nvgRGBA(200, 200, 200, 10);
            NVGcolor fg = nvgRGBA(200, 200, 200, 180);
            bool isHovered = false;
            float scrollbarPosition = 0;
            float scrollbarHeight = 0;
            float dragOffset = 0.0f;
            float animatedGrowth = 0.0f;
            bool growing = false;
        };
        ComponentViewport()
        {
            setWantsFocus(true);

            scrollbar = std::make_unique<Scrollbar>();

            scrollbar->onScroll = [this](float newThumbPos) {
                float viewportHeight = getHeight();
                if (!viewportChild) return;
                float contentHeight = viewportChild->getHeight();

                // Calculate the thumb height based on the visible fraction of the content.
                float thumbHeight = (viewportHeight / contentHeight) * viewportHeight;
                thumbHeight = std::max(thumbHeight, 20.0f); // enforce a minimum height

                // Clamp the new thumb position so the thumb stays fully within the scrollbar.
                float clampedThumbPos = std::clamp(newThumbPos, 0.0f, viewportHeight - thumbHeight);

                // Update the scroll offset based on the thumb's position.
                // When the thumb is at 0, scrollOffset is 0; when at (viewportHeight - thumbHeight), scrollOffset becomes maxScroll.
                scrollOffset = (clampedThumbPos / (viewportHeight - thumbHeight)) * maxScroll;

                updateScrollbar();

            };

            addComponent(scrollbar.get());

            ComponentViewport::resized();
        }

        void resized() override
        {
            scrollbar->setBounds(width - 10, 0, 10, height);
            updateScrollbar();
        }

        void setViewport(std::unique_ptr<Component> child)
        {
            viewportChild = std::move(child);
            if (viewportChild)
            {
                addComponent(viewportChild.get());
                viewportChild->toBack();
                setContentHeight(viewportChild->getHeight());
                ComponentViewport::resized();
            }
        }

        template <typename T>
        T* getViewedComponent()
        {
            return dynamic_cast<T*>(viewportChild.get());
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

            scrollbar->renderAll(nvg);

            nvgResetScissor(nvg);
            nvgRestore(nvg);
        }

        void mouseWheel(CompEvent& e) override
        {
            if (!viewportChild) return;

            scrollOffset -= e.sdlEvent.wheel.y * 50.0f; // Adjust scrolling speed
            clampScroll();

            updateScrollbar();
        }

        void updateScrollbar()
        {
            if (!viewportChild || maxScroll <= 0.0f)
                return; // No scrolling needed

            float contentHeight = viewportChild->getHeight();
            float viewportHeight = getHeight();

            // Calculate scrollbar height based on visible portion
            float scrollbarHeight = (viewportHeight / contentHeight) * viewportHeight;
            scrollbarHeight = std::max(scrollbarHeight, 20.0f); // Minimum size for visibility

            scrollbar->setScrollbarHeight(scrollbarHeight);

            // Calculate scrollbar position
            float scrollbarPos = (scrollOffset / maxScroll) * (viewportHeight - scrollbarHeight);
            scrollbar->setScrollPosition(scrollbarPos);
            repaint();
        }

        void setContentHeight(float contentHeight)
        {
            maxScroll = std::max(0.0f, contentHeight - getHeight());
            clampScroll();
        }

    private:
        std::unique_ptr<Scrollbar> scrollbar;
        std::unique_ptr<Component> viewportChild;

        void clampScroll()
        {
            scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);
        }

        float scrollOffset = 0.0f;
        float maxScroll = 0.0f;

        NVGcolor bg = nvgRGBA(200, 200, 200, 180);
    };
} // namespace pptk
