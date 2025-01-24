#include "LeftPanel.h"
#include "SDL3/SDL.h"
#include "nanovg.h"
#include "Object.h"

LeftPanel::LeftPanel(Component* parent, Canvas* canvas)
    : Component(parent), cnv(canvas)
{
    setMinMaxSize(100, 400, 0, 0);

    cnv->addObjectChangedListener([this]()
    {
        updateCanvasObjectList();
    });

    updateCanvasObjectList();
}

void LeftPanel::updateCanvasObjectList()
{
    objectList.clear();

    for (auto obj : cnv->getObjects())
    {
        objectList.push_back( { obj->getName(), obj->getIsSelected() } );
    }
}

void LeftPanel::render(NVGcontext* nvg)
{
    auto selectedCol = nvgRGB(50, 50, 50);
    nvgFillColor(nvg, nvgRGB(43, 43, 43));
    nvgFillRect(nvg, x, y, width, height);

    // Draw the object list
    float textX = x + 30; // Padding from the left edge
    float textY = y + 40; // Starting Y position with padding from the top
    const float lineHeight = 30; // Line spacing

    nvgFontSize(nvg, 14.0f);
    nvgFontFace(nvg, "sans");
    nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color

    for (const auto& [objectName, isSelected]: objectList)
    {
        if (isSelected)
            nvgDrawRoundedRect(nvg, textX - 10, textY - 18, width - 40, 26, selectedCol, selectedCol, 6.0f);

        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color
        nvgText(nvg, textX, textY, objectName.c_str(), nullptr);
        textY += lineHeight; // Move to the next line
    }

    // Vertical edge line
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, width - 0.5f, y);
    nvgLineTo(nvg, width - 0.5f, y + height);
    nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
    nvgStrokeWidth(nvg, 1.0f);
    nvgStroke(nvg);
}

void LeftPanel::mouseMove(const pptk::Point& position)
{
    if (position.x > getWidth() - 10 && position.x < getWidth())
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_W_RESIZE));
    }
    else
    {
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    }
}

void LeftPanel::mouseLeave(SDL_Event& e)
{
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
}

void LeftPanel::mouseButtonDown(SDL_Event& e)
{
    if (e.button.button == SDL_BUTTON_LEFT)
    {
        if (e.button.x > getWidth() - 10 && e.button.x < getWidth())
        {
            isResizingPanel = true;
        }
        else
        {
            isResizingPanel = false;
        }
    }
}

void LeftPanel::mouseDrag(const pptk::Point& position, const pptk::Point& delta)
{
    auto currBounds = getBounds();
    setBounds(currBounds.x, currBounds.y, position.x, currBounds.h);
}
