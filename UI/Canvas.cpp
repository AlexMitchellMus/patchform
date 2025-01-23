#include "Canvas.h"
#include "Object.h"
#include "Connection.h"

Canvas::Canvas(Component* parent) : pptk::Component(parent)
{
    for (int i = 0; i < 100; ++i)
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

    // Restore previous transformation
    nvgRestore(nvg);
}
