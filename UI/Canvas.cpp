/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Canvas.h"
#include "Object.h"
#include "Connection.h"
#include "Lasso.h"

Canvas::Canvas()
{
}

std::vector<Object*> Canvas::getObjects() const
{
    std::vector<Object*> objs;
    objs.reserve(objects.size());

    for (auto& obj : objects)
    {
        objs.push_back(obj.get());
    }

    return objs;
};

void Canvas::mouseButtonDown(SDL_Event& e)
{
    if (e.button.button == SDL_BUTTON_LEFT)
    {
        clearSelection();

        lasso = std::make_unique<Lasso>(Point(e.button.x, e.button.y));   //lasso->start({e.button.x, e.button.y});
        addComponent(lasso.get());
    }
    else if (e.button.button == SDL_BUTTON_MIDDLE) {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));
    }
}

void Canvas::mouseButtonUp(SDL_Event& e)
{
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    lasso.reset();
}

void Canvas::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button)
{
    if (button == pptk::Button::LEFT)
    {
        lasso->update(position);

        auto lassoBounds = lasso->getLassoBounds();

        for (const auto& obj : objects)
        {
            if (lassoBounds.intersects(obj->getBounds()))
            {
                addToSelection(obj.get());
            }
            else
            {
                removeFromSelection(obj.get());
            }
        }
    }
    else if (button == pptk::Button::MIDDLE)
    {
        auto scale = getAccumulatedScale();
        x += delta.x * scale;
        y += delta.y * scale;

        repaint();
    }
}

void Canvas::mouseWheel(SDL_Event& e)
{
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Translate mouse position to canvas coordinates
    float canvasMouseX = (mouseX - x) / scale;
    float canvasMouseY = (mouseY - y) / scale;

    // Adjust scale with constraints
    float newScale = scale + e.wheel.y * 0.125f;
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);

    // Adjust canvas offset to scale around the mouse point
    x -= canvasMouseX * (newScale - scale);
    y -= canvasMouseY * (newScale - scale);

    // Apply the new scale
    scale = newScale;

    onScaleChange(scale);

    repaint();
}

void Canvas::setScale(float offset)
{
    float newScale = scale + (offset * 0.001f);
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);
    scale = newScale;

    onScaleChange(scale);
    repaint();
}

void Canvas::resetScale()
{
    scale = 1.0f;
    onScaleChange(scale);
    repaint();
}

void Canvas::keyPressed(SDL_Event& e)
{
    if (e.key.key == SDLK_DELETE || e.key.key == SDLK_BACKSPACE)
    {
        deleteSelectedObjects();
    }
}

void Canvas::deleteSelectedObjects()
{
    selected.clear();

    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [](const std::unique_ptr<Object>& obj) {
            return obj->getIsSelected(); // Only remove the objects that are currently selected
        }),
        objects.end());

    callOjbectChangedListeners();

    repaint();
}

void Canvas::addToSelection(Object* obj)
{
    if (std::find(selected.begin(), selected.end(), obj) == selected.end()) // Avoid duplicates
    {
        obj->setSelected(true);
        selected.push_back(obj);

        callOjbectChangedListeners();
    }

    repaint();
}

void Canvas::removeFromSelection(Object* obj)
{
    auto it = std::find(selected.begin(), selected.end(), obj);
    if (it != selected.end())
    {
        obj->setSelected(false);
        selected.erase(it);

        callOjbectChangedListeners();
    }

    repaint();
}

bool Canvas::areMultiObjectsSelected()
{
    return selected.size() > 1;
}

void Canvas::setMultiObjectPosition(pptk::Point pos)
{
    for (auto& obj : selected)
    {
        obj->setPosition(obj->getPosition() + pos);
    }

    updateConnectionsPosition();
}

void Canvas::updateConnectionsPosition() const
{
    for (auto& con : connections)
    {
        con->updateConnectionGeometry();
    }
}

void Canvas::removeConnectionsFor(Object* target)
{
    // TODO: Implement a SmartPointer system so we can give each object a list of connections, which will become null when removed
    auto it = connections.begin();
    while (it != connections.end())
    {
        if ((*it)->getOriginPort()->getParent() == target ||
            (*it)->getDestPort()->getParent() == target)
        {
            it = connections.erase(it); // Erases and moves iterator to next element
        }
        else
        {
            ++it;
        }
    }
}

void Canvas::setSelected(Object* obj)
{
    clearSelection();

    obj->setSelected(true);

    selected.push_back(obj);

    callOjbectChangedListeners();

    repaint();
}

void Canvas::clearSelection()
{
    for (auto& obj : selected)
    {
        obj->setSelected(false);
    }

    selected.clear();

    callOjbectChangedListeners();

    repaint();
}


void Canvas::render(NVGcontext* nvg)
{
    // Draw Background color
    nvgBeginPath(nvg);
    nvgFillColor(nvg, nvgRGB(23, 23, 23));
    nvgFillRect(nvg, 0, 0, width, height);

    // Draw bg lines
    nvgBeginPath(nvg);
    nvgLineStyle(nvg, NVG_SOLID);
    nvgStrokeColor(nvg, nvgRGB(33, 33, 33)); // Set stroke color

    // Draw vertical dashed lines
    for (float x = 0; x <= infinteCanvasSize; x += 100)
    {
        nvgMoveTo(nvg, x, 0);
        nvgLineTo(nvg, x, infinteCanvasSize);
    }

    // Draw horizontal dashed lines
    for (float y = 0; y <= infinteCanvasSize; y += 100)
    {
        nvgMoveTo(nvg, 0, y);
        nvgLineTo(nvg, infinteCanvasSize, y);
    }

    nvgStroke(nvg);

    // Draw dashed origin lines
    nvgBeginPath(nvg);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, infinteCanvasSize, canvasOrigin);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, canvasOrigin, infinteCanvasSize);

    nvgStrokeColor(nvg, nvgRGB(43, 43, 43)); // Set stroke color
    nvgStrokeWidth(nvg, 2.0f);   // Set line width
    nvgDashLength(nvg, 10.0f);
    nvgLineStyle(nvg, NVG_LINE_DASHED);
    nvgStroke(nvg);
}

void Canvas::renderAll(NVGcontext* nvg)
{
    nvgSave(nvg);

    nvgTranslate(nvg, x, y);
    nvgScale(nvg, scale, scale);

    // Render the background
    render(nvg);

    for (auto const& obj : objects)
    {
        // Objects are widgets that have children
        // So we need to render child components
        obj->renderAll(nvg);
    }

    if (newConnection)
    {
        nvgSave(nvg);
        nvgTranslate(nvg, newConnection->getX(), newConnection->getY());

        newConnection->render(nvg);

        nvgRestore(nvg);
    }

    for (auto const& con : connections)
    {
        // Connections have no child components
        nvgSave(nvg);
        nvgTranslate(nvg, con->getX(), con->getY());

        con->render(nvg);

        nvgRestore(nvg);
    }

    if (lasso)
    {
        nvgSave(nvg);
        nvgTranslate(nvg, lasso->getX(), lasso->getY());

        lasso->render(nvg);

        nvgRestore(nvg);
    }

    // Restore previous transformation
    nvgRestore(nvg);
}

void Canvas::addObject(Object* toAdd, Point position)
{
    auto object = std::make_unique<Object>(toAdd->getName());

    addComponent(object.get());
    object->setPosition(position);

    setSelected(object.get());

    objects.push_back(std::move(object));

    callOjbectChangedListeners();
}

void Canvas::addConnection(Port* origin, Port* dest)
{
    auto connection = std::make_unique<Connection>(origin, dest);

    addComponent(connection.get());

    connection->updateConnectionGeometry();

    connections.push_back(std::move(connection));
}

void Canvas::addObjectChangedListener(std::function<void()> callback)
{
    objectChangedListeners.push_back(std::move(callback));
}

void Canvas::removeObjectChangedListener(std::function<void()> callback)
{
    auto it = std::find_if(objectChangedListeners.begin(), objectChangedListeners.end(),
        [&callback](const std::function<void()>& listener) {
            return listener.target_type() == callback.target_type();
        });

    if (it != objectChangedListeners.end()) {
        objectChangedListeners.erase(it);
    }
}

void Canvas::callOjbectChangedListeners()
{
    for (auto& objChangeListener : objectChangedListeners)
    {
        objChangeListener();
    }
}