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

// Class to darken the background of the central dialog window
// This isn't really modal in the sense that it pauses the main thread,
// but we use this component to both darken the whole editor
// and catch any mouse clicks outside the dialog component
class ModalBackground : public pptk::Component
{
public:
    std::function<void()> onClick = [](){};

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        std::cout << "clicking on modal background" << std::endl;
        onClick();
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, 0, 0, width, height, darkenBg, darkenBg, 0);
    }

    NVGcolor darkenBg = nvgRGBA(0, 0, 0, 80);
};

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
        canvas->setBounds(-canvas->canvasOrigin * canvas->scale + canvas->canvasOffset.x, - canvas->canvasOrigin * canvas->scale + canvas->canvasOffset.y, canvas->infinteCanvasSize, canvas->infinteCanvasSize);
        leftPanel->setBounds(0, topBarHeight, 200, getHeight() - topBarHeight);

        resizeToolDock(true);

        rightPanel->setBounds(getWidth() - 200, topBarHeight, 200, getHeight() - topBarHeight);

        if (dialogWindow)
        {
            dialogWindow->setBounds(getWidth() * 0.5f - 400, getHeight() * 0.5f - 300, 800, 600);
        }
    }

    void updateFrameBuffers(NVGcontext* nvg)
    {
        canvas->updateFrameBuffer(nvg);
    }

    WindowPeer* getWindowPeer() const
    {
        return windowPeer;
    }

    void openDialogWindow(std::unique_ptr<Component> comp)
    {
        dialogWindow = std::move(comp);
        dialogWindowModalBackground = std::make_unique<ModalBackground>();
        dialogWindowModalBackground->setBounds(0, 0, getWidth(), getHeight());
        dialogWindowModalBackground->onClick = [this]()
        {
            if (dialogWindow)
            {
                dialogWindow.reset();
                dialogWindowModalBackground->setVisible(false);
                // FIXME: repaints that need to occur due to component changes should be handled by the toolkit!
                repaint();
            }
        };
        addComponent(dialogWindowModalBackground.get());
        addComponent(dialogWindow.get());
        resized();
    }

    void closeDialogWindow()
    {
        dialogWindow.reset();
        repaint();
    }

    GraphManager* graphManager;

private:
    std::unique_ptr<ModalBackground> dialogWindowModalBackground;
    std::unique_ptr<Component> dialogWindow;

    // TODO: Move to Toolkit
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