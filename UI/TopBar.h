/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/CompEvent.h"
#include "../UI_ToolKit/PopupComponent.h"
#include "../UI_ToolKit/ToggleButton.h"

#include "SDL3/SDL.h"

using namespace pptk;

class MainVolumeMeter : public Component
{
    public:
    MainVolumeMeter(){};

    void setValue(float value)
    {
        // Ensure the value is within range and avoid log(0)
        value = std::clamp(value, 1e-6f, 1.0f);

        // Convert linear value to dB scale
        float dbValue = 20.0f * std::log10(value);

        constexpr float dbRange = 80.0f;

        // Normalize dB range (-dbRange dB to 0 dB) to [0, 1] range
        meterPeakValue = (dbValue + dbRange) / dbRange; // Maps -dbRange (0) to 0dB (1)
        meterPeakValue = std::clamp(meterPeakValue, 0.0f, 1.0f); // Ensure it stays in bounds

        repaint();
    }

    float getValue()
    {
        return meterPeakValue;
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        auto bgColor = nvgRGBA(33, 33, 33, 255);

        auto halfHeight = getHeight() * 0.5f;
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bgColor, bgColor, halfHeight);

        float meterWidth = width - (halfHeight * 2);
        float meterX = getHeight() * 0.5f;

        auto meterBgCol = nvgRGBA(40, 40, 40, 255);
        nvgDrawRoundedRect(nvg, meterX, height * 0.3f, meterWidth, height * 0.4f, meterBgCol, meterBgCol, 0);

        // Convert peak value to X position
        float peakX = meterWidth * meterPeakValue;

        const auto blue = nvgRGBA(28, 73, 119, 255 * 0.7f);
        const auto peak = nvgRGB(255, 0, 0);
        auto col = meterPeakValue > 0.99 ? peak : blue;
        nvgDrawRoundedRect(nvg, meterX, height * 0.3f, peakX, height * 0.4f, col, col, 0);
    }
private:
    float meterPeakValue = 0.0f;
};

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

        void mouseEnter(pptk::CompEvent& e) override
        {
            isHovered = true;
            repaint();
        }

        void mouseLeave(pptk::CompEvent& e) override
        {
            isHovered = false;
            repaint();
        }

        void mouseButtonDown(pptk::CompEvent& e) override
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

    void setVolumeMeterValue(float value)
    {
        if (volumeMeter)
            volumeMeter->setValue(value);
    }

    float getVolumeMeterValue()
    {
        if (volumeMeter)
            return volumeMeter->getValue();

        return 0.0f;
    }

    void resized() override
    {
        constexpr int buttonW = 35;
        auto centreY = (getHeight() / 2) - (buttonW * 0.5f);
        int offset = 16;
        mainMenuButton->setBounds(offset, centreY, buttonW, buttonW);
        offset += 50;

        undo->setBounds(offset, centreY, buttonW, buttonW);
        offset += 50;
        redo->setBounds(offset, centreY, buttonW, buttonW);

        auto volCentreY = (getHeight() / 2) - (32 * 0.5f);
        volumeMeter->setBounds(getWidth() - 50 - 180, volCentreY, 150, 32);

        hideSidePanelsToggle->setBounds(getWidth() - 50, centreY, 35, buttonW);

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

    void mouseButtonDown(pptk::CompEvent& e) override {
        if (e.sdlEvent.button.button == SDL_BUTTON_LEFT) {
            isHit = true;
        }
    }

    void mouseButtonUp(pptk::CompEvent& e) override {
        if (e.sdlEvent.button.button == SDL_BUTTON_LEFT) {
            isHit = false;
        }
    }

private:
    bool isHit = false;

    std::unique_ptr<ToggleButton> mainMenuButton;
    SafePointer<MainMenu> mainMenu;

    std::unique_ptr<ToggleButton> undo;
    std::unique_ptr<ToggleButton> redo;

    std::unique_ptr<MainVolumeMeter> volumeMeter;

    std::unique_ptr<ToggleButton> hideSidePanelsToggle;

};
