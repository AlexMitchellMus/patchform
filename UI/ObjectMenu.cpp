//
// Created by alexw on 30/01/2025.
//
#include "ObjectMenu.h"
#include "Object.h"
#include "Canvas.h"
#include "ToolDock.h"
#include "../Graph/AudioGraph.h"
#include "../UI_ToolKit/CompEvent.h"
#include "CursorBitmaps.h"

using namespace ObjectMenuDefs;

Item::Item(ObjectDef def) : definition(def.definition), icon(def.icon), useIcon(def.useIcon)
{
    if (!definition.empty())
    {
        name = def.getDisplayName();
        tint = def.tint;
    }
}

void Item::mouseButtonUp(pptk::CompEvent& e)
{
    onMouseUp(pptk::Point(e.sdlEvent.button.x, e.sdlEvent.button.y));
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
}

void Item::mouseEnter(pptk::CompEvent& e)
{
    hovered = true;
    SDL_Cursor* grab = CursorBitmaps::create(CursorType::GrabWhite);
    SDL_SetCursor(grab);
    repaint();
}

void Item::mouseLeave(pptk::CompEvent& e)
{
    hovered = false;
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    repaint();
}

void Item::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button)
{
    SDL_Cursor* grab = CursorBitmaps::create(CursorType::GrabbingWhite);
    SDL_SetCursor(grab);
    onMouseDrag(position, name, getPositionInParent());
}

void Item::render(NVGcontext* vg)
{
    nvgBeginPath(vg);
    NVGcolor fill = tint;
    fill.a = hovered ? 25 : 10;

    nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), fill, nvgRGB(55, 55, 55), getHeight() * 0.5f);

    if (useIcon)
    {
        nvgFontSize(vg, 28.0f);
        nvgFontFace(vg, "object_icons");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(220, 220, 220));
        nvgText(vg, 6, getHeight() * 0.5f - 3.0f, icon.c_str(), nullptr);
    }

    nvgFontSize(vg, 14.0f);
    nvgFontFace(vg, "Regular");
    nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
    nvgFillColor(vg, nvgRGB(220, 220, 220));
    nvgText(vg, getWidth() - 10, getHeight() * 0.5f, name.c_str(), nullptr);
}

struct CategoryBlock {
    const char* categoryName;
    const ObjectMenuDefs::ObjectDef* items;
    size_t itemCount;
};

#define COUNT_OF(arr) (sizeof(arr) / sizeof(arr[0]))

ObjectMenuList::ObjectMenuList(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
    constexpr CategoryBlock objectMenu[] = {
        { "Control", ControlItems, COUNT_OF(ControlItems) },
        { "UI", UIItems, COUNT_OF(UIItems) },
        { "IO", IOItems, COUNT_OF(IOItems) },
        { "Maths", MathsItems, COUNT_OF(MathsItems) },
        { "Oscillator", OscillatorItems, COUNT_OF(OscillatorItems) },
        { "Effect", EffectItems, COUNT_OF(EffectItems) },
        { "Spectral", SpectralItems, COUNT_OF(SpectralItems) },
        { "Wavetable", WavetableItems, COUNT_OF(WavetableItems) },
    };

    float x = 16;
    float y = 18;
    constexpr int paddingX = 12;
    constexpr int paddingY = 10;
    constexpr int maxRowWidth = 605;
    constexpr int itemHeight = 33;

    for (const auto& block : objectMenu)
    {
        // Store the category header position
        categoryHeaders.push_back({ block.categoryName, {x, y + 10} }); // +16 for vertical centering

        NVGcolor tint;

        switch (hash(block.categoryName))
        {
        case hash("UI"):
            tint = nvgRGB(90, 160, 200);        // Teal Blue
            break;
        case hash("IO"):
            tint = nvgRGB(220, 100, 100);       // Warm Red
            break;
        case hash("Control"):
            tint = nvgRGB(220, 200, 60);        // Gold
            break;
        case hash("Oscillator"):
            tint = nvgRGB(80, 220, 220);        // Bright Cyan
            break;
        case hash("Effect"):
            tint = nvgRGB(200, 100, 220);       // Electric Purple
            break;
        case hash("Spectral"):
            tint = nvgRGB(100, 200, 160);       // Soft Aqua
            break;
        case hash("Wavetable"):
            tint = nvgRGB(80, 160, 100);        // Deep Green
            break;
        case hash("Maths"):
            tint = nvgRGB(240, 150, 50);        // Orange
            break;
        default:
            tint = nvgRGB(44, 44, 44);          // Fallback Dark Grey
            break;
        }

        y += 26;
        x = 16;

        for (size_t i = 0; i < block.itemCount; ++i)
        {
            const auto& def = block.items[i];

            def.tint = tint;
            auto item = std::make_unique<Item>(def);

            if (item->isInvalid())
                continue;

            int textWidth = canvas->findParentOfClass<Editor>()->getTextWidthForFont("Regular", 14, def.getDisplayName());
            int itemWidth = std::max(40, 33 + textWidth + 20);

            if (x + itemWidth > maxRowWidth)
            {
                x = 16;
                y += itemHeight + paddingY;
            }

            item->setBounds(x, y, itemWidth, itemHeight);
            x += itemWidth + paddingX;

            addComponent(item.get());
            items.push_back(std::move(item));
        }

        y += itemHeight + paddingY * 2;
        x = 16;
    }

    // Set lambdas for each item
    for (auto& item : items)
    {
        item->onMouseUp = [this](pptk::Point position) mutable
        {
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

        item->onMouseDrag = [this, itemDef = item->getObjectDefinition()](
            pptk::Point position, const std::string& name, pptk::Point offset)
            {
                auto updateDraggedObject = [this, position, offset](Object* object)
                {
                    pptk::Point globalPos = localToGlobal(position.x, position.y) + offset;

                    // Correctly center the dragged object
                    auto scaledPos = globalPos -
                        pptk::Point(object->getWidth() * 0.5f * object->scale,
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

                        findParentOfClass<ObjectMenu>()->setVisible(false);
                    }
                    else
                    {
                        std::cerr << "Failed to create/get audio node UI!" << std::endl;
                    }
                }
            };
    }
    setBounds(0, 0, maxRowWidth, y + itemHeight + paddingY);
    repaint();
};

void ObjectMenuList::render(NVGcontext* vg)
{
    // Render category headers
    for (const auto& header : categoryHeaders)
    {
        nvgFontSize(vg, 14.0f); // Slightly larger than item text
        nvgFontFace(vg, "SemiBold"); // Or "Bold" if available
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        nvgFillColor(vg, nvgRGB(200, 200, 200)); // Light grey text
        nvgText(vg, header.position.x, header.position.y, header.name, nullptr);
    }

    // Let children (items) render themselves
    Component::render(vg);
}

ObjectMenuView::ObjectMenuView(Canvas* canvas, ToolDock* toolDock)
{
    auto list = std::make_unique<ObjectMenuList>(canvas, toolDock);
    auto listHeight = list->getHeight();
    list->setBounds(0, 0, 600, listHeight);

    setViewport(std::move(list));
    setBounds(0, 0, 620, 300); // initial size
}

ObjectMenu::ObjectMenu(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
    viewport = std::make_unique<ObjectMenuView>(canvas, toolDock);
    viewport->setBounds(10, 10, 620, 300); // adjust as needed
    addComponent(viewport.get());
}

