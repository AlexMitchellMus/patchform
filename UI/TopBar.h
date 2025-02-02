/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/PopupComponent.h"
#include "../UI_ToolKit/ToggleButton.h"

using namespace pptk;

class MainMenu : public PopupComponent
{
public:
    class MenuItem : public Component
    {
    public:
        std::function<void()> onClick = [](){};

        MenuItem(const std::string& itemName) : name(itemName)
        {
        };

        void mouseEnter(SDL_Event& e) override
        {
            isHovered = true;
            repaint();
        }

        void mouseLeave(SDL_Event& e) override
        {
            isHovered = false;
            repaint();
        }

        void mouseButtonDown(SDL_Event& e) override
        {
            onClick();
        }

        void render(NVGcontext* vg) override
        {
            if (isHovered)
            {
                nvgBeginPath(vg);
                nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), outline, outline, 8.0f);
            }

            nvgBeginPath(vg);

            nvgFontSize(vg, 16.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT);
            nvgFillColor(vg, nvgRGB(220, 220, 220)); // Text color
            nvgText(vg, 10, 22, name.c_str(), nullptr);
        }
    private:
        std::string name;
        NVGcolor outline = nvgRGB(53, 53, 53);

        bool isHovered = false;
    };

    MainMenu()
    {
        setSize(150, 6 * 35 + 5);

        loadPatch = std::make_unique<MenuItem>("Open patch...");
        addComponent(loadPatch.get());

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

    void resized() override
    {
        auto b = getBounds();
        b.h = 30;
        b.x = 5;
        b.y = 5;
        b.w = getWidth() - 10;
        if (loadPatch)
            loadPatch->setBounds(b);
        b.y += 35;
        if (savePatch)
            savePatch->setBounds(b);
        b.y += 35;
        if (saveAsPatch)
            saveAsPatch->setBounds(b);
        b.y += 35;
        if (applicationSettings)
            applicationSettings->setBounds(b);
        b.y += 35;
        if (aboutApplication)
            aboutApplication->setBounds(b);
        b.y += 35;
        if (quitApplication)
            quitApplication->setBounds(b);
    }

    void render(NVGcontext* vg) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

private:
    std::unique_ptr<MenuItem> loadPatch;
    std::unique_ptr<MenuItem> savePatch;
    std::unique_ptr<MenuItem> saveAsPatch;
    std::unique_ptr<MenuItem> applicationSettings;
    std::unique_ptr<MenuItem> aboutApplication;
    std::unique_ptr<MenuItem> quitApplication;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);
};

class TopBar : public Component {
public:
    std::function<void(bool)> hideShowPanels = [](bool){};

    TopBar()
    {
        mainMenuButton = std::make_unique<ToggleButton>("A", "A");
        mainMenuButton->setName("MainMenu");
        mainMenuButton->onClick = [this]()
        {
            if (mainMenu && mainMenu->isVisible()
            )
            {
                setPopupComponent(nullptr);
                mainMenuButton->setActive(false);
                return;
            }

            auto popup = std::make_unique<MainMenu>();
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

        hideSidePanelsToggle = std::make_unique<ToggleButton>("D", "D");
        hideSidePanelsToggle->setName("HidePanels");
        addComponent(hideSidePanelsToggle.get());

        hideSidePanelsToggle->onToggle = [this](const bool state)
        {
            hideShowPanels(state);
        };

        TopBar::resized();
    }

    void resized() override
    {
        auto centreY = (getHeight() / 2) - (35 / 2);
        int offset = 16;
        mainMenuButton->setBounds(offset, centreY, 35, 35);
        offset += 50;

        undo->setBounds(offset, centreY, 35, 35);
        offset += 50;
        redo->setBounds(offset, centreY, 35, 35);

        hideSidePanelsToggle->setBounds(getWidth() - 50, centreY, 35, 35);

    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(43, 43, 43));
        nvgFillRect(nvg, 0, 0, width, height);

        // Horizontal line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0, height - 0.5f);
        nvgLineTo(nvg, 0 + width, height - 0.5f);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);
    }

    bool hitTest(float x, float y) const override {
        return getBounds().contains(x, y);
    }

    void mouseButtonDown(SDL_Event& e) override {
        if (e.button.button == SDL_BUTTON_LEFT) {
            isHit = true;
        }
    }

    void mouseButtonUp(SDL_Event& e) override {
        if (e.button.button == SDL_BUTTON_LEFT) {
            isHit = false;
        }
    }

private:
    bool isHit = false;

    std::unique_ptr<ToggleButton> mainMenuButton;
    SafePointer<MainMenu> mainMenu;

    std::unique_ptr<ToggleButton> undo;
    std::unique_ptr<ToggleButton> redo;

    std::unique_ptr<ToggleButton> hideSidePanelsToggle;

};
