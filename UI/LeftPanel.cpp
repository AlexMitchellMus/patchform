/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "LeftPanel.h"

#include <Graph/GraphSystem.h>

#include "LoadedPatchesPanel.h"

#include <UI_Toolkit/ComponentViewport.h>

#include "SDL3/SDL.h"
#include "Object.h"
#include "Canvas.h"
#include "Editor.h"

class ObjectItem : public pptk::Component
{
    public:

    std::function<void()> onClick;

    ObjectItem(Object* canvasObj) : obj(canvasObj)
    {
        name = obj->getName();
        isSelected = obj->getIsSelected();

        setName("object item: " + name);
    }

    void update()
    {
        isSelected = obj->getIsSelected();
        resetHovered();
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

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        if (isSelected || isHovered)
        {
            NVGcolor bg = isSelected
                ? nvgRGB(43, 43, 43)
                : nvgRGBA(43, 43, 43, static_cast<unsigned char>(255 * 0.4f));

            nvgDrawRoundedRect(nvg, 8, 4, width - 16, height - 8, bg, bg, 6.0f);
        }

        nvgFillColor(nvg, nvgRGB(220, 220, 220));
        nvgFontFace(nvg, "Regular");
        nvgFontSize(nvg, 14.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(nvg, 24, ((height - 10) * 0.5f) + 5, name.c_str(), nullptr);
    }

    Object* getObject()
    {
        return obj;
    }

    void resetHovered()
    {
        isHovered = false;
        repaint();
    }
private:
    Object* obj;
    std::string name;
    bool isSelected = false;
    bool isHovered = false;
};

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

class ObjectsListViewport : public pptk::ComponentViewport
{
    public:
    ObjectsListViewport(Canvas* cnv)
    {
        auto viewedComp = std::make_unique<ObjectsList>(cnv);
        viewedComp->setName("List of objects view");
        viewedComp->onBoundsUpdate = [this](int newHeight)
        {
            setContentHeight(newHeight);
        };
        viewedComp->onKeyPressed = [this]()
        {
            scrollToSelectedItem();
        };
        viewedComp->updateBounds();
        setViewport(std::move(viewedComp));
    }

    void scrollToSelectedItem()
    {
        // Get the list component.
        auto list = getViewedComponent<ObjectsList>();
        int selectedIndex = list->getSelectedIndex();
        if (selectedIndex == -1)
            return;

        const int itemHeight = 32;
        int itemTop = selectedIndex * itemHeight;
        int itemBottom = itemTop + itemHeight;

        // Assuming getScrollY() returns current vertical scroll offset
        // and getHeight() returns the viewport height.
        int currentScroll = viewportY;
        int viewHeight = getHeight();

        // If the item is above the visible area, scroll up.
        if (itemTop < currentScroll) {
            scrollToPosition(pptk::Point(0, itemTop));
        }
        // If the item is below the visible area, scroll down.
        else if (itemBottom > currentScroll + viewHeight) {
            scrollToPosition(pptk::Point(0, itemBottom - viewHeight));
        }
    }

    void renderViewportBackground(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(53, 53, 53));
        nvgFillRect(nvg, 0, 0, width, 1);
    }

    void onScroll() override
    {
        reinterpret_cast<ObjectsList*>(getViewedComponent())->resetHover();
    }

    void resized() override
    {
        if (auto viewedComp = getViewedComponent<ObjectsList>())
        {
            viewedComp->updateBounds();
        }

        ComponentViewport::resized();
    }
};

void LeftPanel::updateSelectedTab() const
{
    loadedPatchesPanel->setSelected(cnv->getPatchName());
}

LeftPanel::LeftPanel(Editor* ed) : cnv(ed->getCanvas())
{
    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Right);

    loadedPatchesPanel = std::make_unique<LoadedPatchesPanel>(ed);
    addComponent(loadedPatchesPanel.get());

    if (cnv)
    {
        cnv->addObjectChangedListener([this]()
        {
            if (objectsList)
                objectsList->getViewedComponent<ObjectsList>()->updateCanvasObjectList();
        });
    }

    objectsList = std::make_unique<ObjectsListViewport>(cnv);
    objectsList->setName("object list viewport");
    addComponent(objectsList.get());

    objectsList->getViewedComponent<ObjectsList>()->updateCanvasObjectList();

    LeftPanel::resized();
}

void LeftPanel::resized()
{
    getResizer().setBounds(getBounds());

    auto loadedPatchesBounds = getLocalBounds().withHeight(300);
    loadedPatchesPanel->setBounds(loadedPatchesBounds);

    auto viewportBounds = getLocalBounds().removeFromTop(300);
    objectsList->setBounds(viewportBounds);
}

void LeftPanel::resetScroll()
{
    objectsList->resetViewport();
}

void LeftPanel::updateTabs(std::vector<std::string> tabs)
{
    loadedPatchesPanel->updateTabs(tabs);
}

void LeftPanel::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    nvgFillColor(nvg, theme.app.panel_background);
    nvgFillRect(nvg, 0, 0, width, height);

    // Vertical edge line
    nvgBeginPath(nvg);
    nvgMoveTo(nvg, width - 0.5f, 0);
    nvgLineTo(nvg, width - 0.5f, height);
    nvgStrokeColor(nvg, theme.app.general_border);
    nvgStrokeWidth(nvg, 1.0f);
    nvgStroke(nvg);
}
