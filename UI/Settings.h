#pragma once
#include <string>

class Settings {
public:
    int selectedApiIndex = -1;
    int selectedInputDeviceIndex = -1;
    int selectedOutputDeviceIndex = -1;

    int windowWidth = -1;
    int windowHeight = -1;

    bool windowIsMaximized = false;

    bool load();
    void save() const;
};