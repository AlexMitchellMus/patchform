#pragma once

#include <string>
#include <stdexcept>

class WindowPeer {
public:
    virtual ~WindowPeer() = default;
    // TODO: assert the type with a enum that we set when we create the type, SDL-Cocoa-Win32
    template<typename T>
    T* getNativeHandleAs() const { return static_cast<T*>(getNativeHandle());}
    virtual void swapBuffers() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void setUserSize(int width, int height) = 0;
    virtual void getUserSize(int& width, int& height) const = 0;
    virtual void getWindowSize(int& width, int& height) const = 0;

    // Get the window size in real pixels (for HiDPI)
    virtual void getDrawableSize(int& width, int& height) const = 0;
    virtual void setSize(int width, int height) = 0;
    virtual bool isMaximized() const = 0;
    virtual void setMaximized(bool isMaximized) = 0;
    virtual bool getIsProgrammaticResize() = 0;
private:
    virtual void* getNativeHandle() const = 0;
};