#pragma once

#include "WindowPeer.h"
#include "SDL3/SDL.h"
#include <string>

class SDLWindowPeer : public WindowPeer {
public:
    SDLWindowPeer(const std::string& title, int width, int height, bool isFullScreen);
    ~SDLWindowPeer() override;

    void* getNativeHandle() const override;
    void swapBuffers() override;
    void setTitle(const std::string& title) override;
    void setUserSize(int width, int height) override;
    void getUserSize(int& width, int& height) const override;
    void getWindowSize(int& width, int& height) const override;
    void getDrawableSize(int& width, int& height) const override;
    void setSize(int width, int height) override;
    bool isMaximized() const override;
    void setMaximized(bool isMaximized) override;
    bool getIsProgrammaticResize() override;

    SDL_Window* getSDLWindow() const { return window; }

private:
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;

    int windowUserWidth = -1;
    int windowUserHeight = -1;
    bool isWindowMaximized = false;
    bool isHandlingProgrammaticResize = false;
};
