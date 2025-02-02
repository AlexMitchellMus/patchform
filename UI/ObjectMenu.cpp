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
    ObjectDef objectDef[20] = {
        { {{"obj", "Metro"}, {"hz", 8}}, ICONS::Metro},
        { {{"obj", "Metro"}, {"hz", 1}}, ICONS::Metro},
        { {{"obj", "Osc"}, {"waveform", "sine"}, {"freq",  440}}, ICONS::Osc},
        { {{"obj", "Add"}}, "add", false },
        { {{"obj", "lfo"}}, ICONS::Lfo},
        { {{"obj", "env"}, {"attack", 50}, {"decay", 50}}, ICONS::Adsr},
        { {{"obj", "vol"}}, "vol", false },
        { {{"obj", "count"}}, ICONS::Count},
        { {{"obj", "dial"}}, ICONS::Dial},
        { {{"obj", "aout"}}, ICONS::Aout},
        { {{"obj", "floatbox"}}, "fb", false },
    };

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            auto item = std::make_unique<Item>(objectDef[(i * 10) + j]);
            item->setBounds(16 + (j * (44 + 16)), 16 + (i * 55), 44, 44);

            item->onMouseUp = [this](Point position) mutable {
                if (dndObject)
                {
                    auto objectOffset = Point(dndObject->getWidth() * 0.5f * dndObject->scale, dndObject->getHeight() * 0.5f * dndObject->scale);
                    auto finalPos = position - objectOffset;
                    auto droppedPos = cnv->globalToLocalWithScale(finalPos.x, finalPos.y);

                    cnv->addFromDnDMenu(dndObject.get(), droppedPos);
                    repaint();
                    td->removeAddObjectMenu();
                }
            };

            item->onMouseDrag = [this, itemDef = item->getObjectDefinition()](Point position, const std::string& name, Point offset)
            {
                if (dndObject)
                {
                    Point globalPos = localToGlobal(position.x, position.y) + offset;

                    // Correctly center the dragged object
                    auto scaledPos = globalPos -
                                     Point(dndObject->getWidth() * 0.5f * dndObject->scale,
                                           dndObject->getHeight() * 0.5f * dndObject->scale);

                    dndObject->setPosition(scaledPos);
                }
                else
                {
                    auto newAudioNode = reinterpret_cast<App*>(getRootComponent())->graphManager->addObject(itemDef);
                    if (!newAudioNode)
                    {
                        std::cerr << "Failed to create new Audio Node." << std::endl;
                        return;
                    }
                    dndObject = newAudioNode->getOrCreateUI();
                    dndObject->scale = cnv->scale;
                    dndObject->opacity = 0.4f;
                    getRootComponent()->addComponent(dndObject.get());
                    setVisible(false);
                }
            };

            addComponent(item.get());
            items.push_back(std::move(item));
        }
    }

    repaint();
};

ObjectMenu::~ObjectMenu()
{

}
