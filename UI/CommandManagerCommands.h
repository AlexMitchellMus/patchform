#pragma once

#include "SettingsDialog.h"
#include "AboutDialog.h"

class ShowSettingsCommand : public Command {
    Editor* ed;
public:
    ShowSettingsCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        ed->openDialogWindow(std::make_unique<SettingsDialog>());
    }
};

class ShowAboutCommand : public Command {
    Editor* ed;
public:
    ShowAboutCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        ed->openDialogWindow(std::make_unique<AboutDialog>());
    }
};

class ClosePatchCommand : public Command {
    Editor* ed;
public:
    ClosePatchCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        if (!ed->graphSystem->unloadActivePatch())
            return;

        // We have to wait until the old graph has been swapped out to the active graph
        // We can't just get the active graph right away
        // FIXME: This should fixable. What we need to do is ask if the graph is swapping, and take either transitioning or active!
        // But currently, waiting worst case 4ms is fine for now (on UI thread)
        // As a unload/load is happening, the UI will be changing radically anyway!
        int msDelay = 0;
        for (msDelay; msDelay < 50; ++msDelay)
        {
            if (!ed->graphSystem->graphSwapPending())
                break;
            SDL_Delay(1);
        }

        if (msDelay)
        {
            std::cout << "waited " << msDelay << "ms for graph swap to complete" << std::endl;
        }

        const auto names = ed->graphSystem->getLoadedPatches();
        ed->updateTabs(names);

        if (auto graphManager = ed->graphSystem->getActiveGraph())
        {
            auto newPath = graphManager->getPatchFile();
            auto cnv = ed->getCanvas();
            auto [ graphObjects, connEdges ] = ed->graphSystem->getGraphDump(newPath);
            cnv->setPatchName(FilesystemHelpers::getStem(newPath));
            cnv->reloadAllCanvasObjects(graphObjects);
            cnv->reloadConnections(connEdges);
            cnv->gainFocus();
        }
    }
};

class SaveAsCommand : public Command
{
    Editor* ed;
public:
    SaveAsCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        if (auto graphManager = ed->graphSystem->getActiveRootGraph())
        {
            std::string fullPath;
            std::string existingPath = graphManager->getPatchFile();

            if (!existingPath.empty() && existingPath.rfind("virtual://", 0) != 0)
                fullPath = existingPath;

            std::string newPath = PlatformHelpers::SaveFileChooserDialog(ed->getWindowPeer(), fullPath);
            if (newPath.empty())
            {
                return;
            }

            std::ofstream outputFile(newPath, std::ios::out | std::ios::trunc);
            if (!outputFile.is_open())
            {
                std::cerr << "Failed to write to " << newPath << "\n";
                return;
            }

            outputFile << graphManager->graphToJSON().dump(4);
            outputFile.close();


            // First change the filepath of the current patch
            graphManager->setFilePath(newPath);

            // Update the names of the left tab bar, this will repaint it
            const auto names = ed->graphSystem->getLoadedPatches();
            ed->updateTabs(names);

            // Set the patch name as the active one
            // This will select the tab with the new name
            ed->getCanvas()->setPatchName(FilesystemHelpers::getStem(newPath));
        }
    }
};

class SaveCommand : public Command
{
    Editor* ed;
public:
    SaveCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        auto graphMananger = ed->graphSystem->getActiveRootGraph();

        auto filePath = graphMananger->getPatchFile();
        if (filePath.empty())
            return;

        auto jsonData = graphMananger->graphToJSON();

        std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);

        if (!outputFile.is_open()) {
            std::cerr << "Unable to write to: " << std::filesystem::absolute(filePath).string() << std::endl;
        }

        // Write the JSON to the file with pretty formatting
        outputFile << jsonData.dump(4);
        outputFile.close();

        std::cout << "Graph successfully saved to " << std::filesystem::absolute(filePath).string() << std::endl;
    }
};

class OpenCommand : public Command
{
    Editor* ed;
public:
    OpenCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        auto fileToOpen = PlatformHelpers::OpenFileChooserDialog(ed->getWindowPeer());
        ed->loadFile(fileToOpen);
    }
};

class NewPatchCommand : public Command
{
    Editor* ed;
public:
    NewPatchCommand(Editor* editor) : ed(editor) {}
    void invoke() override
    {
        ed->newEmptyFile();
    }
};