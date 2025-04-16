#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace FilesystemHelpers
{
    inline std::string normalizePath(const std::string& in)
    {
        char out[MAX_PATH];
        return _fullpath(out, in.c_str(), MAX_PATH) ? std::string(out) : in;
    }

    inline std::string getStem(const std::string& path)
    {
        std::filesystem::path p(path);
        return p.stem().string();
    }
}