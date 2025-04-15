/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "../UI_ToolKit/Component.h"
#include "../UI_ToolKit/Resizer.h"

class LoadedPatchesPanel;
class ObjectsListViewport;
class Editor;
class Canvas;
class LeftPanel : public pptk::ResizableComponent
{
public:
    explicit LeftPanel(Editor* ed);

    void render(NVGcontext* nvg) override;

    void resized() override;

    void updateTabs(std::vector<std::string> tabs);

    void updateSelectedTab() const;

    void resetScroll();

private:
    Canvas* cnv;
    std::unique_ptr<ObjectsListViewport> objectsList;

    std::unique_ptr<LoadedPatchesPanel> loadedPatchesPanel;
};
