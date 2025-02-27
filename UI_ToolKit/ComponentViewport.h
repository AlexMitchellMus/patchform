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

        void renderAll(NVGcontext* nvg) override
        {
            nvgSave(nvg);
            nvgScissor(nvg, getX(), getY(), getWidth(), getHeight());

            // Offset rendering for scrolling
            nvgTranslate(nvg, 0, scrollOffset);

            Component::renderAll(nvg); // Render all children

            nvgResetScissor(nvg);
            nvgRestore(nvg);
        }

        void mouseWheel(CompEvent& e) override
        {
            scrollOffset -= e.sdlEvent.wheel.y * 20.0f; // Adjust scrolling speed
            repaint();
        }

        void setContentHeight(float contentHeight)
        {
            maxScroll = std::max(0.0f, contentHeight - getHeight());
        }

    private:
        float scrollOffset = 0.0f;
        float maxScroll = 0.0f;
    };
}   // end namespace pptk
