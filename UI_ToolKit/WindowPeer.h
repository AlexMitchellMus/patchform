#pragma once

#include "SDL3/SDL.h"
#include <string>
#include <stdexcept>

class WindowPeer {
public:
    // Constructs the window with a title, width, and height.
    WindowPeer(const std::string &title, int width, int height);
    ~WindowPeer();

    // Accessor for the underlying SDL_Window (if needed for low-level operations).
    SDL_Window* getSDLWindow() const;

    // Swap the window buffers (for OpenGL rendering).
    void swapBuffers();

    // Set the window title.
    void setTitle(const std::string &title);

    // Set the window size.
    void setSize(int width, int height);

    // Retrieve window dimensions.
    void getSize(int &width, int &height) const;

private:
    SDL_Window* window;
    SDL_GLContext glContext;
};
