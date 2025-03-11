/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "LeftPanel.h"
#include "SDL3/SDL.h"
#include "Object.h"
#include "Canvas.h"

class ObjectItems : public pptk::Component
{
    public:

    std::function<void()> onClick;

    ObjectItems(Object* canvasObj) : obj(canvasObj)
    {
        name = obj->getName();
        isSelected = obj->getIsSelected();
    }

    void update()
    {
        isSelected = obj->getIsSelected();
    }

    void mouseEnter(pptk::CompEvent& e) override
    {
        isHovered = true;
        repaint();
    }

    void mouseLeave(pptk::CompEvent& e) override
    {
        isHovered = false;
        repaint();
    }

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        onClick();
        repaint();
    }

    void render(NVGcontext* nvg) override
    {
        if (isSelected || isHovered)
        {
            auto selectedCol = nvgRGB(43, 43, 43);
            nvgDrawRoundedRect(nvg, 8, 4, width - 16, height - 8, selectedCol, selectedCol, 6.0f);
        }

        nvgFillColor(nvg, nvgRGB(220, 220, 220));
        nvgFontFace(nvg, "Regular");
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(nvg, 24, ((height - 10) * 0.5f) + 5, name.c_str(), nullptr);
    }

    Object* getObject()
    {
        return obj;
    }
private:
    Object* obj;
    std::string name;
    bool isSelected = false;
    bool isHovered = false;
};

LeftPanel::LeftPanel(Canvas* canvas) : cnv(canvas)
{
    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Right);

    if (cnv)
    {
        cnv->addObjectChangedListener([this]()
        {
            updateCanvasObjectList();
        });
    }

    updateCanvasObjectList();

    repaint();
}

void LeftPanel::keyPressed(pptk::CompEvent& e)
{
    if (!cnv || objectListItems.empty())
        return;

    // Use keysym.sym for key comparison.
    int key = e.sdlEvent.key.key;

    // Find the index of the currently selected object.
    int currentIndex = -1;
    for (size_t i = 0; i < objectListItems.size(); ++i)
    {
        if (objectListItems[i]->getObject()->getIsSelected())
        {
            currentIndex = static_cast<int>(i);
            break;
        }
    }

    // If no object is selected, choose one based on the key pressed.
    if (currentIndex == -1)
    {
        if (key == SDLK_DOWN)
        {
            currentIndex = 0;
            cnv->setSelected(objectListItems[currentIndex]->getObject());
        }
        else if (key == SDLK_UP)
        {
            currentIndex = static_cast<int>(objectListItems.size()) - 1;
            cnv->setSelected(objectListItems[currentIndex]->getObject());
        }
        return;
    }

    // Update selection based on the key pressed.
    if (key == SDLK_UP)
    {
        if (currentIndex > 0)
        {
            cnv->setSelected(objectListItems[currentIndex - 1]->getObject());
        }
    }
    else if (key == SDLK_DOWN)
    {
        if (currentIndex < static_cast<int>(objectListItems.size()) - 1)
        {
            cnv->setSelected(objectListItems[currentIndex + 1]->getObject());
        }
    }
}


void LeftPanel::updateCanvasObjectList()
{
    if (!cnv)
        return;

    // Get current canvas objects.
    auto canvasObjects = cnv->getObjects();
    bool sameList = (objectListItems.size() == canvasObjects.size());

    // Check if the existing list is the same.
    if (sameList)
    {
        for (size_t i = 0; i < canvasObjects.size(); ++i)
        {
            if (objectListItems[i]->getObject() != canvasObjects[i])
            {
                sameList = false;
                break;
            }
        }
    }

    if (sameList)
    {
        // If the list is unchanged, update each item.
        for (auto& item : objectListItems)
        {
            item->update(); // refresh state, selection, etc.
        }
    }
    else
    {
        // Rebuild the list if there are differences.
        objectListItems.clear();

        for (auto obj : canvasObjects)
        {
            auto newItem = std::make_unique<ObjectItems>(obj);
            newItem->onClick = [this, obj]()
            {
                cnv->setSelected(obj);
                gainFocus();
            };
            addComponent(newItem.get());
            objectListItems.push_back(std::move(newItem));
        }
    }

    LeftPanel::resized();
    repaint();
}

void LeftPanel::resized()
{
    getResizer().setBounds(getBounds());

    int offsetY = 50;
    for (auto& item : objectListItems)
    {
        item->setBounds(0, offsetY, getWidth(), 32);
        offsetY += 32;
    }
}

void LeftPanel::render(NVGcontext* nvg)
{
    auto selectedCol = nvgRGB(43, 43, 43);
    nvgFillColor(nvg, nvgRGB(33, 33, 33));
    nvgFillRect(nvg, 0, 0, width, height);

    // Draw the object list
    float textX = 24; // Padding from the left edge
    float textY = 40; // Starting Y position with padding from the top

    nvgFontSize(nvg, 14.0f);
    nvgFontFace(nvg, "SemiBold");
    nvgTextAlign(nvg, NVG_ALIGN_LEFT);
    nvgFillColor(nvg, nvgRGB(220, 220, 220));
    nvgText(nvg, textX, textY, "Objects", nullptr);

    // Vertical edge line
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, width - 0.5f, 0);
    nvgLineTo(nvg, width - 0.5f, height);
    nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
    nvgStrokeWidth(nvg, 1.0f);
    nvgStroke(nvg);
}
