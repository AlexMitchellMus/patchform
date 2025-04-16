/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <filesystem>
#include "Editor.h"

#include "FilesystemHelpers.h"
#include <filesystem>
#include "../Graph/GraphSystem.h"
#include "../UI_ToolKit/WindowPeer.h"

Editor::Editor(WindowPeer* peer) : windowPeer(peer) {};

void Editor::init(GraphSystem* gm)
{
    std::cout << "reinit editor" << std::endl;
    graphSystem = gm;

    canvas = std::make_unique<Canvas>(graphSystem);
    canvas->setName("canvas");
    addComponent(canvas.get());

    topBar = std::make_unique<TopBar>(this);
    topBar->setName("topBar");
    addComponent(topBar.get());

    topBar->setPatchName(canvas->getPatchName());

    toolDock = std::make_unique<ToolDock>(canvas.get());
    toolDock->setName("toolDock");
    addComponent(toolDock.get());

    leftPanel = std::make_unique<LeftPanel>(this);
    leftPanel->setName("leftPanel");
    addComponent(leftPanel.get());

    canvas->onPatchChanged = [this]()
    {
        topBar->setPatchName(canvas->getPatchName());
        leftPanel->updateSelectedTab();
    };

    graphSystem->onPatchLoaded = [this](GraphSystem::Graphs& loadedGraphs)
    {
        std::vector<std::string> tabs;
        for (const auto& graph : loadedGraphs)
        {
            tabs.push_back(graph->getPatchFile());
        }

        leftPanel->updateTabs(tabs);
    };

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

    newEmptyFile();

    Editor::resized();
}

void Editor::newEmptyFile() const
{
    nlohmann::json emptyPatch = {
        { "nodes", nlohmann::json::array() },
        { "connections", nlohmann::json::array() }
    };

    std::string virtualPath = generateUniqueUntitledName();
    auto shortName = virtualPath.substr(virtualPath.find_last_of('/') + 1);

    auto [ graphObjects, connEdges ] = graphSystem->loadPatch(virtualPath, emptyPatch, false);
    graphSystem->setActiveGraph(virtualPath);
    canvas->setPatchName(shortName);
    canvas->reloadAllCanvasObjects(graphObjects);
    canvas->reloadConnections(connEdges);
    canvas->gainFocus();
    leftPanel->resetScroll();
}

std::string Editor::generateUniqueUntitledName() const
{
    int counter = 1;
    while (true)
    {
        std::string name = "Untitled-" + std::to_string(counter);
        std::string virtualPath = "virtual://" + name;

        const auto& loaded = graphSystem->getLoadedPatches();
        bool exists = std::any_of(loaded.begin(), loaded.end(),
            [&](const std::string& path) { return path == virtualPath; });

        if (!exists)
            return virtualPath;

        ++counter;
    }
}

void Editor::loadFile(const std::string& fileName) const
{
    if (fileName.empty())
        return;

    auto normalizePath = [](const std::string& in) -> std::string {
        char out[MAX_PATH];
        return _fullpath(out, in.c_str(), MAX_PATH) ? std::string(out) : in;
    };

    std::string absPath = normalizePath(fileName);

    // If the patch is already loaded, load it into the canvas, and make it active
    for (const std::string& path : graphSystem->getLoadedPatches()) {
        if (path == absPath || FilesystemHelpers::getStem(path) == FilesystemHelpers::getStem(absPath)) {
            if (canvas->getPatchName() == FilesystemHelpers::getStem(path))
                return;

            auto [graphObjects, connEdges] = graphSystem->getGraphDump(path);
            graphSystem->setActiveGraph(path);
            canvas->setPatchName(FilesystemHelpers::getStem(path));
            canvas->reloadAllCanvasObjects(graphObjects);
            canvas->reloadConnections(connEdges);
            canvas->gainFocus();
            leftPanel->resetScroll();
            return;
        }
    }

    std::ifstream file(fileName);

    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    try {
        nlohmann::json patch = nlohmann::json::parse(fileContent, nullptr, true, true);
        if (!patch.empty()) {
            auto filePath = std::filesystem::absolute(fileName).string();
            auto [ graphObjects, connEdges ] = graphSystem->loadPatch(filePath, patch, false);

            std::filesystem::path filePathObj(fileName);
            graphSystem->setActiveGraph(filePath);
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

    while (graphSystem->volumeMeterQueue.try_dequeue(peaks))
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

    topBar->setDSPValue(graphSystem->getDspTiming());
}
