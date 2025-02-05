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

#ifndef NANOVG_GL3_IMPLEMENTATION
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg.h"
#endif


#include <functional>
#include <iostream>

#include "SafePointer.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace pptk {

class CompEvent;

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

    [[nodiscard]] float length(const pptk::Point& other) const
    {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
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

class PopupComponent;
class Component : public SafeObject {
public:
    std::function<void()> onVisibilityChanged = [](){};

    explicit Component() = default;

    virtual ~Component();

    void setVisible(bool shouldBeVisible);

    bool isVisible() const { return visible; };

    void setSize(float newWidth, float newHeight)
    {
        width = newWidth;
        height = newHeight;

        resized();
        repaint();
    }

    void setPosition(float newX, float newY);
    void setPosition(const Point& point);

    float getX() const { return x; }
    float getY() const { return y; }
    float getWidth() const { return width; }
    float getHeight() const { return height; }

    void addComponent(Component* child);

    PopupComponent* getPopupComponent();
    void setPopupComponent(std::unique_ptr<PopupComponent> popupComponent);

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

    Point getPositionInParent()
    {
        return getPositionInParent(parent);
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
    virtual bool hitTest(float px, float py) {
        return px >= 0 && px <= width &&
               py >= 0 && py <= height;
    }

    virtual bool consumeEvent(CompEvent& e) { return false; };
    virtual void mouseButtonDown(CompEvent& e);
    virtual void handleMouseMove(CompEvent& e);
    virtual void mouseButtonUp(CompEvent& e) {}

    // Finds the component at global coordinate.
    // Disregards self, so make sure to call it from the Component you want to disregard from
    Component* findComponentAt(int globalX, int globalY);

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

    Component* getRootComponent();

    virtual void mouseEnter(CompEvent& e) { }
    virtual void mouseLeave(CompEvent& e) { }
    virtual void mouseMove(const Point& position) { }
    virtual void mouseDrag(const Point& position, const Point& delta, Button button) { }
    virtual void mouseWheel(CompEvent& e) { }
    virtual void keyPressed(CompEvent& e) { }

    virtual void render(NVGcontext* vg) { };
    virtual void resized() { };
    virtual void renderAll(NVGcontext* vg);

    Rect getBounds() const
    {
        return Rect{ x, y, width, height };
    }

    void setMinWidth(const float width)
    {
        minWidth = width;
    }

    float getMinWidth() const
    {
        return minWidth;
    }

    float getMaxWidth() const
    {
        return maxWidth;
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

    void startFrameTimer(std::function<void()> callback);

    void stopFrameTimer();

    void registerGlobalMouseListener(std::function<void(Component*)> callback);

    void unregisterGlobalMouseListener();

    // TODO: we don't need 4, only 2 (from and to)
    Point globalToLocalWithScale(float globalX, float globalY) const;
    Point globalToLocal2(float globalX, float globalY) const;
    Point globalToLocal(float globalX, float globalY) const;

    Point localToGlobal(float localX, float localY) const;

    bool isOrHasChild(Component* target) {
        if (!target) return false;

        // Direct match
        if (this == target) return true;

        // Recursively check all child components
        for (auto* child : getChildren()) {
            if (child && child->isOrHasChild(target)) {
                return true;
            }
        }

        return false;
    }

    void setName(const std::string& newName) { name = newName;; };
    std::string& getName() { return name; };

    float finalX = 0.0f;
    float finalY = 0.0f;

    float viewportX = 0.0f;
    float viewportY = 0.0f;

    float scale = 1.0f;

    float opacity = 1.0f;

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

    bool needsRepaint();

    void repaint();

private:
    Component* findComponentAt(int globalX, int globalY, Component* selfComponent);

    void removeFromParent();

protected:
    std::string name;

    bool isDirty = true;

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