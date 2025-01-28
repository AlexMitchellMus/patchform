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
    lasso = std::make_unique<Lasso>();
    addComponent(lasso.get());

    for (int i = 0; i < 1000; ++i)
    {
        auto obj = std::make_unique<Object>("obj_" + std::to_string(i));
        addComponent(obj.get());
        objects.push_back(std::move(obj));
    }

    for (const auto& obj : objects)
    {
        obj->setPosition((std::rand() % 8000) + canvasOrigin, (std::rand() % 8000) + canvasOrigin);
    }
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
    clearSelection();

    if (e.button.button == SDL_BUTTON_LEFT)
    {
        lasso->start({e.button.x, e.button.y});
    }
    else if (e.button.button == SDL_BUTTON_MIDDLE) {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));
    }
}

void Canvas::mouseButtonUp(SDL_Event& e)
{
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    lasso->end();
}

void Canvas::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button)
{
    if (button == pptk::Button::LEFT)
    {
        std::cout << "left:" << std::endl;
        lasso->update(position);

        auto lassoBounds = lasso->getLassoBounds();

        for (const auto& obj : objects)
        {
            if (lassoBounds.intersects(obj->getAbsoluteBounds()))
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
        viewportX += delta.x;
        viewportY += delta.y;
    }
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
}

void Canvas::addToSelection(Object* obj)
{
    if (std::find(selected.begin(), selected.end(), obj) == selected.end()) // Avoid duplicates
    {
        obj->setSelected(true);
        selected.push_back(obj);

        callOjbectChangedListeners();
    }
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
}

void Canvas::setSelected(Object* obj)
{
    clearSelection();

    obj->setSelected(true);

    selected.push_back(obj);

    callOjbectChangedListeners();
}

void Canvas::clearSelection()
{
    for (auto& obj : selected)
    {
        obj->setSelected(false);
    }

    selected.clear();

    callOjbectChangedListeners();
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

    nvgTranslate(nvg, finalX, finalY);

    // Render the background
    render(nvg);

    nvgRestore(nvg);

    for (auto const& obj : objects)
    {
        // Objects are widgets that have children
        // So we need to render child components
        obj->renderAll(nvg);
    }

    if (newConnection)
        newConnection->render(nvg);

    for (auto const& con : connections)
    {
        // Connections have no child components
        con->render(nvg);
    }

    lasso->render(nvg);

    // Restore previous transformation

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

void Canvas::updateLayout()
{
    finalX = parent->finalX + x + viewportX;
    finalY = parent->finalY + y + viewportY;

    for (auto& obj : objects)
    {
        // If obj is also derived from Component, it may do
        //   obj->finalX = this->finalX + obj->x;
        //   obj->finalY = this->finalY + obj->y;
        // inside obj->updateLayout(). So just call it:
        if (obj)
            obj->updateLayout();
    }

    if (newConnection)
        newConnection->updateLayout();

    if (lasso)
        lasso->updateLayout();
}