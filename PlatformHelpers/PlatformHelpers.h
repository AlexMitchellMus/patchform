#pragma once

#include <string>
#include "SDL3/SDL.h"
class WindowPeer;

namespace PlatformHelpers
{
    inline std::string formatKey(SDL_Keycode key, SDL_Keymod mod)
    {
#if defined(__APPLE__)
        std::string out;
        if (mod & SDL_KMOD_CTRL)  out += "⌘+";
        if (mod & SDL_KMOD_SHIFT) out += "⇧+";
        if (mod & SDL_KMOD_ALT)   out += "⌥+";
#else
        std::string out;
        if (mod & SDL_KMOD_CTRL)  out += "Ctrl+";
        if (mod & SDL_KMOD_SHIFT) out += "Shift+";
        if (mod & SDL_KMOD_ALT)   out += "Alt+";
#endif
        out += SDL_GetKeyName(key);
        return out;
    }

    inline void disableDenormalsOncePerThread()
    {
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
        thread_local bool denormalsDisabled = [] {
            _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
            _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
            return true;
        }();
        (void)denormalsDisabled;
#else
        thread_local bool denormalsDisabled = true;
        (void)denormalsDisabled;
#endif
    }

    std::string OpenFileChooserDialog(const WindowPeer* peer);
    std::string SaveFileChooserDialog(const WindowPeer* peer, const std::string& existingPath = {});
}
