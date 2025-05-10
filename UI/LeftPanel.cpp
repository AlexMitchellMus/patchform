/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "LeftPanel.h"

#include <Graph/GraphSystem.h>

#include <UI_Toolkit/ComponentViewport.h>

#include "ObjectsListViewport.h"
#include "ObjectsList.h"

#include "SDL3/SDL.h"
#include "Object.h"
#include "Canvas.h"
#include "Editor.h"
#include "LoadedPatchesPanel.h"

LeftPanel::~LeftPanel() {}


void LeftPanel::updateSelectedTab() const
{
    loadedPatchesPanel->setPatchSelected(cnv->getPatchName());
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

void LeftPanel::updateTabs(std::vector<std::tuple<std::string, bool>> tabs)
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
