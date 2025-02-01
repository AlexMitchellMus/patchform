//
// Created by alexw on 30/01/2025.
//
#include "ObjectMenu.h"
#include "Object.h"
#include "Canvas.h"
#include "ToolDock.h"
#include "Constants.h"
#include "../Graph/AudioGraph.h"

ObjectMenu::ObjectMenu(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
    ObjectDef objectDef[10] = {
        { {{"obj", "Metro"}, {"hz", 8}}, ICONS::Metro},
        { {{"obj", "Metro"}, {"hz", 1}}, ICONS::Metro},
        { {{"obj", "Osc"}, {"waveform", "sine"}, {"freq",  660}}, ICONS::Osc},
        { {{"obj", "Osc"}, {"waveform", "saw"}, {"freq",  330}}, ICONS::Osc},
        { {{"obj", "lfo"}}, ICONS::Lfo},
        { {{"obj", "env"}, {"attack", 100}, {"decay", 100}}, ICONS::Adsr},
        { {{"obj", "env"}, {"attack", 500}, {"decay", 500}}, ICONS::Adsr},
        { {{"obj", "count"}}, ICONS::Count},
        { {{"obj", "print"}}, ICONS::Print},
        { {{"obj", "aout"}}, ICONS::Aout}
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
                cnv->addFromDnDMenu(dndObject.get(), droppedPos);
                repaint();
                td->removeAddObjectMenu();
            }
        };

        item->onMouseDrag = [this, itemDef = item->getObjectDefinition()](Point position, const std::string& name, Point offset)
        {
            if (dndObject)
            {
                auto scaledOffset = Point(offset.x * scale, offset.y * scale);
                Point globalPos = localToGlobal(position.x, position.y) + offset;
                dndObject->setPosition(globalPos.x - dndObject->getWidth() / 2, globalPos.y - dndObject->getHeight() / 2);
            }
            else
            {
                auto newAudioNode = reinterpret_cast<App*>(getRootComponent())->graphManager->addObject(itemDef);
                dndObject = newAudioNode->createUI_Raw();
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
