#pragma once

#include "UI_ToolKit/Label.h"
#include "UI_ToolKit/DropdownSelector.h"
#include "UI_ToolKit/PopupListComponent.h"
#include "UI_ToolKit/ToggleSwitch.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>

// AudioSettingsPanel extends pptk::Component to include several audio settings controls.
class AudioSettingsPanel : public pptk::Component
{
public:
    AudioSettingsPanel()
    {
        // Audio Settings Title
        audioLabel = std::make_unique<pptk::Label>("Audio Settings:");
        addComponent(audioLabel.get());

        // Driver controls.
        driverLabel = std::make_unique<pptk::Label>("Driver:");
        addComponent(driverLabel.get());
        driverDropdown = std::make_unique<pptk::DropdownSelector>(std::vector<std::string>{
            "ASIO", "WASAPI", "DirectSound"
        });
        driverDropdown->setOnSelect([](const std::string& selected) {
            std::cout << "Selected audio driver: " << selected << std::endl;
        });
        addComponent(driverDropdown.get());

        // Input Device controls.
        inputDeviceLabel = std::make_unique<pptk::Label>("Input Device:");
        addComponent(inputDeviceLabel.get());
        inputDeviceDropdown = std::make_unique<pptk::DropdownSelector>(std::vector<std::string>{
            "Device A", "Device B", "Device C"
        });
        inputDeviceDropdown->setOnSelect([](const std::string& selected) {
            std::cout << "Selected input device: " << selected << std::endl;
        });
        addComponent(inputDeviceDropdown.get());

        // Input Channels label.
        inputChannelsLabel = std::make_unique<pptk::Label>("Input Channels:");
        addComponent(inputChannelsLabel.get());

        // Create a couple of input channel rows (for example purposes, 2 channels).
        for (int i = 0; i < 2; ++i)
        {
            auto channelLabel = std::make_unique<pptk::Label>("Channel " + std::to_string(i + 1));
            auto toggle = std::make_unique<pptk::ToggleSwitch>();
            // Optionally, you could attach a callback to each toggle switch.
            toggle->onToggle = [i](bool state) {
                std::cout << "Input Channel " << (i + 1) << " toggled " << (state ? "On" : "Off") << std::endl;
            };

            inputChannelLabels.push_back(std::move(channelLabel));
            inputChannelToggles.push_back(std::move(toggle));
            addComponent(inputChannelLabels.back().get());
            addComponent(inputChannelToggles.back().get());
        }

        // Output Channels label.
        outputChannelsLabel = std::make_unique<pptk::Label>("Output Channels:");
        addComponent(outputChannelsLabel.get());

        // Create a couple of output channel rows (again, 2 channels for demonstration).
        for (int i = 0; i < 2; ++i)
        {
            auto channelLabel = std::make_unique<pptk::Label>("Channel " + std::to_string(i + 1));
            auto toggle = std::make_unique<pptk::ToggleSwitch>();
            toggle->onToggle = [i](bool state) {
                std::cout << "Output Channel " << (i + 1) << " toggled " << (state ? "On" : "Off") << std::endl;
            };

            outputChannelLabels.push_back(std::move(channelLabel));
            outputChannelToggles.push_back(std::move(toggle));
            addComponent(outputChannelLabels.back().get());
            addComponent(outputChannelToggles.back().get());
        }
    }

    void resized() override
    {
        int y = 0;
        const int spacing = 5;
        const int margin = 10;
        const int labelHeight = 24;
        const int dropdownHeight = 24;
        const int toggleWidth = 40;
        const int toggleHeight = 24;
        const int labelWidth = 100;
        const int dropdownWidth = 150;

        // Layout Audio Settings Title.
        audioLabel->setBounds(margin, y + margin, 200, labelHeight);
        y += margin + labelHeight + spacing;

        // Layout Driver controls.
        driverLabel->setBounds(margin, y, labelWidth, labelHeight);
        driverDropdown->setBounds(margin + labelWidth, y, dropdownWidth, dropdownHeight);
        y += labelHeight + spacing;

        // Layout Input Device controls.
        inputDeviceLabel->setBounds(margin, y, labelWidth, labelHeight);
        inputDeviceDropdown->setBounds(margin + labelWidth, y, dropdownWidth, dropdownHeight);
        y += labelHeight + spacing;

        // Layout Input Channels.
        inputChannelsLabel->setBounds(margin, y, 200, labelHeight);
        y += labelHeight + spacing;

        for (size_t i = 0; i < inputChannelLabels.size(); ++i)
        {
            inputChannelLabels[i]->setBounds(margin + 20, y, 100, labelHeight);
            inputChannelToggles[i]->setBounds(margin + 130, y, toggleWidth, toggleHeight);
            y += labelHeight + spacing;
        }

        // Layout Output Channels.
        outputChannelsLabel->setBounds(margin, y, 200, labelHeight);
        y += labelHeight + spacing;

        for (size_t i = 0; i < outputChannelLabels.size(); ++i)
        {
            outputChannelLabels[i]->setBounds(margin + 20, y, 100, labelHeight);
            outputChannelToggles[i]->setBounds(margin + 130, y, toggleWidth, toggleHeight);
            y += labelHeight + spacing;
        }
    }

private:
    // Top-level labels and dropdowns.
    std::unique_ptr<pptk::Label> audioLabel;
    std::unique_ptr<pptk::Label> driverLabel;
    std::unique_ptr<pptk::DropdownSelector> driverDropdown;

    std::unique_ptr<pptk::Label> inputDeviceLabel;
    std::unique_ptr<pptk::DropdownSelector> inputDeviceDropdown;

    // Input channels.
    std::unique_ptr<pptk::Label> inputChannelsLabel;
    std::vector<std::unique_ptr<pptk::Label>> inputChannelLabels;
    std::vector<std::unique_ptr<pptk::ToggleSwitch>> inputChannelToggles;

    // Output channels.
    std::unique_ptr<pptk::Label> outputChannelsLabel;
    std::vector<std::unique_ptr<pptk::Label>> outputChannelLabels;
    std::vector<std::unique_ptr<pptk::ToggleSwitch>> outputChannelToggles;
};


class SettingsView : public pptk::ComponentViewport
{
public:
    SettingsView()
    {
        auto panel = std::make_unique<AudioSettingsPanel>();
        panel->setBounds(0, 0, getWidth(), 800);
        setViewport(std::move(panel));
    }

    void onScroll() override
    {
        if (auto popup = getPopupComponent())
            popup->close();

        ComponentViewport::onScroll();
    }

    void renderViewportBackground(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, 0, 0, width, height, bg, outline, 6.0f);
    }

    void resized() override
    {
        if (auto viewed = getViewedComponent())
            viewed->setBounds(0, 0, getWidth(), 800);

        ComponentViewport::resized();
    }

private:
    NVGcolor bg = nvgRGB(46, 46, 46);
    NVGcolor outline = nvgRGB(53, 53, 53);
};

class SettingsDialog : public pptk::Component
{
public:
    SettingsDialog()
    {
        settingsView = std::make_unique<SettingsView>();
        addComponent(settingsView.get());

        SettingsDialog::resized();
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, -3, -3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);
    }

    void resized() override
    {
        settingsView->setBounds(20, 20, getWidth() - 40, getHeight() - 40);
    }

private:
    std::unique_ptr<SettingsView> settingsView;

    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);
};
