/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "BreadcrumbBar.h"
#include "../UI_ToolKit/CompEvent.h"
#include "../UI_ToolKit/PopupComponent.h"
#include "../UI_ToolKit/ToggleButton.h"

#include "MainVolumeMeter.h"

#include "SDL3/SDL.h"


class Editor;
class MainMenu final : public pptk::PopupComponent
{
public:
    class MenuItem : public Component
    {
    public:
        std::function<void()> onClick = [](){};

        MenuItem(const std::string& itemName) : name(itemName)
        {
        };

        void setKeyCommand(const std::string& key) {
            keyCommand = key;
            repaint();
        }

        void setActivated(bool shouldBeActive)
        {
            isActive = shouldBeActive;
            repaint();
        }

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
            if (isActive)
                onClick();
        }

        void render(NVGcontext* vg, const pptk::Theme& theme) override
        {
            if (isHovered && isActive)
            {
                nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), outline, outline, 8.0f);
            }

            nvgBeginPath(vg);

            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE_ASCENT);
            auto middle = getHeight() * 0.5f;
            auto col = isActive ? nvgRGB(255, 255, 255) : nvgRGB(100, 100, 100);
            nvgFillColor(vg, col); // Text color
            nvgText(vg, 10, middle, name.c_str(), nullptr);

            if (!keyCommand.empty()) {
                nvgFontSize(vg, 14.0f);
                nvgFontFace(vg, "Regular");
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE_ASCENT);
                auto keyColor = isActive ? nvgRGB(100, 100, 100) : nvgRGB(60, 60, 60);
                nvgFillColor(vg, keyColor);
                nvgText(vg, getWidth() - 10, middle, keyCommand.c_str(), nullptr);
            }
        }
    private:
        std::string name;
        std::string keyCommand;
        NVGcolor outline = nvgRGB(53, 53, 53);

        bool isActive = true;
        bool isHovered = false;
    };

    MainMenu(Editor* ed);

    void resized() override
    {
        auto offset = [](float& value)
        {
            value += 35;
        };

        spacers.clear();

        auto b = getBounds();
        b.h = 30;
        b.x = 5;
        b.y = 5;
        b.w = getWidth() - 10;
        if (newPatch)
            newPatch->setBounds(b);
        offset(b.y);
        if (loadPatch)
            loadPatch->setBounds(b);
        offset(b.y);
        if (closePatch)
            closePatch->setBounds(b);
        offset(b.y);
        addSpacer(b.y);
        if (savePatch)
            savePatch->setBounds(b);
        offset(b.y);
        if (saveAsPatch)
            saveAsPatch->setBounds(b);
        offset(b.y);
        addSpacer(b.y);
        if (applicationSettings)
            applicationSettings->setBounds(b);
        offset(b.y);
        if (aboutApp)
            aboutApp->setBounds(b);
        offset(b.y);
        addSpacer(b.y);
        if (quitApplication)
            quitApplication->setBounds(b);
        offset(b.y);

        setSize(230, b.y);
    }

    void render(NVGcontext* vg, const pptk::Theme& theme) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);

        nvgBeginPath(vg);
        nvgStrokeColor(vg, theme.app.general_border);
        nvgLineStyle(vg, NVG_LINE_SOLID);
        nvgStrokeWidth(vg, 1);

        for (auto yPos : spacers)
        {
            nvgMoveTo(vg, 0, yPos);
            nvgLineTo(vg, getWidth() - 0, yPos);
        }
        nvgStroke(vg);
    }

private:
    void addSpacer(float& pos)
    {
        pos += 2.5f;
        spacers.push_back(pos - 2.5f);
        pos += 2.5f;
    }
    std::vector<float> spacers;

    std::unique_ptr<MenuItem> newPatch;
    std::unique_ptr<MenuItem> loadPatch;
    std::unique_ptr<MenuItem> savePatch;
    std::unique_ptr<MenuItem> saveAsPatch;
    std::unique_ptr<MenuItem> closePatch;
    std::unique_ptr<MenuItem> applicationSettings;
    std::unique_ptr<MenuItem> aboutApp;
    std::unique_ptr<MenuItem> quitApplication;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);
};

class Editor;
class BreadcrumbBar;
class TopBar : public pptk::Component {
public:
    std::function<void(bool)> hideShowPanels = [](bool){};

    explicit TopBar(Editor* ed);

    void setDSPValue(float dspVal)
    {
        auto newDspString = std::format("{:.2f}", dspVal) + " %";
        if (dspPercent != newDspString)
        {
            dspPercent = newDspString;
            repaint();
        }
    }

    void setPatchName(const std::string& patchName)
    {
        loadedPatch = patchName;
        repaint();
    }

    void setBreadcrumbGraph(GraphManager* graphManager) const
    {
        breadcrumbBar->setViewedGraph(graphManager);
    }

    void setVolumeMeterValue(float peakL, float peakR, float holdL, float holdR)
    {
        if (volumeMeter)
            volumeMeter->setValue(peakL, peakR, holdL, holdR);
    }

    float getVolumeMeterLeft()
    {
        if (volumeMeter)
            return volumeMeter->getLeftVal();

        return 0.0f;
    }

    float getVolumeMeterRight()
    {
        if (volumeMeter)
            return volumeMeter->getRightVal();

        return 0.0f;
    }

    void resized() override;

    void render(NVGcontext* nvg, const pptk::Theme& theme) override {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, theme.app.topbar_background);
        nvgFillRect(nvg, 0, 0, width, height);

        // Horizontal line
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, 0, height - 0.5f);
        nvgLineTo(nvg, 0 + width, height - 0.5f);
        nvgStrokeColor(nvg, nvgRGB(53, 53, 53));
        nvgStrokeWidth(nvg, 1.0f);
        nvgStroke(nvg);

        nvgSave(nvg);

        nvgFillColor(nvg, theme.app.general_text);
        nvgFontFace(nvg, "Regular");
        nvgFontSize(nvg, 14.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        // DSP CPU %
        nvgText(nvg, getWidth() - 250, height / 2, dspPercent.c_str(), nullptr);

        nvgRestore(nvg);
    }

    bool hitTest(float x, float y) override {
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
    pptk::SafePointer<MainMenu> mainMenu;

    std::unique_ptr<ToggleButton> undo;
    std::unique_ptr<ToggleButton> redo;

    std::unique_ptr<BreadcrumbBar> breadcrumbBar;

    std::unique_ptr<MainVolumeMeter> volumeMeter;

    std::unique_ptr<ToggleButton> hideSidePanelsToggle;

    std::string loadedPatch;
    int textOffset = 0;

    std::string dspPercent = "";
};
