#include "Settings.h"
#include <fstream>
#include <filesystem>
#include <iostream>

#include "json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

fs::path getSettingsPath()
{
#if defined(_WIN32)
    char* appdata = std::getenv("APPDATA");
    return fs::path(appdata ? appdata : ".") / "Patchform" / "Patchform.settings";
#elif defined(__APPLE__)
    return fs::path(std::getenv("HOME")) / "Library/Application Support/Patchform/Patchform.settings";
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg)
        return fs::path(xdg) / "Patchform/Patchform.settings";
    return fs::path(std::getenv("HOME")) / ".config/Patchform/Patchform.settings";
#endif
}

bool Settings::load()
{
    auto path = getSettingsPath();
    if (!fs::exists(path)) return false;

    std::ifstream in(path);
    if (!in) return false;

    json j;
    in >> j;
    if (!j.contains("audio")) return false;

    const auto& a = j["audio"];
    selectedApiIndex = a.value("selectedApiIndex", selectedApiIndex);
    selectedInputDeviceIndex = a.value("selectedInputDeviceIndex", selectedInputDeviceIndex);
    selectedOutputDeviceIndex = a.value("selectedOutputDeviceIndex", selectedOutputDeviceIndex);

    if (!j.contains("UI")) return false;

    const auto& ui = j["UI"];
    windowWidth = ui.value("windowWidth", windowWidth);
    windowHeight = ui.value("windowHeight", windowHeight);
    windowIsFullscreen = ui.value("windowIsFullscreen", windowIsFullscreen);

    return true;
}

void Settings::save() const
{
    auto path = getSettingsPath();

    fs::create_directory(path.parent_path()); // just in case

    json j;
    j["audio"] = {
        {"selectedApiIndex", selectedApiIndex},
        {"selectedInputDeviceIndex", selectedInputDeviceIndex},
        {"selectedOutputDeviceIndex", selectedOutputDeviceIndex},
    };

    j["UI"] = {
        {"windowWidth", windowWidth},
        {"windowHeight", windowHeight},
        {"windowIsFullscreen", windowIsFullscreen}
    };

    std::ofstream out(path);
    if (out)
    {
        out << j.dump(4);
        std::cout << "writing settings to: " << path << std::endl;
    } else
        std::cerr << "could not write settings to file" << std::endl;
}