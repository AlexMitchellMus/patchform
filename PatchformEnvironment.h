#pragma once
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif

class PatchformEnvironment {
public:
    static void setStandalone()
    {
#ifdef _WIN32
        SetEnvironmentVariableA("PATCHFORM_EXE", "1");
#else
        setenv("PATCHFORM_EXE", "1", 1);
#endif
    }

    static bool isPlugin() {
        return getenv("PATCHFORM_EXE") == nullptr;
    }

    static bool isStandalone() {
        return !isPlugin();
    }
};