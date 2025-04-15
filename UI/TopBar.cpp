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

MainMenu::MainMenu(Editor* ed)
{
    setSize(150, 8 * 35 + 5);

    newPatch = std::make_unique<MenuItem>("New patch");
    addComponent(newPatch.get());
    newPatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->newEmptyFile();
            close();
        }
    };

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

    savePatch = std::make_unique<MenuItem>("Save patch");
    addComponent(savePatch.get());

    if (ed->graphSystem->getActiveGraph()->getPatchFile().contains("virtual"))
        savePatch->setActivated(false);

    savePatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            auto graphMananger = ed->graphSystem->getActiveGraph();
            setVisible(false);
            auto filePath = graphMananger->getPatchFile();
            if (filePath.empty())
            {
                close();
                return;
            }

            auto jsonData = graphMananger->graphToJSON();

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

    closePatch = std::make_unique<MenuItem>("Close patch");
    addComponent(closePatch.get());

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

TopBar::TopBar(Editor* ed)
{
    mainMenuButton = std::make_unique<ToggleButton>("A", "A");
    mainMenuButton->setName("MainMenu");
    mainMenuButton->onClick = [this, ed]()
    {
        if (mainMenu && mainMenu->isVisible()
        )
        {
            setPopupComponent(nullptr);
            mainMenuButton->setActive(false);
            return;
        }

        auto popup = std::make_unique<MainMenu>(ed);
        mainMenu = popup.get();
        setPopupComponent(std::move(popup));
        getRootComponent()->addComponent(mainMenu.get());
        mainMenu->registerMouseListener(mainMenuButton.get());
        mainMenu->setPosition(18, 50);
        mainMenuButton->setActive(true);
    };

    addComponent(mainMenuButton.get());

    undo = std::make_unique<ToggleButton>("B", "B");
    undo->setName("Undo");
    addComponent(undo.get());

    redo = std::make_unique<ToggleButton>("C", "C");
    redo->setName("Redo");
    addComponent(redo.get());

    volumeMeter = std::make_unique<MainVolumeMeter>();
    addComponent(volumeMeter.get());

    hideSidePanelsToggle = std::make_unique<ToggleButton>("D", "D");
    hideSidePanelsToggle->setName("HidePanels");
    addComponent(hideSidePanelsToggle.get());

    hideSidePanelsToggle->onToggle = [this](const bool state)
    {
        hideShowPanels(state);
    };

    TopBar::resized();
}