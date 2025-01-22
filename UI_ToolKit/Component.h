//
// Created by alexw on 22/01/2025.
//

// Base Component class

#pragma once

#include <memory>
#include <sstream>
#include <vector>
#include <functional>
#include <unordered_map>

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

class Component {
public:
    virtual ~Component() = default;

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

    virtual void render(NVGcontext* vg) { };

    void addComponent(Component* child) {
        child->parent = this;
        children.push_back(child);
    }

    const std::vector<Component*>& getChildren() const
    {
        return children;
    }

    // Find the root component
    Component* getRoot() {
        if (parent == nullptr) {
            return this; // This is the root component
        }
        return parent->getRoot(); // Recursively find the root
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

    virtual void mouseButtonDown(SDL_Event& e)
    {
        for (auto& child : children)
        {
            if (child->hitTest(e.button.x, e.button.y))
                child->mouseButtonDown(e);
        }
    }

    virtual void handleMouseMove(SDL_Event& e)
    {
        if (getBounds().contains(e.button.x, e.button.y))
            bool isHovered;
    }

    virtual void mouseButtonUp(SDL_Event& e)
    {
        for (auto& child : children)
        {
            child->isDragging = false;
            child->mouseButtonUp(e);
        }
    }

    virtual void mouseMove(const Point& position) {}

    virtual void mouseDrag(const Point& position, const Point& delta) {}

    void renderAll(NVGcontext* vg)
    {
        nvgSave(vg);

        // Apply translation for this component's position
        Point offsetPos;
        if (parent != nullptr)
            offsetPos = parent->getPosition();

        nvgTranslate(vg, offsetPos.x, offsetPos.y);

        // Render this component
        render(vg);

        // Render children
        for (auto& child : children) {
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

protected:
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    std::vector<Component*> children;
    bool isDragging = false;

    Component* parent = nullptr;
    Component* draggingComponent = nullptr;
};

} // namespace ppuitk