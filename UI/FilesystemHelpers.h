#pragma once

#include <filesystem>
#include <string>

#ifdef _WIN32
    #include <direct.h> // for _fullpath
    #define PATH_MAX MAX_PATH
#else
    #include <limits.h> // for PATH_MAX
    #include <unistd.h> // for realpath
#endif

namespace FilesystemHelpers
{
    inline std::string normalizePath(const std::string& in)
    {
        char out[PATH_MAX];

#ifdef _WIN32
        return _fullpath(out, in.c_str(), PATH_MAX) ? std::string(out) : in;
#else
        return realpath(in.c_str(), out) ? std::string(out) : in;
#endif
    }

    inline std::string getStem(const std::string& path)
    {
        return std::filesystem::path(path).stem().string();
    }
}


