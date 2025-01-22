#include "Canvas.h"
#include "Object.h"
#include "Connection.h"

Canvas::Canvas()
{
    for (int i = 0; i < 500; ++i)
    {
        objects.push_back(addComponent<Object>(std::make_unique<Object>(this, "obj_" + std::to_string(i))));
    }

    for (auto obj : objects)
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
