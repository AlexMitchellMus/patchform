#pragma once

#include "SDL3/SDL.h"
#include <string>
#include <stdexcept>

class WindowPeer {
public:
    // Constructs the window with a title, width, and height.
    WindowPeer(const std::string &title, int width, int height, bool isFullScreen);
    ~WindowPeer();

    // Accessor for the underlying SDL_Window (if needed for low-level operations).
    SDL_Window* getSDLWindow() const;

    // Swap the window buffers (for OpenGL rendering).
    void swapBuffers();

    // Set the window title.
    void setTitle(const std::string &title);

    bool isMaximized() const;
    void setMaximized(const bool isMaximized)
    {
        isHandlingProgrammaticResize = true;
        isWindowMaximized = isMaximized;
    };

    void setUserSize(int width, int height);
    void getUserSize(int &width, int &height) const;

    // Set the window size.
    void setSize(int width, int height);

    bool getIsProgrammaticResize()
    {
        const bool wasProgrammatic = isHandlingProgrammaticResize;
        isHandlingProgrammaticResize = false;
        return wasProgrammatic;
    }

private:
    SDL_Window* window;
    SDL_GLContext glContext;

    int windowUserWidth = -1;
    int windowUserHeight = -1;
    bool isWindowMaximized = false;

    bool isHandlingProgrammaticResize = false;
};
