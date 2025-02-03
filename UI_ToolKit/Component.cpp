/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <utility>

#include "RootComponent.h"
#include "PopupComponent.h"
#include "CompEvent.h"

namespace pptk {

Component::~Component()
{
    stopFrameTimer();

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

void Component::addComponent(Component* child)
{
    if (child->parent)
    {
        child->removeFromParent();
    }

    child->parent = this;
    children.push_back(child);
}

void Component::setVisible(bool shouldBeVisible)
{
    visible = shouldBeVisible;
};

void Component::removeFromParent()
{
    if (parent)
    {
        auto& siblings = parent->children;

        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
        parent = nullptr;
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

void Component::startFrameTimer(std::function<void()> callback)
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


}
