/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/RootComponent.h"
#include "../UI_ToolKit/ToggleButton.h"

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include <atomic>

#include "Canvas.h"
#include "ToolDock.h"
#include "LeftPanel.h"
#include "RightPanel.h"
#include "Object.h"
#include "TopBar.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class GraphManager;
class App : public pptk::RootComponent {
public:
    App(GraphManager* gm);

    void updateObjectsFromDSP();

    void mouseMove(const pptk::Point& position) override
    {
        // FIXME: Mouse move is not registering ATM, we need to add a listener or think about a better solution
        if (position.y > getHeight() - 100)
        {
            unregisterTimerCallback(this);
            resizeToolDock(true);
        }
    }

    void resizeToolDock(bool reset)
    {
        int toolDockWidth = 300;

        toolDockPosY = reset ? (getHeight() - 60) : toolDockPosY + 0.5f;

        float toolDockOffset = (getWidth() / 2.0f) - (toolDockWidth / 2.0f);
        toolDock->setBounds(toolDockOffset, toolDockPosY, toolDockWidth, 45);
    }

    void resized() override
    {
        const auto topBarHeight = 40;
        topBar->setBounds(0, 0, getWidth(), topBarHeight);
        canvas->setBounds(-canvas->canvasOrigin, - canvas->canvasOrigin + topBarHeight, canvas->infinteCanvasSize, canvas->infinteCanvasSize);
        leftPanel->setBounds(0, topBarHeight, 200, getHeight() - topBarHeight);

        resizeToolDock(true);

        rightPanel->setBounds(getWidth() - 200, topBarHeight, 200, getHeight() - topBarHeight);
    }

    GraphManager* graphManager;

private:

    std::unique_ptr<Canvas> canvas;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<ToolDock> toolDock;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<RightPanel> rightPanel;

    float toolDockAnimator = 1.0f;
    bool animateToolDock = false;

    float toolDockPosY;
};