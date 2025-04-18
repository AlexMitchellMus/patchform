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

class MainVolumeMeter : public pptk::Component
{
public:
    MainVolumeMeter() = default;

    void setValue(float left, float right)
    {
        left = std::clamp(left, 1e-6f, 1.0f);
        right = std::clamp(right, 1e-6f, 1.0f);

        leftMeterPeakVal = 20.0f * std::log10(left);
        rightMeterPeakVal = 20.0f * std::log10(right);

        constexpr float dbRange = 40.0f;
        float meterLeft = std::clamp((leftMeterPeakVal + dbRange) / dbRange, 0.0f, 1.0f);
        float meterRight = std::clamp((rightMeterPeakVal + dbRange) / dbRange, 0.0f, 1.0f);

        float meterWidth = width - (height * 2) * getAccumulatedScale();
        int newPos = static_cast<int>((meterLeft + meterRight) * 0.5f * meterWidth * 2);

        if (peakMeterPos != newPos)
        {
            peakMeterPos = newPos;
            repaint();
        }

        meterLeftNorm = meterLeft;
        meterRightNorm = meterRight;
    }

    float getLeftVal() { return meterLeftNorm; }
    float getRightVal() { return meterRightNorm; }

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        auto bgColor = nvgRGBA(33, 33, 33, 255);
        float halfHeight = getHeight() * 0.5f;

        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bgColor, bgColor, halfHeight);

        float meterWidth = width - (halfHeight * 2);
        float meterX = halfHeight;
        float meterY = height * 0.25f;
        float meterH = height * 0.2f;

        // Left channel
        auto meterBgCol = nvgRGBA(40, 40, 40, 255);
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        float leftX = meterWidth * meterLeftNorm;
        auto colL = meterLeftNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
        nvgDrawRoundedRect(nvg, meterX, meterY, leftX, meterH, colL, colL, 0);

        // Right channel
        meterY = height * 0.55f;
        nvgDrawRoundedRect(nvg, meterX, meterY, meterWidth, meterH, meterBgCol, meterBgCol, 0);
        float rightX = meterWidth * meterRightNorm;
        auto colR = meterRightNorm > 0.99f ? nvgRGB(255, 0, 0) : nvgRGBA(28, 73, 119, 180);
        nvgDrawRoundedRect(nvg, meterX, meterY, rightX, meterH, colR, colR, 0);
    }

private:
    float leftMeterPeakVal = 0.0f;
    float rightMeterPeakVal = 0.0f;
    float meterLeftNorm = 0.0f;
    float meterRightNorm = 0.0f;
    int peakMeterPos = 0;
};

class Editor;
class MainMenu : public pptk::PopupComponent
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
                nvgBeginPath(vg);
                nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), outline, outline, 8.0f);
            }

            nvgBeginPath(vg);

            nvgFontSize(vg, 14.0f);
            nvgFontFace(vg, "Regular");
            nvgTextAlign(vg, NVG_ALIGN_LEFT);
            auto col = isActive ? nvgRGB(255, 255, 255) : nvgRGB(100, 100, 100);
            nvgFillColor(vg, col); // Text color
            nvgText(vg, 10, 22, name.c_str(), nullptr);

            if (!keyCommand.empty()) {
                nvgFontSize(vg, 14.0f);
                nvgFontFace(vg, "Regular");
                nvgTextAlign(vg, NVG_ALIGN_RIGHT);
                auto keyColor = isActive ? nvgRGB(100, 100, 100) : nvgRGB(60, 60, 60);
                nvgFillColor(vg, keyColor);
                nvgText(vg, getWidth() - 10, 22, keyCommand.c_str(), nullptr);
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
        if (savePatch)
            savePatch->setBounds(b);
        offset(b.y);
        if (saveAsPatch)
            saveAsPatch->setBounds(b);
        offset(b.y);
        if (closePatch)
            closePatch->setBounds(b);
        offset(b.y);
        if (applicationSettings)
            applicationSettings->setBounds(b);
        offset(b.y);
        if (aboutApp)
            aboutApp->setBounds(b);
        offset(b.y);
        if (quitApplication)
            quitApplication->setBounds(b);
    }

    void render(NVGcontext* vg, const pptk::Theme& theme) override
    {
        nvgBeginPath(vg);
        nvgDrawRoundedRect(vg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(vg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

private:
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

    void setVolumeMeterValue(float left, float right)
    {
        if (volumeMeter)
            volumeMeter->setValue(left, right);
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
        offset += 50;
        textOffset = offset;

        auto volCentreY = (getHeight() / 2) - (32 * 0.5f);
        volumeMeter->setBounds(getWidth() - 50 - 180, volCentreY, 150, 32);

        hideSidePanelsToggle->setBounds(getWidth() - 50, centreY, 35, buttonW);

    }

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
        // Patch name text
        nvgText(nvg, textOffset, height / 2, loadedPatch.c_str(), nullptr);

        // DSP CPU %
        nvgText(nvg, getWidth() - 300, height / 2, dspPercent.c_str(), nullptr);

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

    std::unique_ptr<MainVolumeMeter> volumeMeter;

    std::unique_ptr<ToggleButton> hideSidePanelsToggle;

    std::string loadedPatch;
    int textOffset = 0;

    std::string dspPercent = "";
};
