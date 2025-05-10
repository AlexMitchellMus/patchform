#pragma once

#include <NanoVGWrapper.h>
#include "../UI_ToolKit/ComponentViewport.h"

class Canvas;
class ObjectsListViewport : public pptk::ComponentViewport
{
    public:
    ObjectsListViewport(Canvas* cnv);

    void scrollToSelectedItem();

    void renderViewportBackground(NVGcontext* nvg, const pptk::Theme& theme) override;

    void onScroll() override;

    void resized() override;
};