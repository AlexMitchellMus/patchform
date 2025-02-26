/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "TopBar.h"
#include "Editor.h"
#include "../Graph/AudioGraph.h"
#include "../UI_ToolKit/PlatformHelpers.h"
#include "../GitInfo.h"

class AboutDialog : public pptk::Component
{
    public:
    AboutDialog(){}

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "SemiBold");
        nvgTextAlign(nvg, NVG_ALIGN_CENTER);
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color

        int yPos = 50;
        nvgText(nvg, getWidth() * 0.5f, yPos, "Patchform", nullptr);
        yPos += 40;

        nvgFontSize(nvg, 14.0f);
        nvgFontFace(nvg, "Regular");
        nvgText(nvg, getWidth() * 0.5f, yPos, "by Alex Mitchell", nullptr);
        yPos += 70;

        std::stringstream versionText;
        versionText << "Version: " << patchform_git_version << "      git hash: " << patchform_git_hash;
        nvgText(nvg, getWidth() * 0.5f, yPos, versionText.str().c_str(), nullptr);
        yPos += 30;


    }

    void resized() override
    {
        setBounds(getRootComponent()->getWidth() * 0.5f - 400, getRootComponent()->getHeight() * 0.5f - 300, 800, 600);
    }

private:
    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);
};

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
                throw std::ios_base::failure("Failed to open the file for writing.");
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

    aboutApp = std::make_unique<MenuItem>("About...");
    addComponent(aboutApp.get());
    aboutApp->onClick = [this]()
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            setVisible(false);
            aboutDialog = std::make_unique<AboutDialog>();
            ed->addComponent(aboutDialog.get());
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