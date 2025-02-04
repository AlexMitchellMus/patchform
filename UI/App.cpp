#include "App.h"
#include "../Graph/AudioGraph.h"

App::App(GraphManager* gm) : graphManager(gm) {
    canvas = std::make_unique<Canvas>(graphManager);
    canvas->setName("canvas");
    addComponent(canvas.get());

    topBar = std::make_unique<TopBar>();
    topBar->setName("topBar");
    addComponent(topBar.get());

    toolDock = std::make_unique<ToolDock>(canvas.get());
    toolDock->setName("toolDock");
    addComponent(toolDock.get());

    leftPanel = std::make_unique<LeftPanel>(canvas.get());
    leftPanel->setName("leftPanel");
    addComponent(leftPanel.get());

    rightPanel = std::make_unique<RightPanel>(canvas.get());
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

    float val;
    float sumPeaks = 0.0f;
    int count = 0;

    //  Read and process all available peak values
    while (graphManager->volumeMeterQueue.try_dequeue(val))
    {
        sumPeaks += val;
        count++;
    }

    if (count > 0)
    {
        float averagedPeak = sumPeaks / count;  // ✅ Process the average
        float lastValue = topBar->getVolumeMeterValue();

        //  Only update if there’s a significant change
        constexpr float PEAK_THRESHOLD = 0.0001f;
        if (std::abs(averagedPeak - lastValue) > PEAK_THRESHOLD)
        {
            topBar->setVolumeMeterValue(averagedPeak);
        }
    }
}



