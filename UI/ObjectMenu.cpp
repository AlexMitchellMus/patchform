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
    constexpr ObjectDef objectDef[30] = {
        { R"({"obj": "Metro"})", ICONS::Metro, true },
        { R"({"obj": "Osc", "waveform": "sine", "freq": 440})", ICONS::Osc, true },
        { R"({"obj": "Add"})", "add", false },
        { R"({"obj": "lfo"})", ICONS::Lfo, true },
        { R"({"obj": "env", "attack": 50, "decay": 50})", ICONS::Adsr, true },
        { R"({"obj": "gain"})", "gain", false },
        { R"({"obj": "count"})", ICONS::Count, true },
        { R"({"obj": "dial", "min": 0, "max": 10, "value": 3})", ICONS::Dial, true },
        { R"({"obj": "If"})", "if", false },
        { R"({"obj": "aout"})", ICONS::Aout, true },
        { R"({"obj": "floatbox"})", "fb", false },
        { R"({"obj": "ping", "width": 60, "height": 60})", "Png", false },
        { R"({"obj": "scope"})", "Scp", false },
        { R"({"obj": "bpf"})", "bpf", false },
        { R"({"obj": "spec"})", "spec", false },
        { R"({"obj": "mtof"})", "mtof", false },
        { R"({"obj": "changed"})", "chg", false },
        { R"({"obj": "fdn"})", "fdn", false },
        { R"({"obj": "notein"})", "MIDIIN", false },
        { R"({"obj": "get"})", "get", false },
        { R"({"obj": "listbox"})", "lb", false },
        { R"({"obj": "pack", "values": 5 })", "pack", false },
        { R"({"obj": "tag"})", "tag", false },
        { R"({"obj": "radiobox"})", "rb", false },
        { R"({"obj": "strip"})", "strp", false },
        { R"({"obj": "comment"})", "com", false }
    };

    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            auto item = std::make_unique<Item>(objectDef[(i * 10) + j]);
            if (item->isInvalid())
                return;
            item->setBounds(16 + (j * (44 + 16)), 16 + (i * 55), 44, 44);

            item->onMouseUp = [this](pptk::Point position) mutable {
                if (dndObject)
                {
                    auto objectOffset = pptk::Point(dndObject->getWidth() * 0.5f * dndObject->scale, dndObject->getHeight() * 0.5f * dndObject->scale);
                    auto finalPos = position - objectOffset;
                    auto droppedPos = cnv->globalToLocal(finalPos.x, finalPos.y);

                    cnv->addFromDnDMenu(dndObject.get(), droppedPos);
                    repaint();
                    td->removeAddObjectMenu();
                }
            };

            item->onMouseDrag = [this, itemDef = item->getObjectDefinition()](pptk::Point position, const std::string& name, pptk::Point offset)
            {
                auto updateDraggedObject = [this, position, offset](Object* object) {
                    pptk::Point globalPos = localToGlobal(position.x, position.y) + offset;

                    // Correctly center the dragged object
                    auto scaledPos = globalPos -
                        pptk::Point(object->getWidth()  * 0.5f * object->scale,
                                    object->getHeight() * 0.5f * object->scale);

                    object->setPosition(scaledPos);
                };

                if (dndObject)
                {
                    updateDraggedObject(dndObject.get());
                }
                else
                {
                    auto newAudioNode = reinterpret_cast<Editor*>(getRootComponent())->graphManager->addObject(itemDef);
                    if (!newAudioNode)
                    {
                        std::cerr << "Failed to create new Audio Node." << std::endl;
                        return;
                    }
                    // Check it again - just to be super safe (this hasn't been an issue yet!)
                    if ((dndObject = newAudioNode->getOrCreateUI()))
                    {
                        dndObject->scale = cnv->scale;
                        dndObject->opacity = 0.4f;
                        getRootComponent()->addComponent(dndObject.get());
                        updateDraggedObject(dndObject.get());
                        setVisible(false);
                    } else
                    {
                        std::cerr << "Failed to create/get audio node UI!" << std::endl;
                    }
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
