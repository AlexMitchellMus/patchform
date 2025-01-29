/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_ToolKit/Component.h"

#include "../UI_ToolKit/ToggleButton.h"

class ZoomSlider : public pptk::Component
{
public:
    std::function<void(float)> onChange = [](float){};

    std::function<void()> onClick = [](){};

    explicit ZoomSlider() = default;

    void setZoomValue(float value)
    {
        zoomValue = value * 100;
    }

    void mouseEnter(SDL_Event& e) override
    {
        isHovered = true;
    }

    void mouseLeave(SDL_Event& e) override
    {
        isHovered = false;
    }

    void mouseButtonDown(SDL_Event& e) override
    {
        onClick();
    }

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
    {
        onChange(delta.y);
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        if (isHovered)
        {
            auto bgCol = nvgRGBA(0, 0, 0, 30);
            nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, bgCol, 8);
        }

        nvgBeginPath(nvg);

        nvgFontSize(nvg, 18.0f);
        nvgFontFace(nvg, "Regular");
        nvgTextAlign(nvg, NVG_ALIGN_RIGHT);
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color
        nvgText(nvg, 60, 24, std::string(std::to_string(zoomValue) + " %").c_str(), nullptr);
    }

private:
    int zoomValue = 100;
    bool isHovered = false;
};

class ToolDock : public pptk::Component {
public:
    explicit ToolDock(Canvas* canvas) : cnv(canvas)
    {
        editButton = std::make_unique<ToggleButton>("E", "F", "icons");
        addComponent(editButton.get());

        addObjectButton = std::make_unique<ToggleButton>("G", "G", "icons");
        addComponent(addObjectButton.get());

        viewButton = std::make_unique<ToggleButton>("H", "H", "icons");
        addComponent(viewButton.get());

        resizeToFit = std::make_unique<ToggleButton>("I", "I", "icons");
        addComponent(resizeToFit.get());

        zoomSlider = std::make_unique<ZoomSlider>();
        addComponent(zoomSlider.get());

        cnv->onScaleChange = [this](float scale)
        {
            zoomSlider->setZoomValue(scale);
        };

        zoomSlider->onChange = [this](float value)
        {
            cnv->setScale(value);
        };

        zoomSlider->onClick = [this]()
        {
            cnv->resetScale();
        };

        ToolDock::resized();
    };

    void resized() override
    {
        int offset = 15;
        editButton->setBounds(offset, 5, 35, 35);
        offset += 50;
        addObjectButton->setBounds(offset, 5, 35, 35);
        offset += 50;
        viewButton->setBounds(offset, 5, 35, 35);
        offset += 50;
        resizeToFit->setBounds(offset, 5, 35, 35);
        offset += 50;
        zoomSlider->setBounds(offset, 5, 70, 35);
    }

    void render(NVGcontext* nvg) override
    {
        auto dropShadowCol = nvgRGBA(0, 0, 0, 30);
        auto dropShadowCornerRadius = (getHeight() + 6) / 2;

        auto bgCol = nvgRGB(43, 43, 43);
        auto outLineCol = nvgRGB(53, 53, 53);
        auto cornerRadius = getHeight() / 2;

        nvgDrawRoundedRect(nvg, - 3,  - 3, width + 6, height + 6, dropShadowCol, dropShadowCol, dropShadowCornerRadius);
        nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, cornerRadius);
    }

private:
    Canvas* cnv;

    std::unique_ptr<ToggleButton> editButton;
    std::unique_ptr<ToggleButton> addObjectButton;
    std::unique_ptr<ToggleButton> viewButton;
    std::unique_ptr<ToggleButton> resizeToFit;
    std::unique_ptr<ZoomSlider> zoomSlider;
};
