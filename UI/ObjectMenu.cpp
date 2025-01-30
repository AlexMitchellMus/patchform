//
// Created by alexw on 30/01/2025.
//
#include "ObjectMenu.h"
#include "Object.h"
#include "Canvas.h"
#include "ToolDock.h"

ObjectMenu::ObjectMenu(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
    bg = nvgRGB(43, 43, 43);
    outline = nvgRGB(53, 53, 53);

    std::string names[10] = {"osc", "env", "if", "aout", "value", "value", "value", "value", "value", "value"};

    for (int i = 0; i < 10; i++)
    {
        auto item = std::make_unique<Item>(names[i]);
        item->setBounds(16 + (i * (44 + 16)), 16, 44, 44);

        item->onMouseUp = [this]() mutable {
            std::cout << "deleting object menu" << std::endl;
            dndObject.reset();
            repaint();
            td->removeAddObjectMenu();
        };

        item->onMouseDrag = [this](Point position, const std::string& name, Point offset)
        {
            if (dndObject)
            {
                auto scaledOffset = Point(offset.x * scale, offset.y * scale);
                Point globalPos = localToGlobal(position.x, position.y) + offset;
                dndObject->setPosition(globalPos.x - dndObject->getWidth() / 2, globalPos.y - dndObject->getHeight() / 2);
            }
            else
            {
                dndObject = std::make_unique<Object>(name);
                dndObject->scale = cnv->scale;
                dndObject->opacity = 0.7f;
                getRootComponent()->addComponent(dndObject.get());
                setVisible(false);
            }
        };

        addComponent(item.get());
        items.push_back(std::move(item));
    }

    repaint();
};

ObjectMenu::~ObjectMenu()
{
    std::cout << "deleting object menu" << std::endl;
}
