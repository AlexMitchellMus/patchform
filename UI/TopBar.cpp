/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "TopBar.h"
#include "Editor.h"
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
            auto fileToOpen = PlatformHelpers::OpenFileChooserDialog();
            ed->loadFile(fileToOpen);
            close();
        }
    };

    savePatch = std::make_unique<MenuItem>("Save patch...");
    addComponent(savePatch.get());

    saveAsPatch = std::make_unique<MenuItem>("Save patch as...");
    addComponent(saveAsPatch.get());

    applicationSettings = std::make_unique<MenuItem>("Settings...");
    addComponent(applicationSettings.get());

    aboutApplication = std::make_unique<MenuItem>("About...");
    addComponent(aboutApplication.get());
    aboutApplication->onClick = [this]()
    {

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