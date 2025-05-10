// ObjectsListViewport.cpp
#include "ObjectsListViewport.h"
#include "Canvas.h"
#include "LeftPanel.h"
#include "ObjectsList.h"

ObjectsListViewport::ObjectsListViewport(Canvas* cnv)
{
    auto viewedComp = std::make_unique<ObjectsList>(cnv);
    viewedComp->setName("List of objects view");
    viewedComp->onBoundsUpdate = [this](int newHeight)
    {
        setContentHeight(newHeight);
    };
    viewedComp->onKeyPressed = [this]() {
        scrollToSelectedItem();
    };
    viewedComp->updateBounds();
    setViewport(std::move(viewedComp));
}

void ObjectsListViewport::scrollToSelectedItem()
{
    auto list = getViewedComponent<ObjectsList>();
    int selectedIndex = list->getSelectedIndex();
    if (selectedIndex == -1)
        return;

    const int itemHeight = 32;
    int itemTop = selectedIndex * itemHeight;
    int itemBottom = itemTop + itemHeight;

    int currentScroll = viewportY;
    int viewHeight = getHeight();

    if (itemTop < currentScroll)
    {
        scrollToPosition(pptk::Point(0, itemTop));
    }
    else if (itemBottom > currentScroll + viewHeight)
    {
        scrollToPosition(pptk::Point(0, itemBottom - viewHeight));
    }
}

void ObjectsListViewport::renderViewportBackground(NVGcontext* nvg, const pptk::Theme& theme)
{
    nvgBeginPath(nvg);
    nvgFillColor(nvg, nvgRGB(53, 53, 53));
    nvgFillRect(nvg, 0, 0, width, 1);
}

void ObjectsListViewport::onScroll()
{
    reinterpret_cast<ObjectsList*>(getViewedComponent())->resetHover();
}

void ObjectsListViewport::resized()
{
    if (auto viewedComp = getViewedComponent<ObjectsList>())
    {
        viewedComp->updateBounds();
    }

    ComponentViewport::resized();
}
