/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Editor.h"
#include "../Graph/AudioGraph.h"
#include "../UI_ToolKit/WindowPeer.h"

Editor::Editor(WindowPeer* peer) : windowPeer(peer) {};

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

            if (!graphManager->wasPatchLoadSuccessful())
            {
                std::cerr << "Failed to load graph: " << filePath << std::endl;
                return;
            }
            std::filesystem::path filePathObj(fileName);
            canvas->setPatchName(filePathObj.stem().string());
            canvas->reloadAllCanvasObjects(graphObjects);
            canvas->reloadConnections(connEdges);

            // Loading a patch makes the canvas gain focus
            canvas->gainFocus();
            leftPanel->resetScroll();
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

    std::vector<float> peaks;
    float sumL = 0.0f, sumR = 0.0f;
    int count = 0;

    while (graphManager->volumeMeterQueue.try_dequeue(peaks))
    {
        if (peaks.size() == 2)
        {
            sumL += peaks[0];
            sumR += peaks[1];
            count++;
        }
    }

    if (count > 0)
    {
        float avgL = sumL / count;
        float avgR = sumR / count;

        float lastL = topBar->getVolumeMeterLeft();
        float lastR = topBar->getVolumeMeterRight();

        constexpr float PEAK_THRESHOLD = 0.0001f;

        if (std::abs(avgL - lastL) > PEAK_THRESHOLD)
            lastL = avgL;

        if (std::abs(avgR - lastR) > PEAK_THRESHOLD)
            lastR = avgR;

        topBar->setVolumeMeterValue(lastL, lastR);
    }

    topBar->setDSPValue(graphManager->getDspTiming());
}
