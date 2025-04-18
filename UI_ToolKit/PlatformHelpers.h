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
    static std::string formatKey(const SDL_Keycode key, SDL_Keymod mod)
    {
#ifdef __APPLE__
        std::string out;
        if (mod & SDL_KMOD_CTRL)  out += u8"⌘+";
        if (mod & SDL_KMOD_SHIFT) out += u8"⇧+";
        if (mod & SDL_KMOD_ALT)   out += u8"⌥+";
#else
        std::string out;
        if (mod & SDL_KMOD_CTRL)  out += "Ctrl+";
        if (mod & SDL_KMOD_SHIFT) out += "Shift+";
        if (mod & SDL_KMOD_ALT)   out += "Alt+";
#endif
        out += SDL_GetKeyName(key);
        return out;
    }

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

    static std::string SaveFileChooserDialog(const WindowPeer* peer, const std::string& existingPath = {})
    {
        HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(peer->getSDLWindow()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);

        char filePath[MAX_PATH] = {0};
        if (!existingPath.empty())
            strncpy(filePath, existingPath.c_str(), MAX_PATH - 1);

        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize  = sizeof(ofn);
        ofn.hwndOwner    = hwnd;
        ofn.lpstrFile    = filePath;
        ofn.nMaxFile     = MAX_PATH;
        ofn.lpstrFilter  = "Patchform JSON\0*.json\0All Files\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags        = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn))
            return std::string(filePath);

        return {};
    }
#endif

    inline void disableDenormalsOncePerThread()
    {
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
        thread_local bool denormalsDisabled = []
        {
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            return true;
        }();
        (void)denormalsDisabled;
#elif defined(__aarch64__) || defined(__arm__)
        // On ARM, we may need to flush manually, or use architecture-specific intrinsics
        // Most modern ARM chips should avoid denormals entirely in NEON.
        // No-op fallback for now:
        thread_local bool denormalsDisabled = true;
        (void)denormalsDisabled;
#else
        // Fallback: no action
        thread_local bool denormalsDisabled = true;
        (void)denormalsDisabled;
#endif
    }


} // end namespace PlatformHelpers
