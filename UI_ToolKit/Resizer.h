#pragma once

#include "CompEvent.h"
#include "Component.h"
#include "SDL3/SDL.h"

namespace pptk {

class Resizer : public Component
{
public:

    enum class ResizerMode : unsigned int
    {
        None   = 0,
        Left   = 1 << 0,
        Right  = 1 << 1,
        Top    = 1 << 2,
        Bottom = 1 << 3,
        All    = static_cast<unsigned int>(Left)   |
                 static_cast<unsigned int>(Right)  |
                 static_cast<unsigned int>(Top)    |
                 static_cast<unsigned int>(Bottom)
    };

    Resizer(Component* compTarget)
    {
        target = compTarget;
        target->onVisibilityChanged = [this]()
        {
            setVisible(target->isVisible());
        };
    };

    void mouseButtonDown(CompEvent& e) override
    {
        mouseDownPos.x = e.sdlEvent.button.x;
        originalBounds = target->getBounds();
    }

    void mouseButtonUp(CompEvent& e) override
    {
        draggingEdge = DraggedEdge::None;
    }

    void mouseEnter(CompEvent& e) override
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_W_RESIZE));
    }

    void mouseLeave(CompEvent& e) override
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    }

    bool hitTest(float x, float y) override
    {
        if (resizerMode == ResizerMode::None)
            return false;

        // FIXME: We should have a base hitTest that checks if the point is inside the bounds
        // Then call the user hitTest to further refine it - maybe?
        if (y < 8 || y > height - 8)
            return false;

        switch (resizerMode)
        {
        case ResizerMode::Left:
            // hit left edge
            if (x > 0 && x < 8)
            {
                draggingEdge = DraggedEdge::Left;
                return true;
            }
            break;
        default:
        case ResizerMode::Right:
            if (x > getWidth() - 8 && x < getWidth())
            {
                draggingEdge = DraggedEdge::Right;
                return true;
            }
            break;
        }

        return false;
    }

    void setBounds(const Rect& bounds) override
    {
        auto expandedBounds = bounds;
        Component::setBounds(expandedBounds.expanded(8));
    }

    void mouseDrag(const Point& position, const Point& delta, Button button) override
    {
        auto posMoved = position - mouseDownPos;

        switch (draggingEdge)
        {
        case DraggedEdge::Left:
            {
                auto currBounds = target->getBounds();
                // Calculate new x and width for left edge drag.
                float newX = currBounds.x + posMoved.x;
                float newWidth = currBounds.w - posMoved.x;

                if (newWidth < target->getMinWidth())
                {
                    // Clamp to minimum width.
                    float diff = target->getMinWidth() - newWidth;
                    newX -= diff;
                    newWidth = target->getMinWidth();
                }
                else if (newWidth > target->getMaxWidth())
                {
                    newWidth = target->getMaxWidth();
                    newX = currBounds.x + (currBounds.w - target->getMaxWidth());
                }

                target->setBounds(newX, currBounds.y, newWidth, currBounds.h);
                break;
            }
        case DraggedEdge::Right:
            {
                // Adjust width when dragging the right edge.
                float newWidth = originalBounds.w + posMoved.x;

                target->setBounds(originalBounds.x, originalBounds.y, newWidth, originalBounds.h);
                break;
            }
        default:
            break;
        }
    }
//#define DEBUG_RESIZER
#ifdef DEBUG_RESIZER
    void render(NVGcontext* vg) override
    {
        if (resizerMode == ResizerMode::None)
            return;

        nvgBeginPath(vg);
        switch (resizerMode)
        {
        case ResizerMode::Left:
            nvgRect(vg, 0, 0, 8, height);
            break;
        case ResizerMode::Right:
            nvgRect(vg, width - 8, 0, 8, height);
            break;
        }

        nvgFillColor(vg, nvgRGB(255, 0, 0));
        nvgFill(vg);
    }
#endif

    ResizerMode resizerMode = ResizerMode::None;

private:
    Component* target;
    Point mouseDownPos;
    Rect originalBounds;

    enum class DraggedEdge { None, Left, Right };
    DraggedEdge draggingEdge = DraggedEdge::None;
};

/**
 * @brief A component that supports user-driven resizing.
 *
 * This class acts as a decorator by extending the functionality of Component.
 * It lazily creates a Resizer instance and delegates the resizing behavior to it. By calling
 * setResizable() with a specific ResizerMode, you can configure which edges (Left, Right, Top, Bottom, All)
 * are active for resizing.
 *
 * Make sure to resize the resizer in the resized() function of the target class:
 * @code
 * class MyComponent : public ResizableComponent {
 * public:
 *     MyComponent() {
 *         // Enable left and right resizing
 *         setResizable(Resizer::ResizerMode::Left | Resizer::ResizerMode::Right);
 *     }
 *
 *     // Override resized() to ensure the resizer is correctly positioned and sized.
 *     void resized() override {
 *         // Update your component's own layout logic here.
 *         // For example, reposition children or update internal state.
 *
 *         // Now update the resizer bounds to match the new size of this component.
 *         auto bounds = getBounds();
 *         getResizer().setBounds(bounds.x, bounds.y, bounds.w, bounds.h);
 *     }
 * };
 */
class ResizableComponent : public Component
{
public:

    void setResizable(Resizer::ResizerMode mode)
    {
        getResizer().resizerMode = mode;
    }

    Resizer& getResizer()
    {
        if (resizer == nullptr)
            resizer = std::make_unique<Resizer>(this);
        return *resizer;
    };

private:
    std::unique_ptr<Resizer> resizer;
};

}