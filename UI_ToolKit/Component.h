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

#include <NanoVGWrapper.h>

#include <functional>
#include <iostream>

#include "SafePointer.h"
#include "Theme.h"
#include "InputEvents.h"
#include "TileMask.h"

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

    Point operator+(const float other) const {
        return Point(x + other, y + other);
    }

    Point operator-(const Point& other) const {
        return Point(x - other.x, y - other.y);
    }

    Point operator-(const int other) const {
        return Point(x - other, y - other);
    }

    Point operator+=(const Point& other) {
        return Point(x += other.x, y += other.y);
    }

    Point operator*(float factor) const {
        return Point(x * factor, y * factor);
    }

    Point operator/(float factor) const {
        return Point(x / factor, y / factor);
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

    Point getPosition() const
    {
        return Point(x, y);
    }

    Point getSize() const
    {
        return Point(w, h);
    }

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

    Rect withHeight(int newHeight)
    {
        return Rect(x, y, w, newHeight);
    }

    Rect removeFromTop(int toRemove)
    {
        Rect result = Rect(x, y + toRemove, w, h - toRemove - y);
        return result;
    }

    Rect expanded(int toExpand) const
    {
        return Rect(x - toExpand, y - toExpand, w + 2 * toExpand, h + 2 * toExpand);
    }

    Rect removeFromBottom(int toRemove)
    {
        Rect result = Rect(x, y, w, h - toRemove);
        return result;
    }

    Rect reduced(int toReduce) const
    {
        return Rect(x + toReduce, y + toReduce, w - 2 * toReduce, h - 2 * toReduce);
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << "Rect(x: " << x << ", y: " << y << ", w: " << w << ", h: " << h << ")";
        return oss.str();
    }
};

class PopupComponent;
class SDK_EXPORT Component : public SafeObject
{
public:
    std::function<void()> onVisibilityChanged = [](){};

    explicit Component() = default;

    ~Component() override;

    void setVisible(bool shouldBeVisible);

    [[nodiscard]] bool isVisible() const { return visible; };

    void setSize(float newWidth, float newHeight)
    {
        constexpr float epsilon = 0.1f;
        if (std::abs(width - newWidth) < epsilon && std::abs(height - newHeight) < epsilon)
            return;

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
    void toBack();

    PopupComponent* getPopupComponent();
    void setPopupComponent(std::unique_ptr<PopupComponent> popupComponent);

    const std::vector<Component*>& getChildren() const
    {
        return children;
    }

    void removeAllChildren();

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
        return getPositionInParent(parent.get());
    }

    Point getPositionInParent(Component* specificParent) const
    {
        Point relativePosition(0, 0);

        const Component* current = this;

        while (current && current != specificParent)
        {
            relativePosition.x += current->x;
            relativePosition.y += current->y;
            current = current->parent.get();
        }

        if (!current) {
            throw std::runtime_error("Specified parent is not an ancestor of this component.");
        }

        return relativePosition;
    }

    Rect getGlobalBounds() const;


    Rect getAbsoluteBounds() const {
        Point absolutePosition = getAbsolutePosition();
        return pptk::Rect{absolutePosition.x, absolutePosition.y, width, height};
    }

    void setInterceptsMouseClicks(bool allowClicksOnThisComponent, bool allowClicksOnChildComponents) noexcept {
        m_allowClicksOnThisComponent = allowClicksOnThisComponent;
        m_allowClicksOnChildComponents = allowClicksOnChildComponents;
    }

    bool allowsClicksOnChildComponents() { return m_allowClicksOnChildComponents; };

    bool interceptsMouseClicks() { return m_allowClicksOnThisComponent; };

    // Hit test in local coords
    virtual bool hitTest(float px, float py) {
        if (!interceptsMouseClicks())
            return false;

        Rect expandedBounds = Rect(0, 0, width, height).expanded(externalMargin);
        return expandedBounds.contains(px, py);
    }

    virtual bool consumeEvent(CompEvent& e) { return false; };
    virtual void handleMouseMove(CompEvent& e);

    // Finds the component at global coordinate.
    // Disregards self, so make sure to call it from the Component you want to disregard from

    // Find the component from root of component tree
    Component* findComponentFromRootAt(int globalX, int globalY);

    // Find the component from the current component
    // This allows us to only search inside a layer (for example finding a port inside the objects layer)
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
            current = current->parent.get(); // Move up to the parent
        }
        return nullptr; // No parent of the specified type found
    }

    Component* getRootComponent();

    virtual void mouseEnter(CompEvent& e) { }
    virtual void mouseLeave(CompEvent& e) { }
    virtual void mouseButtonUp(CompEvent& e) { }
    virtual void mouseButtonDown(CompEvent& e) { }
    virtual void mouseMove(const Point& position) { }
    virtual void mouseDrag(const Point& position, const Point& delta, Button button) { }
    virtual void mouseWheel(CompEvent& e) { }
    virtual void keyPressed(CompEvent& e) { }
    virtual void gesture(GestureEvent& e) { }
    virtual void focusGained() { }
    virtual void focusLost() { }

    virtual void resized() { }

    virtual void computeTileCoverage(TileMaskBuffer& tileMask);

    virtual void render(NVGcontext* vg, const Theme& theme) { }
    virtual void renderAll(NVGcontext* vg, const Theme& theme);

    // Gets the actual bounds of this object inside parent
    Rect getBounds() const
    {
        return Rect{ x, y, width, height };
    }

    // Gets the local bounds (origin {0,0} )
    [[nodiscard]] virtual Rect getLocalBounds() const
    {
        return Rect{ 0, 0, width, height };
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

    virtual void setBounds(const Rect& bounds)
    {
        setBounds(bounds.x, bounds.y, bounds.w, bounds.h);
    }

    void setBounds(const float newX, const float newY, const float newW, const float newH);

    Point getPosition()
    {
        return Point(x, y);
    }

    /**
     * @brief Starts a per-frame timer callback tied to this component.
     *
     * The callback will be invoked once per frame by the RootComponent's timer system.
     * This is useful for animations or deferred updates that need to occur over time.
     *
     * @param callback A function taking (uint32_t time, uint32_t deltaTime), where:
     *        - time: the current time in milliseconds since the application started.
     *        - deltaTime: the time in milliseconds since the last frame.
     * @param timerID Optional ID to allow multiple timers per component. Default is 0.
     */
    void startFrameTimer(std::function<void(uint32_t time, uint32_t deltaTime)> callback, int timerID = 0);

    void stopFrameTimer(int timerID = 0);

    void registerGlobalMouseListener(std::function<void(Component*)> callback);

    void unregisterGlobalMouseListener();

    [[nodiscard]] Point globalToLocal(float globalX, float globalY) const;
    [[nodiscard]] Point localToGlobal(float localX, float localY) const;

    [[nodiscard]] virtual bool shouldApplyViewportOffset() const { return true; };

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

    void setName(const std::string& newName) { name = newName; }
    std::string& getName() { return name; };

    float finalX = 0.0f;
    float finalY = 0.0f;

    float viewportX = 0.0f;
    float viewportY = 0.0f;

    float scale = 1.0f;

    // nanoVG opacity doesn't work like scale/translation state
    // Each time it's set in the render loop it reset the full state
    // not the accumulated state, eg:
    // parent opacity = 0.5f : component opacity = 0.5f
    // child opacity  = 1.0f : component opacity = 1.0f

    // So we set it here as -1.0f and inside render we check if it's > 0.0f
    // If so we set the opacity
    float opacity = -1.0f;

    Component* getParent() const { return parent.get(); };

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

    void repaint();

    float getTextWidthForFont(const std::string& fontName, float size, const std::string& text);

    bool getWantsFocus() const
    {
        return wantsFocus;
    }

    void setWantsFocus(bool shouldHaveFocus)
    {
        wantsFocus = shouldHaveFocus;
    }

    void gainFocus();
    void loseFocus();

    // Provides a theme change when called, theme only exists for this call
    // It's up to the class to save the theme colours it needs
    virtual void themeChanged(const Theme& theme) {};

    void setExternalMargin(float newMargin)
    {
        externalMargin = newMargin;
    }

    std::vector<uint64_t> tileBits;

private:
    Component* findComponentAt(int globalX, int globalY, Component* selfComponent);

    void removeFromParent();

    // If true, this component intercepts clicks; otherwise, it lets clicks pass through.
    bool m_allowClicksOnThisComponent = true;

    // If true and m_allowClicksOnThisComponent is false, then child components can still be clicked.
    bool m_allowClicksOnChildComponents = true;


    // Only accessible via root component (EDITOR)
    friend class RootComponent;
    void applyThemeChange(const Theme& theme)
    {
        for (auto* child : children)
        {
            child->themeChanged(theme);
            child->applyThemeChange(theme);
        }
    }

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

    bool wantsFocus = false;

    float externalMargin = 0.0f;

    std::vector<Component*> children;
    bool isDragging = false;

    SafePointer<Component> parent;

    Component* rootComponent = nullptr;
};

} // namespace ppuitk