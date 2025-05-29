#include "Glad/gl.h"
#include <iostream>
#include "SDLWindowPeer.h"
#include "PluginLogger.h"

SDLWindowPeer::SDLWindowPeer(const std::string& title, int width, int height, bool isFullScreen)
{
    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        throw std::runtime_error(SDL_GetError());
    }

    isWindowMaximized = isFullScreen;

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(title.c_str(), width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                              SDL_WINDOW_HIGH_PIXEL_DENSITY |
                              (isWindowMaximized ? SDL_WINDOW_MAXIMIZED : 0));
    if (!window) {
        throw std::runtime_error(SDL_GetError());
    }

    SDL_SetHint("SDL_TOUCH_MOUSE_EVENTS", "1");
    SDL_SetHint("SDL_MOUSE_TOUCH_EVENTS", "1");
    SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "90");

    SDL_SetWindowMinimumSize(window, 800, 600);

    SDL_GetWindowSizeInPixels(window, &width, &height);
    setUserSize(width, height);

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        SDL_DestroyWindow(window);
        throw std::runtime_error(SDL_GetError());
    }

    if (!SDL_GL_MakeCurrent(window, glContext))
        throw std::runtime_error("Failed to make OpenGL context current");

    int stencilBits = 0;
    int depthBits = 0;
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencilBits);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depthBits);
    std::cout << "SDL_GL_STENCIL_SIZE: " << stencilBits << "\n";
    std::cout << "SDL_GL_DEPTH_SIZE: " << depthBits << "\n";

    if (!gladLoadGL(SDL_GL_GetProcAddress)) {
        throw std::runtime_error("Failed to load GLAD GL");
    }

    std::cout << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;

    SDL_GL_SetSwapInterval(0);
}


SDLWindowPeer::~SDLWindowPeer()
{
    if (glContext)
        SDL_GL_DestroyContext(glContext);
    if (window)
        SDL_DestroyWindow(window);
}

void* SDLWindowPeer::getNativeHandle() const {
    return window;
    /*
#if __APPLE__
    auto props = SDL_GetWindowProperties(window);
    auto ret = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);

    return nullptr;
#else
    auto props = SDL_GetWindowProperties(window);
    auto ret = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    return ret;
#endif
    */
}

void SDLWindowPeer::swapBuffers() {
    LOG_TO_FILE("sdl flush");
    SDL_GL_SwapWindow(window);
}

void SDLWindowPeer::setTitle(const std::string& title) {
    SDL_SetWindowTitle(window, title.c_str());
}

void SDLWindowPeer::setUserSize(int width, int height) {
    windowUserWidth = width;
    windowUserHeight = height;
}

void SDLWindowPeer::getUserSize(int& width, int& height) const {
    width = windowUserWidth;
    height = windowUserHeight;
}

void SDLWindowPeer::getWindowSize(int& w, int& h) const {
    SDL_GetWindowSize(window, &w, &h);
}
void SDLWindowPeer::getDrawableSize(int& w, int& h) const {
    SDL_GetWindowSizeInPixels(window, &w, &h);
}


void SDLWindowPeer::setSize(int width, int height) {
    isHandlingProgrammaticResize = true;
    SDL_SetWindowSize(window, width, height);
}

bool SDLWindowPeer::isMaximized() const {
    return isWindowMaximized;
}

void SDLWindowPeer::setMaximized(bool isMaximized) {
    isHandlingProgrammaticResize = true;
    isWindowMaximized = isMaximized;
}

bool SDLWindowPeer::getIsProgrammaticResize() {
    bool wasProgrammatic = isHandlingProgrammaticResize;
    isHandlingProgrammaticResize = false;
    return wasProgrammatic;
}
