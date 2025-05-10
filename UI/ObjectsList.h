#pragma once
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

#include "Canvas.h"
#include "Object.h"
#include "UI_ToolKit/CompEvent.h"
#include "UI_ToolKit/Component.h"
#include "UI_ToolKit/ComponentViewport.h"
#include "ObjectItem.h"

class ObjectsList : public pptk::Component
{
public:
    std::function<void(int)> onBoundsUpdate = [](int){};
    std::function<void()> onKeyPressed = [](){};

    ObjectsList(Canvas* canvas) : cnv(canvas) {}

    void resetHover()
    {
        for (auto& obj : objectListItems)
        {
            obj->resetHovered();
        }
    }

    void updateCanvasObjectList()
    {
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
                auto newItem = std::make_unique<ObjectItem>(obj);
                newItem->onClick = [this, obj]()
                {
                    cnv->setSelected(obj);
                    gainFocus();
                };
                addComponent(newItem.get());
                objectListItems.push_back(std::move(newItem));
            }
        }

        updateBounds();
        resized();
        repaint();
    }

    void keyPressed(pptk::CompEvent& e)
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
        onKeyPressed();
    }

    int getSelectedIndex() const {
        for (size_t i = 0; i < objectListItems.size(); ++i) {
            if (objectListItems[i]->getObject()->getIsSelected())
                return static_cast<int>(i);
        }
        return -1;
    }

    void updateBounds()
    {
        if (auto parent = getParent())
        {
            calculatedHeight = objectListItems.size() * 32;

            setBounds(0, 0, parent->getWidth(), calculatedHeight);
            // This will reset the viewport to the top when the size changes
            // TODO: make the viewport respect the position of the current scroll
            if (calculatedHeight != previousHeight)
            {
                reinterpret_cast<pptk::ComponentViewport*>(getParent())->resetViewport();
            }
            previousHeight = calculatedHeight;
        }
    }

    void resized() override
    {
        // TODO: Make offset 16 (so the top item is not directly below top of component)
        // to do this we need to also workout how to make the viewport scroll up/down with this extra padding
        int offsetY = 0;
        for (const auto& item : objectListItems)
        {
            item->setBounds(0, offsetY, getWidth(), 32);
            offsetY += 32;
        }
        onBoundsUpdate(calculatedHeight);
    }

private:
    Canvas* cnv;
    std::vector<std::unique_ptr<ObjectItem>> objectListItems;
    int calculatedHeight = 0;
    int previousHeight = 0;
};
