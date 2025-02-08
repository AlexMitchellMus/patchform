/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <utility>

#include "RootComponent.h"
#include "PopupComponent.h"
#include "CompEvent.h"
#include "Resizer.h"

namespace pptk {

Component::~Component()
{
    // Then! Remove component
    removeFromParent();

    for (auto* child : children)
    {
        child->parent = nullptr;
    }

    children.clear();
}

void Component::mouseButtonDown(CompEvent& e)
{
    for (auto it = children.begin(); it != children.end();)
    {
        auto& child = *it;

        if (child->hitTest(e.sdlEvent.button.x, e.sdlEvent.button.y))
        {
            child->mouseButtonDown(e);
        }
        ++it;
    }
}

void Component::handleMouseMove(CompEvent& e)
{
    if (getBounds().contains(e.sdlEvent.button.x, e.sdlEvent.button.y))
    {
        mouseMove(Point(e.sdlEvent.button.x, e.sdlEvent.button.y));
    }
}

Component* Component::findComponentAt(int globalX, int globalY)
{
    return (getRootComponent())->findComponentAt(globalX, globalY, this);
}

Component* Component::findComponentAt(int globalX, int globalY, Component* selfComponent)
{
    // Always check children first, in reverse order for topmost components
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        Component* child = *it;
        if (child->isVisible())
        {
            // Pass the original global coordinates to the child
            Component* found = child->findComponentAt(globalX, globalY, selfComponent);
            if (found && found != selfComponent)
            {
                return found; // Return the first matching child
            }
        }
    }

    Point localPos = globalToLocalWithScale(globalX, globalY);

    // Check this component only after its children
    if (hitTest(localPos.x, localPos.y))
    {
        return this;
    }

    // No matching component found
    return nullptr;
}

Component* Component::getRootComponent()
{
    // Return the cached root if available.
    if (rootComponent != nullptr)
        return rootComponent;

    Component* current = this;
    // Traverse upward until no parent exists.
    while (current->parent != nullptr)
    {
        current = current->parent;
    }
    // Cache the computed root.
    rootComponent = current;
    return current;
}

void Component::addComponent(Component* child)
{
    if (child->parent)
    {
        child->removeFromParent();
    }

    child->parent = this;
    children.push_back(child);
    child->rootComponent = rootComponent;
    child->resized();

    if (auto resizibleChild = dynamic_cast<ResizableComponent*>(child))
    {
        children.push_back(&resizibleChild->getResizer());
    }
}

void Component::toBack()
{
    // If the component has no parent, there's nothing to do.
    if (!parent)
        return;

    // Get a reference to the parent's children vector.
    auto& siblings = parent->children;

    // Find and remove this component from the siblings.
    auto it = std::find(siblings.begin(), siblings.end(), this);
    if (it != siblings.end())
    {
        siblings.erase(it);
        // Insert at the beginning so it is drawn first (at the back).
        siblings.insert(siblings.begin(), this);
    }

    // If this component is a ResizableComponent, also move its resizer.
    if (auto resizable = dynamic_cast<ResizableComponent*>(this))
    {
        Component* resizer = &resizable->getResizer();
        auto itResizer = std::find(siblings.begin(), siblings.end(), resizer);
        if (itResizer != siblings.end())
        {
            siblings.erase(itResizer);
            siblings.insert(siblings.begin(), resizer);
        }
    }
}

void Component::setVisible(bool shouldBeVisible)
{
    visible = shouldBeVisible;
    onVisibilityChanged();
};

void Component::removeFromParent()
{
    if (parent)
    {
        auto& siblings = parent->children;

        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        parent = nullptr;
        // FIXME: not sure if we should or shouldn't do this, leave it out for now
        //rootComponent = nullptr;
    }
}

void Component::renderAll(NVGcontext* vg)
{
    nvgSave(vg);

    // Apply translation for this component's position
    nvgTranslate(vg, x, y);
    nvgScale(vg, scale, scale);

    nvgGlobalAlpha(vg, opacity);

    // Render this component
    render(vg);

    // Render children
    for (auto& child : children)
    {
        if (child->isVisible())
            child->renderAll(vg);
    }

    nvgRestore(vg);
}

void Component::repaint()
{
    isDirty = true;

    // Set all parents dirty in this branch
    if (parent)
    {
        parent->repaint();
    }
}

bool Component::needsRepaint()
{
    auto root = getRootComponent();
    if (root->isDirty)
    {
        root->isDirty = false;
        return true;
    }
    return false;
}

void Component::setBounds(const float newX, const float newY, const float newW, const float newH)
{
    float clampedW = newW;
    float clampedH = newH;

    if (minWidth > 0) clampedW = std::max(minWidth, clampedW);
    if (minHeight > 0) clampedH = std::max(minHeight, clampedH);
    if (maxWidth > 0) clampedW = std::min(maxWidth, clampedW);
    if (maxHeight > 0) clampedH = std::min(maxHeight, clampedH);

    if (width != clampedW || height != clampedH || x != newX || y != newY)
    {
        width = clampedW;
        height = clampedH;
        x = newX;
        y = newY;

        resized();
        repaint();
    }
}

void Component::setPosition(float newX, float newY)
{
    if (x != newX || y != newY)
    {
        x = newX;
        y = newY;

        repaint();
    }
}

void Component::setPosition(const Point& point)
{
    setPosition(point.x, point.y);
}

void Component::startFrameTimer(std::function<void(uint32_t)> callback)
{
    reinterpret_cast<RootComponent*>(getRootComponent())->registerTimerCallback(this, std::move(callback));
}

void Component::stopFrameTimer()
{
    reinterpret_cast<RootComponent*>(getRootComponent())->unregisterTimerCallback(this);
}

void Component::registerGlobalMouseListener(std::function<void(Component*)> callback)
{
    reinterpret_cast<RootComponent*>(getRootComponent())->registerGlobalMouse(this, callback);
}

void Component::unregisterGlobalMouseListener()
{
    reinterpret_cast<RootComponent*>(getRootComponent())->unregisterGlobalMouse(this);
}

PopupComponent* Component::getPopupComponent()
{
    return reinterpret_cast<RootComponent*>(getRootComponent())->popupWindow.get();
}

void Component::setPopupComponent(std::unique_ptr<PopupComponent> popupWindow)
{
    reinterpret_cast<RootComponent*>(getRootComponent())->popupWindow = std::move(popupWindow);
}

Point Component::globalToLocalWithScale(float globalX, float globalY) const
{
    // If there's a parent, first convert to the parent's local space
    if (parent)
    {
        // First, transform into parent's coordinate space
        Point parentLocal = parent->globalToLocalWithScale(globalX, globalY);
        globalX = parentLocal.x;
        globalY = parentLocal.y;
    }

    // Offset by this component's position
    globalX -= x;
    globalY -= y;

    // **Apply parent's scale recursively**
    if (scale != 1.0f && scale > 0.0f)
    {
        globalX /= scale;
        globalY /= scale;
    }

    return Point(globalX, globalY);
}

Point Component::globalToLocal2(float globalX, float globalY) const
{
    // Recursively transform to parent's local coordinates
    if (parent)
    {
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

Point Component::globalToLocal(float globalX, float globalY) const
{
    // Recursively transform to parent's local coordinates
    if (parent)
    {
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

Point Component::localToGlobal(float localX, float localY) const
{
    // Apply scaling before translation
    localX *= scale;
    localY *= scale;

    // Apply this component's viewport offset (translation)
    localX += x + viewportX;
    localY += y + viewportY;

    // Recursively transform to parent's global coordinates
    if (parent)
    {
        return parent->localToGlobal(localX, localY);
    }

    return Point(localX, localY);
}

float Component::getTextWidthForFont(const std::string& fontName, float size, const std::string& text)
{
    if (auto root = dynamic_cast<RootComponent*>(getRootComponent()))
    {
        return root->getTextWidth(fontName, size, text);
    }
    return -3.0f;
}

}
