/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/
#include <filesystem>
#include "Editor.h"

#include "FilesystemHelpers.h"
#include "../Graph/GraphSystem.h"
#include "CommandManagerCommands.h"

Editor::Editor(WindowPeer* peer) : windowPeer(peer) {};

void Editor::init(GraphSystem* gm)
{
    initCommands();

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
        topBar->setBreadcrumbGraph(graphSystem->getActiveGraph());
        leftPanel->updateSelectedTab();
    };

    graphSystem->onPatchLoaded = [this](GraphSystem::Graphs& loadedGraphs)
    {
        // Set up dirty-change callbacks
        for (const auto& graph : loadedGraphs)
        {
            graph->graphModifiedCallback = [this]()
            {
                // Rebuild full tab list when any graph is modified
                auto tabs = graphSystem->getLoadedPatches();
                leftPanel->updateTabs(tabs);
            };
        }

        // Initial full tab update when patches are loaded
        leftPanel->updateTabs(graphSystem->getLoadedPatches());
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

void Editor::initCommands()
{
    commandIDManager.registerCommand("NewPatch", std::make_unique<NewPatchCommand>(this));
    commandIDManager.bindKey(SDLK_N, SDL_KMOD_CTRL, "NewPatch");

    commandIDManager.registerCommand("OpenPatch", std::make_unique<OpenCommand>(this));
    commandIDManager.bindKey(SDLK_O, SDL_KMOD_CTRL, "OpenPatch");

    commandIDManager.registerCommand("SavePatch", std::make_unique<SaveCommand>(this));
    commandIDManager.bindKey(SDLK_S, SDL_KMOD_CTRL, "SavePatch", true);

    commandIDManager.registerCommand("SavePatchAs", std::make_unique<SaveAsCommand>(this));
    commandIDManager.bindKey(SDLK_S, SDL_KMOD_CTRL | SDL_KMOD_SHIFT, "SavePatchAs", true);

    commandIDManager.registerCommand("ClosePatch", std::make_unique<ClosePatchCommand>(this));
    commandIDManager.bindKey(SDLK_W, SDL_KMOD_CTRL, "ClosePatch");

    commandIDManager.registerCommand("ShowSettingsDialog", std::make_unique<ShowSettingsCommand>(this));
    commandIDManager.bindKey(SDLK_COMMA, SDL_KMOD_CTRL, "ShowSettingsDialog");

    commandIDManager.registerCommand("ShowAboutDialog", std::make_unique<ShowAboutCommand>(this));
    commandIDManager.bindKey(SDLK_F1, SDL_KMOD_NONE, "ShowAboutDialog");
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
            [&](const std::tuple<std::string, bool>& tup) {
                return std::get<0>(tup) == virtualPath;
            });

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
    for (const auto& [ path , isDirty ]: graphSystem->getLoadedPatches()) {
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

    std::array<float, 6> values{};
    float peakL = 0.0f, peakR = 0.0f;
    float holdL = 0.0f, holdR = 0.0f;

    bool haveValues = false;

    while (graphSystem->volumeMeterQueue.try_dequeue(values))
    {
        // Only update if the incoming values are > 0
        if (values[1] > 0.0f) peakL = std::max(peakL, values[1]);
        if (values[4] > 0.0f) peakR = std::max(peakR, values[4]);
        if (values[2] > 0.0f) holdL = std::max(holdL, values[2]);
        if (values[5] > 0.0f) holdR = std::max(holdR, values[5]);

        haveValues = true;
    }
    if (haveValues)
        topBar->setVolumeMeterValue(peakL, peakR, holdL, holdR);

    topBar->setDSPValue(graphSystem->getDspTiming());
}
