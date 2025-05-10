#include "WindowPeer.h"
#include "../Glad/gl.h"
#include <iostream>

WindowPeer::WindowPeer(const std::string &title, int width, int height, const bool isMaximized)
    : window(nullptr), glContext(nullptr)
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        throw std::runtime_error(SDL_GetError());
    }

    isWindowMaximized = isMaximized;

    // Set attributes for an OpenGL context (adjust as needed)
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | (isWindowMaximized ? SDL_WINDOW_MAXIMIZED : 0));
    if (!window) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetWindowMinimumSize(window, 800, 600);

    SDL_GetWindowSizeInPixels(window, &width, &height);
    setUserSize(width, height);

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_DestroyWindow(window);
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGL(SDL_GL_GetProcAddress))
    {
        throw std::runtime_error("Failed to load GLAD GL");
    }

    SDL_GL_SetSwapInterval(0);
}

WindowPeer::~WindowPeer() {
    if (glContext) {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

bool WindowPeer::isMaximized() const
{
    return isWindowMaximized;
}

SDL_Window* WindowPeer::getSDLWindow() const {
    return window;
}

void WindowPeer::swapBuffers() {
    SDL_GL_SwapWindow(window);
}

void WindowPeer::setTitle(const std::string &title) {
    SDL_SetWindowTitle(window, title.c_str());
}

void WindowPeer::setSize(const int width, const int height) {
    SDL_SetWindowSize(window, width, height);
    int w, h;
    SDL_GetWindowSizeInPixels(window, &w, &h);
    setUserSize(w, h);
}

void WindowPeer::setUserSize(const int width, const int height)
{
    windowUserWidth = width;
    windowUserHeight = height;
}
void WindowPeer::getUserSize(int &width, int &height) const
{
    width = windowUserWidth;
    height = windowUserHeight;
}
