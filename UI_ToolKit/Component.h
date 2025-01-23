//
// Created by alexw on 22/01/2025.
//

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

    std::string toString() const {
        std::ostringstream oss;
        oss << "Rect(x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << ")";
        return oss.str();
    }
};

class ComponentRegister;

class Component {
public:
    Component() : rootComponent(this) {};

    explicit Component(Component* parent);

    virtual ~Component();

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

    std::vector<Component*>& getChildren()
    {
        return children;
    }

    Component* getRoot() const
    {
        return rootComponent;
    }

    // Computes the absolute position of the component
    Point getAbsolutePosition() const {
        if (parent) {
            Point parentPosition = parent->getAbsolutePosition();
            return Point(parentPosition.x + x, parentPosition.y + y);
        }
        return Point(x, y); // Root component
    }

    //void removeComponent(Component* child)
    //{
    //    children.erase(std::remove(children.begin(), children.end(), child), children.end());
    //}

    /*
    virtual bool hitTest(float x, float y)
    {
        return (getBounds().contains(x, y));
    }*/
    // Hit test that accounts for parent position
    virtual bool hitTest(float px, float py) const {
        Point absolutePosition = getAbsolutePosition();
        return px >= absolutePosition.x && px <= absolutePosition.x + width &&
               py >= absolutePosition.y && py <= absolutePosition.y + height;
    }

    virtual void mouseButtonDown(SDL_Event& e) {
        for (auto it = children.begin(); it != children.end(); ) {
            auto& child = *it;

            if (!isComponentValid(child)) {
                it = children.erase(it);
            } else {
                if (child->hitTest(e.button.x, e.button.y)) {
                    child->mouseButtonDown(e);
                }
                ++it;
            }
        }
    }

    virtual void handleMouseMove(SDL_Event& e)
    {
        if (getBounds().contains(e.button.x, e.button.y))
        {
        }
    }

    virtual void mouseButtonUp(SDL_Event& e) {
        for (auto it = children.begin(); it != children.end(); ) {
            auto& child = *it;

            if (!isComponentValid(child)) {
                it = children.erase(it);
            } else {
                child->isDragging = false;
                child->mouseButtonUp(e);
                ++it;
            }
        }
    }

    // Find the component at (x, y), including children
    Component* findComponentAt(int x, int y) {
        // Always check children first
        for (auto it = getChildren().rbegin(); it != getChildren().rend(); ++it) {
            if (isComponentValid(*it)) {
                Component* child = (*it)->findComponentAt(x, y);
                if (child)
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

    virtual void mouseEnter(SDL_Event& e) {}

    virtual void mouseLeave(SDL_Event& e) {}

    virtual void mouseMove(const Point& position) {}

    virtual void mouseDrag(const Point& position, const Point& delta) {}

    virtual void render(NVGcontext* vg) { };

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
            if (isComponentValid(child))
                child->renderAll(vg);
        }

        // Restore previous transformation
        nvgRestore(vg);
    }

    Rect getBounds() const
    {
        return Rect{ x, y, width, height };
    }

    void setBounds(const Rect& bounds)
    {
        setBounds(bounds.x, bounds.y, bounds.w, bounds.h);
    }

    void setBounds(const float newX, const float newY, float newW, float newH)
    {
        x = newX;
        y = newY;
        width = newW;
        height = newH;
    }

    Point getPosition()
    {
        return Point(x, y);
    }

private:
    bool isComponentValid(Component* c);

protected:

    std::string name;

    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    std::vector<Component*> children;
    bool isDragging = false;

    Component* draggingComponent = nullptr;

    Component* parent = nullptr;
    Component* rootComponent = nullptr;
};

} // namespace ppuitk