/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Editor.h"
#include "../Graph/AudioGraph.h"

Editor::Editor(){};

void Editor::init(GraphManager* gm)
{
    graphManager = gm;

    canvas = std::make_unique<Canvas>(graphManager);
    canvas->setName("canvas");
    addComponent(canvas.get());

    topBar = std::make_unique<TopBar>();
    topBar->setName("topBar");
    addComponent(topBar.get());

    canvas->onPatchChanged = [this]()
    {
        topBar->setPatchName(canvas->getPatchName());
    };

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

    Editor::resized();
}

void Editor::loadFile(const std::string& fileName) const
{
    if (fileName.empty())
        return;

    std::ifstream file(fileName);

    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    try {
        nlohmann::json patch = nlohmann::json::parse(fileContent, nullptr, true, true);
        if (!patch.empty()) {
            auto filePath = std::filesystem::absolute(fileName).string();
            auto [ graphObjects, connEdges ] = graphManager->setActiveGraph(filePath, patch, false);
            std::filesystem::path filePathObj(fileName);
            canvas->setPatchName(filePathObj.stem().string());
            canvas->reloadAllCanvasObjects(graphObjects);
            canvas->reloadConnections(connEdges);
        }
    }
    catch (const nlohmann::json::parse_error& ex)
    {
        std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
    }
}

void Editor::updateObjectsFromDSP() const
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



