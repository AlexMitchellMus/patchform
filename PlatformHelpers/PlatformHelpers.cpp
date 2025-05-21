#include "../UI_ToolKit/PlatformHelpers.h"
#include "SDL3/SDL.h"
#include "UI_ToolKit/WindowPeer.h"

#include <string>
#include <iostream>
#include <filesystem>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#elif defined(__linux__)
#include "tinyfiledialogs.h"
#endif

namespace PlatformHelpers {

std::string OpenFileChooserDialog(const WindowPeer* peer) {
#if defined(_WIN32)
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(peer->getSDLWindow()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    char filePath[MAX_PATH] = {0};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "All Files\0*.*\0Text Files\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
        return std::string(filePath);

#elif defined(__linux__)
    const char* result = tinyfd_openFileDialog("Select File", "", 0, NULL, NULL, 0);
    if (result) return std::string(result);
#endif
    return {};
}

std::string SaveFileChooserDialog(const WindowPeer* peer, const std::string& existingPath) {
#if defined(_WIN32)
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(peer->getSDLWindow()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

    char filePath[MAX_PATH] = {0};
    if (!existingPath.empty())
        strncpy(filePath, existingPath.c_str(), MAX_PATH - 1);

    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "JSON Files\0*.json\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn))
        return std::string(filePath);

#elif defined(__linux__)
    const char* result = tinyfd_saveFileDialog("Save File", existingPath.c_str(), 0, NULL, NULL);
    if (result) return std::string(result);
#endif
    return {};
}

} // namespace PlatformHelpers
