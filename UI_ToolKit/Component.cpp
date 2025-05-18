/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <utility>
#include <glaze/util/string_literal.hpp>

#include "RootComponent.h"
#include "PopupComponent.h"
#include "CompEvent.h"
#include "Resizer.h"
#include "ComponentViewport.h"
#include "glaze/beve/skip.hpp"

namespace pptk {

Component::~Component()
{
    removeFromParent();
}

void Component::handleMouseMove(CompEvent& e)
{
    if (getLocalBounds().contains(e.sdlEvent.button.x, e.sdlEvent.button.y))
    {
        mouseMove(Point(e.sdlEvent.button.x, e.sdlEvent.button.y));
    }
}

Component* Component::findComponentFromRootAt(int globalX, int globalY)
{
    if (auto root = getRootComponent())
        return root->findComponentAt(globalX, globalY, this);

    return nullptr;
}

Component* Component::findComponentAt(int x, int y)
{
    return findComponentAt(x, y, this);
}

Component* Component::findComponentAt(int globalX, int globalY, Component* selfComponent)
{
    Point localPos = globalToLocal(globalX, globalY);
    // Always check children first, in reverse order for topmost components
    for (auto it = children.rbegin(); it != children.rend(); ++it)
    {
        Component* child = *it;
        if (child->isVisible())
        {
            if (dynamic_cast<ComponentViewport*>(this))
            {
                if (localPos.x < 0 || localPos.x > getWidth() || localPos.y < 0 || localPos.y > getHeight())
                {
                    continue; // Skip this child if it's outside the viewport's visible bounds
                }
            }
            // Pass the original global coordinates to the child
            Component* found = child->findComponentAt(globalX, globalY, selfComponent);
            if (found && found != selfComponent)
            {
                return found; // Return the first matching child
            }
        }
    }

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
    if (rootComponent)
    {
        return rootComponent;
    }

    // Walk parent and try to find root
    Component* current = this;
    while (current->parent)
    {
        Component* next = current->parent.get();
        if (!next)
            break; // prevent use-after-free
        current = next;
    }

    // If we find the real root, cache the root component, otherwise return nullptr
    const auto rootComp = dynamic_cast<RootComponent*>(current);
    if (rootComp)
        rootComponent = rootComp;

    // If we have not found root, then this component is not part of the component hierarchy!

    //if (!rootComp)
    //{
    //    static int c = 0;
    //    std::cerr << c++ << " Failed to find root component!" << std::endl;
    //}

    return rootComp;
}

void Component::addComponent(Component* child)
{
    if (child->parent)
    {
        child->removeFromParent();
    }

    child->parent = this;
    children.push_back(child);
    rootComponent = nullptr; // Force to refind root
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
    if (visible != shouldBeVisible)
    {
        visible = shouldBeVisible;
        onVisibilityChanged();
        repaint();
    }
};

void Component::removeFromParent()
{
    if (parent)
    {
        if (Component* p = parent.get())
        {
            auto& siblings = p->children;

            auto it = std::find(siblings.begin(), siblings.end(), this);
            if (it != siblings.end())
                siblings.erase(it);
        }

        parent = nullptr;
        rootComponent = nullptr;
    }
}

void Component::removeAllChildren()
{
    children.clear();
}

void Component::renderAll(NVGcontext* vg, const Theme& theme)
{
    auto *root = dynamic_cast<RootComponent *>(getRootComponent());
    if (!root || !root->tileMaskBuffer.isInit())
        return;

    bool intersectsDirty = false;

    const auto& localBits = tileBits.raw();
    const auto& globalBits = root->tileMaskBuffer.merged.raw();

    for (size_t i = 0; i < localBits.size(); ++i)
    {
        if (localBits[i] & globalBits[i]) {
            intersectsDirty = true;
            break;
        }
    }

    if (!intersectsDirty) {
        return;
    }

    //std::cout << "rendering: " << getName() << std::endl;

    nvgSave(vg);

    // Apply translation for this component's position
    nvgTranslate(vg, x, y);
    nvgScale(vg, scale, scale);

    if (opacity >= 0.0f)
        nvgGlobalAlpha(vg, opacity);

    // Render this component
    render(vg, theme);

    // Render children
    for (auto& child : children)
    {
        if (child->isVisible())
        {

            child->renderAll(vg, theme);
        }
    }

    nvgRestore(vg);
}

void Component::repaint()
{
    auto *root = dynamic_cast<RootComponent *>(getRootComponent());
    if (!root)
        return;

    repaintSubtree(root);
}

void Component::repaintSubtree(RootComponent *root)
{
    isDirty = true;
    root->repaintQueue.enqueue(makeSafePointer(this));

    for (auto *child: getChildren()) {
        if (child)
            child->repaintSubtree(root);
    }
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

void Component::startFrameTimer(std::function<void(uint32_t time, uint32_t deltaTime)> callback, int timerID)
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->registerTimerCallback(this, std::move(callback), timerID);
}

void Component::stopFrameTimer(int timerID)
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->unregisterTimerCallback(this, timerID);
}

void Component::registerGlobalMouseListener(std::function<void(Component*)> callback)
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->registerGlobalMouse(this, callback);
}

void Component::unregisterGlobalMouseListener()
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->unregisterGlobalMouse(this);
}

PopupComponent* Component::getPopupComponent()
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        return root->popupWindow.get();

    return nullptr;
}

void Component::setPopupComponent(std::unique_ptr<PopupComponent> popupWindow)
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->popupWindow = std::move(popupWindow);
}

void Component::gainFocus()
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->setFocusedComponent(this);
}

void Component::loseFocus()
{
    if (auto* root = dynamic_cast<RootComponent*>(getRootComponent()))
        root->setFocusedComponent(nullptr);
}

Point Component::globalToLocal(float globalX, float globalY) const
{
    // If there's a parent, first convert to the parent's local space
    if (parent)
    {
        Point parentLocal = parent->globalToLocal(globalX, globalY);
        if (shouldApplyViewportOffset()) {
            globalX = parentLocal.x + parent->viewportX;
            globalY = parentLocal.y + parent->viewportY;
        } else {
            globalX = parentLocal.x;
            globalY = parentLocal.y;
        }
    }

    // Offset by this component's position
    globalX -= x;
    globalY -= y;

    // Apply parent's scale recursively
    if (scale > 0.0f)
    {
        globalX /= scale;
        globalY /= scale;
    }

    return Point(globalX, globalY);
}

Point Component::localToGlobal(float localX, float localY) const
{
    // Apply scaling before translation
    localX *= scale;
    localY *= scale;

    // Apply this component's viewport offset (translation)
    localX += x;
    localY += y;

    // Recursively transform to parent's global coordinates
    if (parent)
    {
        Point parentGlobal = parent->localToGlobal(localX, localY);
        if (shouldApplyViewportOffset()) {
            localX = parentGlobal.x - parent->viewportX;
            localY = parentGlobal.y - parent->viewportY;
        } else {
            localX = parentGlobal.x;
            localY = parentGlobal.y;
        }
    }

    return Point(localX, localY);
}

float Component::getTextWidthForFont(const std::string& fontName, float size, const std::string& text)
{
    if (auto root = dynamic_cast<RootComponent*>(getRootComponent()))
        return root->getTextWidth(fontName, size, text);

    return -3.0f;
}

void Component::computeTileCoverage(TileMaskBuffer& tileMaskBuffer)
{
    // Resize component tile bits if there isn't enough
    // Otherwise clear them
    auto const tilesX = tileMaskBuffer.getX();
    auto const tilesY = tileMaskBuffer.getY();

    tileBits.resizeTiles(tilesX, tilesY);

    const Rect gb = getGlobalBounds();

    int minX = std::max(0, static_cast<int>(gb.x / TileMask::tileSize));
    int maxX = std::min(tilesX - 1, static_cast<int>((gb.x + gb.w) / TileMask::tileSize));
    int minY = std::max(0, static_cast<int>(gb.y / TileMask::tileSize));
    int maxY = std::min(tilesY - 1, static_cast<int>((gb.y + gb.h) / TileMask::tileSize));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            tileBits.set(x, y);
            tileMaskBuffer.current.set(x, y); // mainMask
        }
    }
}

Rect Component::getGlobalBounds() const
{
    float sx = scale;
    float sy = scale;
    float tx = x;
    float ty = y;

    const Component *p = parent.get();

    while (p) {
        tx = tx * p->scale + p->x;
        ty = ty * p->scale + p->y;
        sx *= p->scale;
        sy *= p->scale;

        if (p->shouldApplyViewportOffset()) {
            tx -= p->viewportX;
            ty -= p->viewportY;
        }

        p = p->parent.get();
    }

    return {tx, ty, getWidth() * sx, getHeight() * sy};
}

}
