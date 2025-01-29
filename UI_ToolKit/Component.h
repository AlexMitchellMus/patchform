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

enum class Button { LEFT, RIGHT, MIDDLE };

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

    Point getPositionInParent(Component* specificParent) const
    {
        Point relativePosition(0, 0);

        const Component* current = this;

        while (current && current != specificParent)
        {
            relativePosition.x += current->x;
            relativePosition.y += current->y;
            current = current->parent;
        }

        if (!current) {
            throw std::runtime_error("Specified parent is not an ancestor of this component.");
        }

        return relativePosition;
    }


    Rect getAbsoluteBounds() const {
        Point absolutePosition = getAbsolutePosition();
        return pptk::Rect{absolutePosition.x, absolutePosition.y, width, height};
    }

    void removeComponent(Component* child)
    {
        children.erase(std::remove(children.begin(), children.end(), child), children.end());
    }


    // Hit test in local coords
    virtual bool hitTest(float px, float py) const {
        return px >= 0 && px <= width &&
               py >= 0 && py <= height;
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

    // Finds the component at global coordinate.
    // Disregards self, so make sure to call it from the Component you want to disregard from
    Component* findComponentAt(int globalX, int globalY)
    {
        return (getRootComponent())->findComponentAt(globalX, globalY, this);
    }

    // Find the component at (x, y), including children
    Component* findComponentAt(int globalX, int globalY, Component* selfComponent) {

        // Transform the global coordinates to local coordinates for this component
        Point localPos = globalToLocalWithScale(globalX, globalY);

        // Always check children first, in reverse order for topmost components
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            Component* child = *it;
            if (child->isVisible()) {
                // Pass the original global coordinates to the child
                Component* found = child->findComponentAt(globalX, globalY, selfComponent);
                if (found && found != selfComponent) {
                    return found; // Return the first matching child
                }
            }
        }

        // Check this component only after its children
        if (hitTest(localPos.x, localPos.y)) {
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

    Component* getRootComponent()
    {
        // Find the root component, because we can assign components inside constructors, so root can't be set
        if (rootCoponent)
            return rootCoponent;

        Component* current = this;
        while (current->parent)
        {
            current = current->parent;
        }

        rootCoponent = current;

        return current;
    }

    virtual void mouseEnter(SDL_Event& e) { }
    virtual void mouseLeave(SDL_Event& e) { }
    virtual void mouseMove(const Point& position) { }
    virtual void mouseDrag(const Point& position, const Point& delta, Button button) { }
    virtual void mouseWheel(SDL_Event& e) { }
    virtual void keyPressed(SDL_Event& e) { }

    virtual void render(NVGcontext* vg) { };
    virtual void resized() { };
    virtual void renderAll(NVGcontext* vg);

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

    void setBounds(const float newX, const float newY, const float newW, const float newH);

    Point getPosition()
    {
        return Point(x, y);
    }

    void registerTimer(std::function<void()> callback);

    Point globalToLocalWithScale(float globalX, float globalY) const {
        // If there's a parent, first convert to the parent's local space
        if (parent) {
            // First, transform into parent's coordinate space
            Point parentLocal = parent->globalToLocalWithScale(globalX, globalY);
            globalX = parentLocal.x;
            globalY = parentLocal.y;
        }

        // Offset by this component's position
        globalX -= x;
        globalY -= y;

        // **Apply parent's scale recursively**
        if (scale != 1.0f && scale > 0.0f) {
            globalX /= scale;
            globalY /= scale;
        }

        return Point(globalX, globalY);
    }

    Point globalToLocal2(float globalX, float globalY) const {
        // Recursively transform to parent's local coordinates
        if (parent) {
            Point parentLocal = parent->globalToLocal(globalX, globalY);
            globalX = parentLocal.x;
            globalY = parentLocal.y;
        }

        // Optionally handle viewport and scaling (if applicable)
        globalX -= x;
        globalY -= y;

        // Optionally apply scaling (uncomment if scaling is used)
        // globalX /= scale;
        // globalY /= scale;

        return Point(globalX, globalY);
    }


    Point globalToLocal(float globalX, float globalY) const {
        // Recursively transform to parent's local coordinates
        if (parent) {
            Point parentLocal = parent->globalToLocal(globalX, globalY);
            globalX = parentLocal.x;
            globalY = parentLocal.y;
        }

        // Offset by this component's position
        //globalX -= x;
        //globalY -= y;

        // Optionally handle viewport and scaling (if applicable)
        globalX -= viewportX;
        globalY -= viewportY;

        // Optionally apply scaling (uncomment if scaling is used)
        globalX /= scale;
        globalY /= scale;

        return Point(globalX, globalY);
    }

    Point localToGlobal(float localX, float localY) const {
        // Apply this component's viewport offset
        localX += viewportX;
        localY += viewportY;

        // Optionally apply scaling (uncomment if scaling is used)
        localX *= scale;
        localY *= scale;

        // Recursively transform to parent's global coordinates
        if (parent) {
            Point parentGlobal = parent->localToGlobal(localX, localY);
            localX = parentGlobal.x;
            localY = parentGlobal.y;
        }

        return Point(localX, localY);
    }

    void setName(const std::string& newName) { name = newName;; };
    std::string& getName() { return name; };

    float finalX = 0.0f;
    float finalY = 0.0f;

    float viewportX = 0.0f;
    float viewportY = 0.0f;

    float scale = 1.0f;

    Component* getParent() const { return parent; };

    // Compute accumulated scale from root to this component
    float getAccumulatedScale() const {
        float accumulatedScale = 1.0f;
        const Component* current = this;
        while (current) {
            accumulatedScale *= current->scale; // Multiply each parent's scale
            current = current->getParent();
        }
        return accumulatedScale;
    }

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

    Component* rootCoponent = nullptr;
};

} // namespace ppuitk