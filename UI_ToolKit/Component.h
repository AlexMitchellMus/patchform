/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

// Base Component class

#pragma once

#include <memory>
#include <sstream>
#include <utility>
#include <vector>
#include "SDL3/SDL.h"
#include "nanovg.h"
#include <functional>
#include <iostream>

#include "unordered_dense.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace pptk {
// Event structure
struct Event {
    SDL_Event sdlEvent;
    std::string type;
};

struct Point {
    float x = 0.0f;
    float y = 0.0f;

    Point() = default;

    Point(float x, float y) : x(x), y(y) {}

    std::string toString() const {
        std::ostringstream oss;
        oss << "Point(x: " << x << ", y: " << y << ")";
        return oss.str();
    }

    Point operator+(const Point& other) const {
        return Point(x + other.x, y + other.y);
    }

    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    bool contains(const Point& point) const
    {
        return (point.x >= x && point.x <= x + w && point.y >= y && point.y <= y + h);
    }

    bool contains(float px, float py) const
    {
        return contains(Point(px, py));
    }

    bool contains(const Rect& other) const
    {
        // Ensure dimensions are valid (non-negative)
        if (w < 0 || h < 0 || other.w < 0 || other.h < 0)
            return false;

        // Use epsilon for floating-point comparisons
        const float epsilon = 0.0001f;

        return (other.x + epsilon >= x &&
                other.x + other.w - epsilon <= x + w &&
                other.y + epsilon >= y &&
                other.y + other.h - epsilon <= y + h);
    }

    bool intersects(const Rect& other) const
    {
        // Ensure dimensions are valid (non-negative)
        if (w < 0 || h < 0 || other.w < 0 || other.h < 0)
            return false;

        // Check if there is no overlap
        bool noOverlap = (x + w <= other.x ||       // This rect is to the left of the other
                          other.x + other.w <= x || // Other rect is to the left of this
                          y + h <= other.y ||       // This rect is above the other
                          other.y + other.h <= y);  // Other rect is above this

        return !noOverlap; // Rectangles intersect if there is overlap
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "Rect(x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << ")";
        return oss.str();
    }
};

class ComponentRegister;

class Component {
public:
    explicit Component() = default;

    virtual ~Component();

    void setVisible(bool shouldBeVisible);

    bool isVisible() const { return visible; };

    void setSize(float newWidth, float newHeight)
    {
        width = newWidth;
        height = newHeight;
    }

    void setPosition(float newX, float newY)
    {
        x = newX;
        y = newY;
    }

    void setPosition(const Point& point)
    {
        setPosition(point.x, point.y);
    }

    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    void addComponent(Component* child);

    const std::vector<Component*>& getChildren() const
    {
        return children;
    }

    // Computes the absolute position of the component
    Point getAbsolutePosition() const {
        if (parent) {
            Point parentPosition = parent->getAbsolutePosition();
            return Point(parentPosition.x + x, parentPosition.y + y);
        }
        return Point(x, y); // Root component
    }

    Rect getAbsoluteBounds() const {
        Point absolutePosition = getAbsolutePosition();
        return pptk::Rect{absolutePosition.x, absolutePosition.y, width, height};
    }

    void removeComponent(Component* child)
    {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }


    // Hit test that accounts for parent position
    virtual bool hitTest(float px, float py) const {
        Point absolutePosition = getAbsolutePosition();
        return px >= absolutePosition.x && px <= absolutePosition.x + width &&
               py >= absolutePosition.y && py <= absolutePosition.y + height;
    }

    virtual void mouseButtonDown(SDL_Event& e) {
        for (auto it = children.begin(); it != children.end(); ) {
            auto& child = *it;

            if (child->hitTest(e.button.x, e.button.y)) {
                child->mouseButtonDown(e);
            }
            ++it;
        }
    }

    virtual void handleMouseMove(SDL_Event& e)
    {
        if (getBounds().contains(e.button.x, e.button.y))
        {
            mouseMove(Point(e.button.x, e.button.y));
        }
    }

    virtual void mouseButtonUp(SDL_Event& e) {}

    // Find the component at (x, y), including children
    Component* findComponentAt(int x, int y) {
        // Always check children first
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (isVisible()) {
                Component* child = (*it)->findComponentAt(x, y);
                if (child && child->isVisible())
                    return child; // Return the first matching child
            }
        }

        // Check the current component only after its children
        if (hitTest(x, y)) {
            return this;
        }

        // No matching component found
        return nullptr;
    }

    template <typename T>
    T* findParentOfClass()
    {
        Component* current = this;
        while (current != nullptr)
        {
            T* parent = dynamic_cast<T*>(current);
            if (parent != nullptr)
            {
                return parent; // Found a parent of the specified type
            }
            current = current->parent; // Move up to the parent
        }
        return nullptr; // No parent of the specified type found
    }

    Component* findRootComponent()
    {
        // Find the root component, because we can assign components inside constructors, so root can't be set
        Component* current = this;
        while (current->parent)
        {
            current = current->parent;
        }
        return current;
    }

    virtual void mouseEnter(SDL_Event& e) { }

    virtual void mouseLeave(SDL_Event& e) { }

    virtual void mouseMove(const Point& position) { }

    virtual void mouseDrag(const Point& position, const Point& delta) { }

    virtual void keyPressed(SDL_Event& e) { }

    virtual void render(NVGcontext* vg) { };

    virtual void resized() { };

    virtual void renderAll(NVGcontext* vg)
    {
        nvgSave(vg);

        // Apply translation for this component's position
        if (parent)
        {
            auto offsetPos = parent->getBounds();
            nvgTranslate(vg, offsetPos.x, offsetPos.y);
        }

        // Render this component
        render(vg);

        // Render children
        for (auto& child : children) {
            if (child->isVisible())
                child->renderAll(vg);
        }

        // Restore previous transformation
        nvgRestore(vg);
    }

    Rect getBounds() const
    {
        return Rect{ x, y, width, height };
    }

    void setMinSize(const float width, const float height)
    {
        minWidth = width;
        minHeight = height;
    }

    void setMaxSize(const float width, const float height)
    {
        maxWidth = width;
        maxHeight = height;
    }

    void setMinMaxSize(const float newMinWidth, const float newMaxWidth, const float newMinHeight, const float newMaxHeight)
    {
        setMinSize(newMinWidth, newMinHeight);
        setMaxSize(newMaxWidth, newMaxHeight);
    }

    void setBounds(const Rect& bounds)
    {
        setBounds(bounds.x, bounds.y, bounds.w, bounds.h);
    }

    void setBounds(const float newX, const float newY, const float newW, const float newH)
    {
        x = newX;
        y = newY;

        float clampedW = newW;
        float clampedH = newH;

        if (minWidth > 0) clampedW = std::max(minWidth, clampedW);
        if (minHeight > 0) clampedH = std::max(minHeight, clampedH);
        if (maxWidth > 0) clampedW = std::min(maxWidth, clampedW);
        if (maxHeight > 0) clampedH = std::min(maxHeight, clampedH);

        width = clampedW;
        height = clampedH;

        resized();
    }

    Point getPosition()
    {
        return Point(x, y);
    }

    void registerTimer(std::function<void()> callback);

private:
    void removeFromParent();

protected:

    std::string name;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    float minWidth = -1;
    float minHeight = -1;

    float maxWidth = -1;
    float maxHeight = -1;

    bool visible = true;

    std::vector<Component*> children;
    bool isDragging = false;

    Component* draggingComponent = nullptr;

    Component* parent = nullptr;
};

} // namespace ppuitk