/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "TopBar.h"
#include "Editor.h"
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "../UI_ToolKit/PlatformHelpers.h"

MainMenu::MainMenu()
{
    setSize(150, 6 * 35 + 5);

    loadPatch = std::make_unique<MenuItem>("Open patch...");
    addComponent(loadPatch.get());
    loadPatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            auto fileToOpen = PlatformHelpers::OpenFileChooserDialog(ed->getWindowPeer());
            ed->loadFile(fileToOpen);
            close();
        }
    };

    savePatch = std::make_unique<MenuItem>("Save patch...");
    addComponent(savePatch.get());
    savePatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            auto filePath = ed->graphManager->getPatchFile();
            if (filePath.empty())
            {
                close();
                return;
            }

            auto jsonData = ed->graphManager->graphToJSON();

            std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);

            if (!outputFile.is_open()) {
                std::cerr << "Unable to write to: " << std::filesystem::absolute(filePath).string() << std::endl;
            }

            // Write the JSON to the file with pretty formatting
            outputFile << jsonData.dump(4);
            outputFile.close();

            std::cout << "Graph successfully saved to " << std::filesystem::absolute(filePath).string() << std::endl;
            close();
        }
    };

    saveAsPatch = std::make_unique<MenuItem>("Save patch as...");
    addComponent(saveAsPatch.get());

    applicationSettings = std::make_unique<MenuItem>("Settings...");
    addComponent(applicationSettings.get());
    applicationSettings->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->openDialogWindow(std::make_unique<SettingsDialog>());
        }
    };

    aboutApp = std::make_unique<MenuItem>("About...");
    addComponent(aboutApp.get());
    aboutApp->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->openDialogWindow(std::make_unique<AboutDialog>());
        }
    };

    quitApplication = std::make_unique<MenuItem>("Exit");
    addComponent(quitApplication.get());
    quitApplication->onClick = [this]()
    {
        SDL_Event event;
        event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&event);
    };

    MainMenu::resized();
};