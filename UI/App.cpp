#include "App.h"
#include "../Graph/AudioGraph.h"

App::App(GraphManager* gm) : graphManager(gm) {
    canvas = std::make_unique<Canvas>(graphManager);
    canvas->setName("canvas");
    addComponent(canvas.get());

    topBar = std::make_unique<TopBar>();
    topBar->setName("topBar");
    addComponent(topBar.get());

    gm->repaintMeter = [this]()
    {
        meterRepaintFlag.store(true, std::memory_order_relaxed);
    };

    toolDock = std::make_unique<ToolDock>(canvas.get());
    toolDock->setName("toolDock");
    addComponent(toolDock.get());

    leftPanel = std::make_unique<LeftPanel>(canvas.get());
    leftPanel->setName("leftPanel");
    addComponent(leftPanel.get());

    rightPanel = std::make_unique<RightPanel>();
    rightPanel->setName("rightPanel");
    addComponent(rightPanel.get());

    topBar->hideShowPanels = [this](bool state)
    {
        leftPanel->setVisible(!state);
        rightPanel->setVisible(!state);

        //#define AUTO_HIDE_DOCK
#ifdef AUTO_HIDE_DOCK
        if (state)
        {
            resizeToolDock(true);
            registerTimer([this]()
            {
                resizeToolDock(false);
            });
        } else
        {
            unregisterTimerCallback(this);
            resizeToolDock(true);
        }
#endif
    };

    App::resized();
}

void App::updateObjectsFromDSP()
{
    canvas->updateGraphValuesIfNeeded();

    if (meterRepaintFlag.load(std::memory_order_relaxed))
    {
        float val;
        while (graphManager->volumeMeterQueue.try_dequeue(val)){};
        topBar->setVolumeMeterValue(val);
        meterRepaintFlag.store(false, std::memory_order_relaxed);
    }
}