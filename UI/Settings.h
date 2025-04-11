#pragma once
#include <string>

class Settings {
public:
    int selectedApiIndex = -1;
    int selectedInputDeviceIndex = -1;
    int selectedOutputDeviceIndex = -1;

    bool load();
    void save() const;
};