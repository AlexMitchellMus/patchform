//
// Created by alexw on 30/01/2025.
//
#include "ObjectMenu.h"
#include "Object.h"
#include "Canvas.h"
#include "ToolDock.h"
#include "Constants.h"
#include "../Graph/AudioGraph.h"
#include "../UI_ToolKit/CompEvent.h"
#include "CursorBitmaps.h"

Item::Item(ObjectDef def) : definition(def.definition), icon(def.icon), useIcon(def.useIcon)
{
    if (!definition.empty())
    {
        name = def.getDisplayName();
        //json j = json::parse(def.definition);
        //name = j.value<std::string>("obj", "empty");
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
    auto col = hovered ? highlight : bg;
    nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), col, nvgRGB(55, 55, 55), getHeight() * 0.5f);

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

ObjectMenuList::ObjectMenuList(Canvas* canvas, ToolDock* toolDock) : cnv(canvas), td(toolDock)
{
constexpr ObjectDef objectDef[100] = {
    { R"({"obj": "Metro"})", ICONS::Metro, true, "Metronome" },
    { R"({"obj": "Osc", "waveform": "sine", "freq": 440})", ICONS::Osc, true, "Oscillator" },
    { R"({"obj": "tableosc"})", "tosc", false, "Table Oscillator" },
    { R"({"obj": "Add"})", "add", false, "Add" },
    { R"({"obj": "mul"})", "mul", false, "Multiply" },
    { R"({"obj": "div"})", "div", false, "Divide" },
    { R"({"obj": "lfo"})", ICONS::Lfo, true, "LFO" },
    { R"({"obj": "env", "attack": 50, "decay": 50})", ICONS::Adsr, true, "Envelope" },
    { R"({"obj": "gain"})", "gain", false, "Gain" },
    { R"({"obj": "count"})", ICONS::Count, true, "Counter" },
    { R"({"obj": "dial", "min": 0, "max": 10, "value": 3})", ICONS::Dial, true, "Dial" },
    { R"({"obj": "IfElse"})", "ifelse", false, "If / Else" },
    { R"({"obj": "select", "outputs": 8 })", "sel", false, "Selector" },
    { R"({"obj": "ain"})", "ain", false, "Audio Input" },
    { R"({"obj": "aout"})", ICONS::Aout, true, "Audio Output" },
    { R"({"obj": "floatbox"})", "fb", false, "Float Box" },
    { R"({"obj": "ping", "width": 60, "height": 60})", "Png", false, "Ping" },
    { R"({"obj": "scope"})", "Scp", false, "Scope" },
    { R"({"obj": "bpf"})", "bpf", false, "Bandpass Filter" },
    { R"({"obj": "spec"})", "spec", false, "Spectrum" },
    { R"({"obj": "mtof"})", "mtof", false, "MIDI to Frequency" },
    { R"({"obj": "changed"})", "chg", false, "Change Detector" },
    { R"({"obj": "fdn"})", "fdn", false, "FDN Reverb" },
    { R"({"obj": "notein"})", "MIDIIN", false, "Note Input" },
    { R"({"obj": "get"})", "get", false, "Get Value" },
    { R"({"obj": "listbox"})", "lb", false, "List Box" },
    { R"({"obj": "pack", "values": 5 })", "pack", false, "Pack" },
    { R"({"obj": "tag"})", "tag", false, "Tag" },
    { R"({"obj": "radiobox"})", "rb", false, "Radio Box" },
    { R"({"obj": "strip"})", "strp", false, "Strip" },
    { R"({"obj": "comment"})", "com", false, "Comment" },
    { R"({"obj": "filtertag"})", "filtag", false, "Filter by Tag" },
    { R"({"obj": "activemidinotes"})", "act notes", false, "Active MIDI Notes" },
    { R"({"obj": "random"})", "rnd", false, "Random" },
    { R"({"obj": "intify", "mode": 0 })", "intify", false, "Intify" },
    { R"({"obj": "evdelay", "ms": 100 })", "evdel", false, "Event Delay" },
    { R"({"obj": "drive", "mode": 0 })", "drive", false, "Drive" },
    { R"({"obj": "loadevent" })", "ldev", false, "Load Event" },
    { R"({"obj": "specfft" })", "fft", false, "FFT" },
    { R"({"obj": "specifft" })", "ifft", false, "Inverse FFT" },
    { R"({"obj": "specmerge" })", "smerge", false, "Spectrum Merge" },
    { R"({"obj": "multitapdelay" })", "mtdel", false, "Multitap Delay" },
    { R"({"obj": "zerox" })", "zerox", false, "Zero Crossings" },
    { R"({"obj": "limiter" })", "limit", false, "Limiter" },
    { R"({"obj": "pitchdetect" })", "pdetect", false, "Pitch Detect" },
    { R"({"obj": "table", "size": 256 })", "table", false, "Table" },
    { R"({"obj": "tablexfade" })", "txf", false, "Table XFade" },
    { R"({"obj": "value" })", "val", false, "Value Holder" },
    { R"({"obj": "tablexphase" })", "txp", false, "Table XPhase" },
    { R"({"obj": "tablexspectral" })", "txs", false, "Table XSpectral" },
};


    int x = 16;
    int y = 16;
    const int paddingX = 12;
    const int paddingY = 10;
    const int maxRowWidth = 600;
    const int itemHeight = 33;
    const int minItemWidth = 40;

    for (const auto& def : objectDef)
    {
        auto item = std::make_unique<Item>(def);

        if (item->isInvalid())
            continue;

        auto objectName = def.getDisplayName();
        int textWidth = canvas->findParentOfClass<Editor>()->getTextWidthForFont("Regular", 14, objectName );
        int itemWidth = std::max(minItemWidth, 33 + textWidth + 20);

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

