#pragma once

class PatchformBuildMode {
public:
    enum class Mode { Unknown, Standalone, Plugin };

    constexpr PatchformBuildMode(Mode m) : mode(m) {
        instance = this;
    }

    static const PatchformBuildMode& get() {
        return *instance;
    }

    static bool isPlugin()     { return get().mode == Mode::Plugin; }
    static bool isStandalone() { return get().mode == Mode::Standalone; }
    static Mode getMode()      { return get().mode; }

private:
    Mode mode;
    static inline const PatchformBuildMode* instance = nullptr;
};
