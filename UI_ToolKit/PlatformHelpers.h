//
// Created by alexw on 9/02/2025.
//

#pragma once

#include <string>
#include <iostream>

//#include "../external/SDL2/src/video/SDL_sysvideo.h"
#include "SDL3/SDL.h"
#include "../UI_ToolKit/WindowPeer.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#ifdef __APPLE__
    #ifdef __OBJC__
        // We're in Objective-C++ mode on macOS.
        #import <Cocoa/Cocoa.h>
    #endif
#endif

#ifdef __linux__
    // Include tinyfiledialogs for Linux.
    #include "tinyfiledialogs.h"
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace PlatformHelpers
{
#ifdef _WIN32
    static std::string OpenFileChooserDialog(const WindowPeer* peer)
    {
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(peer->getSDLWindow()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

        char filePath[MAX_PATH] = {0};  // Buffer to store the file path.

        // Initialize the OPENFILENAME structure.
        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize  = sizeof(ofn);         // Size of the structure.
        ofn.hwndOwner    = hwnd;                // Owner window STOPS the base window from being selectable when modal dialog is shown.
        ofn.lpstrFile    = filePath;            // Buffer to store the file name.
        ofn.nMaxFile     = MAX_PATH;            // Size of the buffer.
        // Filter format: "Description\0Filter\0", terminated by an extra '\0'.
        ofn.lpstrFilter  = "All Files\0*.*\0Text Files\0*.txt\0";
        ofn.nFilterIndex = 1;                   // The default filter index.
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        // Display the Open dialog box.
        if (GetOpenFileName(&ofn))
        {
            return std::string(filePath);       // Return the selected file path.
        }
        return std::string();                   // Return an empty string if canceled or error.
    }
#endif
} // end namespace PlatformHelpers
