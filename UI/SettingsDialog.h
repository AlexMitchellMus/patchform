#pragma once

#include "PatchformApp.h"
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
        auto app = PatchformApp::getApp();
        if (!app) return;

        // Title
        audioLabel = std::make_unique<pptk::Label>("Audio Settings:");
        addComponent(audioLabel.get());

        // --- Driver Dropdown ---
        driverLabel = std::make_unique<pptk::Label>("Driver:");
        addComponent(driverLabel.get());

        auto drivers = app->getAvailableAudioApis();
        driverDropdown = std::make_unique<pptk::DropdownSelector>(drivers);
        addComponent(driverDropdown.get());

        driverDropdown->setOnSelect([this, app](const std::string& selectedApi)
        {
            auto apis = app->getAvailableAudioApis();
            auto it = std::find(apis.begin(), apis.end(), selectedApi);
            if (it == apis.end()) return;

            int apiIndex = static_cast<int>(std::distance(apis.begin(), it));
            selectedApiIndex = apiIndex;

            std::vector<std::string> inputDevices, outputDevices;
            int deviceCount = Pa_GetDeviceCount();

            for (int i = 0; i < deviceCount; ++i)
            {
                const PaDeviceInfo* dev = Pa_GetDeviceInfo(i);
                if (!dev || dev->hostApi != apiIndex) continue;

                std::string name = dev->name;
                if (dev->maxInputChannels > 0)
                    inputDevices.push_back(name);
                if (dev->maxOutputChannels > 0)
                    outputDevices.push_back(name);
            }

            inputDevices.insert(inputDevices.begin(), "None");
            outputDevices.insert(outputDevices.begin(), "None");

            inputDeviceDropdown->updateOptions(inputDevices);
            outputDeviceDropdown->updateOptions(outputDevices);

            app->setAudioDriver(apiIndex);
        });

        // --- Device Dropdown ---
        inputDeviceLabel = std::make_unique<pptk::Label>("Input Device:");
        addComponent(inputDeviceLabel.get());

        inputDeviceDropdown = std::make_unique<pptk::DropdownSelector>(std::vector<std::string>{});
        addComponent(inputDeviceDropdown.get());

        inputDeviceDropdown->setOnSelect([this, app](const std::string& selectedDevice)
        {
            if (selectedApiIndex < 0) return;

            if (selectedDevice == "None")
            {
                app->setAudioInputDevice(selectedApiIndex, -1);
                inputChannelsLabel->setText("Input Channels: 0");
                createInputOutputChannelToggles(0);
                return;
            }

            auto allDevices = app->getAvailableDevices(selectedApiIndex);
            auto it = std::find(allDevices.begin(), allDevices.end(), selectedDevice);
            if (it != allDevices.end())
            {
                int globalIndex = static_cast<int>(std::distance(allDevices.begin(), it));
                const PaDeviceInfo* info = app->setAudioInputDevice(selectedApiIndex, globalIndex);
                if (info)
                {
                    inputChannelsLabel->setText("Input Channels: " + std::to_string(info->maxInputChannels));
                    createInputOutputChannelToggles(info->maxInputChannels);
                }
            }
        });

        inputChannelsLabel = std::make_unique<pptk::Label>("Input Channels:");
        addComponent(inputChannelsLabel.get());

        // --- Output Device Dropdown ---
        outputDeviceLabel = std::make_unique<pptk::Label>("Output Device:");
        addComponent(outputDeviceLabel.get());

        outputDeviceDropdown = std::make_unique<pptk::DropdownSelector>(std::vector<std::string>{});
        addComponent(outputDeviceDropdown.get());

        outputDeviceDropdown->setOnSelect([this, app](const std::string& selectedDevice)
        {
            if (selectedApiIndex < 0) return;

            if (selectedDevice == "None")
            {
                app->setAudioOutputDevice(selectedApiIndex, -1);
                outputChannelsLabel->setText("Output Channels: 0");
                createInputOutputChannelToggles(0, true);
                return;
            }

            auto allDevices = app->getAvailableDevices(selectedApiIndex);
            auto it = std::find(allDevices.begin(), allDevices.end(), selectedDevice);
            if (it != allDevices.end())
            {
                int globalIndex = static_cast<int>(std::distance(allDevices.begin(), it));
                const PaDeviceInfo* info = app->setAudioOutputDevice(selectedApiIndex, globalIndex);
                if (info)
                {
                    outputChannelsLabel->setText("Output Channels: " + std::to_string(info->maxOutputChannels));
                    createInputOutputChannelToggles(info->maxOutputChannels, true);
                }
            }
        });

        // Output Channels label.
        outputChannelsLabel = std::make_unique<pptk::Label>("Output Channels:");
        addComponent(outputChannelsLabel.get());

        updateAudioProperties();
    }

    void updateAudioProperties()
    {
        auto* app = PatchformApp::getApp();
        if (!app) return;

        int currentApi = app->getSelectedApiIndex() >= 0 ? app->getSelectedApiIndex() : Pa_GetDefaultHostApi();
        selectedApiIndex = currentApi;

        auto drivers = app->getAvailableAudioApis();
        driverDropdown->updateOptions(drivers);
        if (currentApi < drivers.size())
            driverDropdown->setSelected(drivers[currentApi]);

        std::vector<std::string> inputDevices = {"None"};
        std::vector<std::string> outputDevices = {"None"};

        int selectedInput = app->getSelectedInputDeviceIndex();
        int selectedOutput = app->getSelectedOutputDeviceIndex();

        for (int i = 0; i < Pa_GetDeviceCount(); ++i)
        {
            const PaDeviceInfo* dev = Pa_GetDeviceInfo(i);
            if (!dev || dev->hostApi != currentApi) continue;

            if (dev->maxInputChannels > 0)
                inputDevices.push_back(dev->name);
            if (dev->maxOutputChannels > 0)
                outputDevices.push_back(dev->name);
        }

        inputDeviceDropdown->updateOptions(inputDevices);
        outputDeviceDropdown->updateOptions(outputDevices);

        if (selectedInput >= 0)
        {
            const PaDeviceInfo* dev = Pa_GetDeviceInfo(selectedInput);
            if (dev)
            {
                inputDeviceDropdown->setSelected(dev->name);
                inputChannelsLabel->setText("Input Channels: " + std::to_string(dev->maxInputChannels));
                createInputOutputChannelToggles(dev->maxInputChannels, false);
            }
        }
        else
        {
            inputDeviceDropdown->setSelected("None");
            inputChannelsLabel->setText("Input Channels: 0");
            createInputOutputChannelToggles(0, false);
        }

        if (selectedOutput >= 0)
        {
            const PaDeviceInfo* dev = Pa_GetDeviceInfo(selectedOutput);
            if (dev)
            {
                outputDeviceDropdown->setSelected(dev->name);
                outputChannelsLabel->setText("Output Channels: " + std::to_string(dev->maxOutputChannels));
                createInputOutputChannelToggles(dev->maxOutputChannels, true);
            }
        }
        else
        {
            outputDeviceDropdown->setSelected("None");
            outputChannelsLabel->setText("Output Channels: 0");
            createInputOutputChannelToggles(0, true);
        }
    }

    void createInputOutputChannelToggles(int count, bool isOutput = false)
    {
        auto& labelList = isOutput ? outputChannelLabels : inputChannelLabels;
        auto& toggleList = isOutput ? outputChannelToggles : inputChannelToggles;

        for (auto& label : labelList) removeComponent(label.get());
        for (auto& toggle : toggleList) removeComponent(toggle.get());
        labelList.clear();
        toggleList.clear();

        for (int i = 0; i < count; ++i)
        {
            auto label = std::make_unique<pptk::Label>("Channel " + std::to_string(i + 1));
            auto toggle = std::make_unique<pptk::ToggleSwitch>();
            toggle->onToggle = [i, isOutput](bool state)
            {
                const char* type = isOutput ? "Output" : "Input";
                std::cout << type << " Channel " << (i + 1) << " toggled " << (state ? "On" : "Off") << std::endl;
            };

            addComponent(label.get());
            addComponent(toggle.get());

            labelList.push_back(std::move(label));
            toggleList.push_back(std::move(toggle));
        }

        resized();
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
        driverDropdown->setBounds(margin + labelWidth, y, getWidth() - (margin * 2 + labelWidth), dropdownHeight);
        y += labelHeight + spacing;

        // Layout Input Device controls.
        inputDeviceLabel->setBounds(margin, y, labelWidth, labelHeight);
        inputDeviceDropdown->setBounds(margin + labelWidth, y, getWidth() - (margin * 2 + labelWidth), dropdownHeight);
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

        // NEW: Layout Output Device controls.
        outputDeviceLabel->setBounds(margin, y, labelWidth, labelHeight);
        outputDeviceDropdown->setBounds(margin + labelWidth, y, getWidth() - (margin * 2 + labelWidth), dropdownHeight);
        y += labelHeight + spacing;

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
    int selectedApiIndex = -1;

    // Top-level labels and dropdowns.
    std::unique_ptr<pptk::Label> audioLabel;
    std::unique_ptr<pptk::Label> driverLabel;
    std::unique_ptr<pptk::DropdownSelector> driverDropdown;

    std::unique_ptr<pptk::Label> inputDeviceLabel;
    std::unique_ptr<pptk::DropdownSelector> inputDeviceDropdown;

    std::unique_ptr<pptk::Label> outputDeviceLabel;
    std::unique_ptr<pptk::DropdownSelector> outputDeviceDropdown;

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
