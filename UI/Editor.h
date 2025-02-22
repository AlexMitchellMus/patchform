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

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

class GraphManager;
class WindowPeer;
class Editor : public pptk::RootComponent {
public:
    Editor(WindowPeer* peer);

    void init(GraphManager* gm);

    void updateObjectsFromDSP() const;

    void mouseMove(const pptk::Point& position) override
    {
        // FIXME: Mouse move is not registering ATM, we need to add a listener or think about a better solution
        if (position.y > getHeight() - 100)
        {
            unregisterTimerCallback(this);
            resizeToolDock(true);
        }
    }

    Canvas* getActiveCanvas() const
    {
        return canvas.get();
    }

    void loadFile(const std::string& file) const;

    void resizeToolDock(bool reset)
    {
        int toolDockWidth = 300;

        toolDockPosY = reset ? (getHeight() - 60) : toolDockPosY + 0.5f;

        float toolDockOffset = (getWidth() / 2.0f) - (toolDockWidth / 2.0f);
        toolDock->setBounds(toolDockOffset, toolDockPosY, toolDockWidth, 45);
    }

    void resized() override
    {
        std::cout << "resizing editor" << std::endl;
        constexpr auto topBarHeight = 40;
        topBar->setBounds(0, 0, getWidth(), topBarHeight);
        canvas->setBounds(-canvas->canvasOrigin, - canvas->canvasOrigin + topBarHeight, canvas->infinteCanvasSize, canvas->infinteCanvasSize);
        leftPanel->setBounds(0, topBarHeight, 200, getHeight() - topBarHeight);

        resizeToolDock(true);

        rightPanel->setBounds(getWidth() - 200, topBarHeight, 200, getHeight() - topBarHeight);
    }

    void updateFrameBuffers(NVGcontext* nvg)
    {
        canvas->updateFrameBuffer(nvg);
    }

    WindowPeer* getWindowPeer() const
    {
        return windowPeer;
    }


    GraphManager* graphManager;

private:
    WindowPeer* windowPeer;

    std::unique_ptr<Canvas> canvas;
    std::unique_ptr<TopBar> topBar;
    std::unique_ptr<ToolDock> toolDock;
    std::unique_ptr<LeftPanel> leftPanel;
    std::unique_ptr<RightPanel> rightPanel;

    float toolDockAnimator = 1.0f;
    bool animateToolDock = false;

    float toolDockPosY;
};