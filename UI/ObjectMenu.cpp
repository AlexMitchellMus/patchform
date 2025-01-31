//
// Created by alexw on 30/01/2025.
//
#include "ObjectMenu.h"
#include "Object.h"
#include "Canvas.h"
#include "ToolDock.h"
#include "Constants.h"

ObjectMenu::ObjectMenu(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
    ObjectDef objectDef[10] = {
        {"metro", ICONS::Metro},
        {"osc", ICONS::Osc},
        {"lfo", ICONS::Lfo},
        {"adsr", ICONS::Adsr},
        {"count", ICONS::Count},
        {"print", ICONS::Print},
        {"aout", ICONS::Aout}
    };

    for (int i = 0; i < 10; i++)
    {
        auto item = std::make_unique<Item>(objectDef[i]);
        item->setBounds(16 + (i * (44 + 16)), 16, 44, 44);

        item->onMouseUp = [this](Point position) mutable {
            if (dndObject)
            {
                auto droppedPos = cnv->globalToLocalWithScale(position.x, position.y);
                std::cout << "dropping object at: " << droppedPos.toString() << std::endl;
                cnv->addObject(dndObject.get(), droppedPos);
                dndObject.reset();
                repaint();
                td->removeAddObjectMenu();
            }
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
                dndObject->opacity = 0.4f;
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
