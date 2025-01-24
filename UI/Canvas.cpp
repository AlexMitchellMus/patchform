#include "Canvas.h"
#include "Object.h"
#include "Connection.h"
#include "Lasso.h"

Canvas::Canvas(Component* parent) : pptk::Component(parent)
{
    lasso = std::make_unique<Lasso>(this);
    addComponent(lasso.get());

    for (int i = 0; i < 400; ++i)
    {
        auto obj = std::make_unique<Object>(this, "obj_" + std::to_string(i));
        addComponent(obj.get());
        objects.push_back(std::move(obj));
    }

    for (const auto& obj : objects)
    {
        obj->setPosition(std::rand() % 800, std::rand() % 800);
    }
}

void Canvas::mouseButtonDown(SDL_Event& e)
{
    clearSelection();

    if (e.button.button == SDL_BUTTON_LEFT)
    {
        lasso->start({e.button.x, e.button.y});
    }
}

void Canvas::mouseButtonUp(SDL_Event& e)
{
    lasso->end();
}

void Canvas::mouseDrag(const pptk::Point& position, const pptk::Point& delta)
{
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

void Canvas::addToSelection(Object* obj)
{
    if (std::find(selected.begin(), selected.end(), obj) == selected.end()) // Avoid duplicates
    {
        obj->setSelected(true);
        selected.push_back(obj);
    }
}

void Canvas::removeFromSelection(Object* obj)
{
    auto it = std::find(selected.begin(), selected.end(), obj);
    if (it != selected.end())
    {
        obj->setSelected(false);
        selected.erase(it);
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
}

void Canvas::clearSelection()
{
    for (auto& obj : selected)
    {
        obj->setSelected(false);
    }

    selected.clear();
}


void Canvas::render(NVGcontext* nvg)
{
    nvgBeginPath(nvg);
    nvgFillColor(nvg, nvgRGB(23, 23, 23));
    nvgFillRect(nvg, x, y, width, height);
}

void Canvas::renderAll(NVGcontext* nvg)
{
    nvgSave(nvg);

    auto offsetPos = parent->getBounds();
    nvgTranslate(nvg, offsetPos.x, offsetPos.y);

    // Render the background
    render(nvg);

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
    nvgRestore(nvg);
}

void Canvas::removeObject(Object* obj)
{
    // Remove the object from the `selected` list if it exists there
    auto it = std::find(selected.begin(), selected.end(), obj);
    if (it != selected.end()) {
        selected.erase(it);
    }

    // Remove the object from the `objects` list by comparing raw pointers
    auto objIt = std::find_if(objects.begin(), objects.end(),
        [obj](const std::unique_ptr<Object>& uniqueObj) {
            return uniqueObj.get() == obj;
        });
    if (objIt != objects.end()) {
        objects.erase(objIt);
    }
}
