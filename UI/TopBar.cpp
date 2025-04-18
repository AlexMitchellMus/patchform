/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "TopBar.h"
#include "Editor.h"
#include "../Graph/GraphSystem.h"
#include "../UI_ToolKit/PlatformHelpers.h"
#include "FilesystemHelpers.h"

MainMenu::MainMenu(Editor* ed)
{
    setSize(230, 8 * 35 + 5);

    newPatch = std::make_unique<MenuItem>("New patch");
    newPatch->setKeyCommand(PlatformHelpers::formatKey(SDLK_N, SDL_KMOD_CTRL));
    addComponent(newPatch.get());
    newPatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["NewPatch"]->invoke();
            close();
        }
    };

    loadPatch = std::make_unique<MenuItem>("Open patch...");
    loadPatch->setKeyCommand(PlatformHelpers::formatKey(SDLK_O, SDL_KMOD_CTRL));
    addComponent(loadPatch.get());
    loadPatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["OpenPatch"]->invoke();
            close();
        }
    };

    savePatch = std::make_unique<MenuItem>("Save patch");
    savePatch->setKeyCommand(PlatformHelpers::formatKey(SDLK_S, SDL_KMOD_CTRL));
    addComponent(savePatch.get());

    if (ed->graphSystem->getActiveGraph()->getPatchFile().contains("virtual"))
        savePatch->setActivated(false);

    savePatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["SavePatch"]->invoke();
        }
        close();
    };

    saveAsPatch = std::make_unique<MenuItem>("Save patch as...");
    saveAsPatch->setKeyCommand(PlatformHelpers::formatKey(SDLK_S, SDL_KMOD_CTRL | SDL_KMOD_SHIFT));
    addComponent(saveAsPatch.get());

    saveAsPatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["SavePatchAs"]->invoke();
        }
        close();
    };


    closePatch = std::make_unique<MenuItem>("Close patch");
    closePatch->setKeyCommand(PlatformHelpers::formatKey(SDLK_W, SDL_KMOD_CTRL));
    addComponent(closePatch.get());

    closePatch->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            ed->commandIDManager["ClosePatch"]->invoke();
        }
        close();
    };

    applicationSettings = std::make_unique<MenuItem>("Settings...");
    applicationSettings->setKeyCommand(PlatformHelpers::formatKey(SDLK_COMMA, SDL_KMOD_CTRL));
    addComponent(applicationSettings.get());
    applicationSettings->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["ShowSettingsDialog"]->invoke();
        }
    };

    aboutApp = std::make_unique<MenuItem>("About...");
    addComponent(aboutApp.get());
    aboutApp->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            ed->commandIDManager["ShowAboutDialog"]->invoke();
        }
    };

    quitApplication = std::make_unique<MenuItem>("Exit");
    quitApplication->setKeyCommand(PlatformHelpers::formatKey(SDLK_Q, SDL_KMOD_CTRL));
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