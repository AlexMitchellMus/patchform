#import <AppKit/AppKit.h>
#import "PluginWindowPeer_mac.h"
#include "../PluginLogger.h"

PluginWindowPeer_mac::PluginWindowPeer_mac(void* nativeHandle)
    : PluginWindowPeer(nativeHandle) {}

void PluginWindowPeer_mac::swapBuffers() {
    NSOpenGLContext* ctx = [NSOpenGLContext currentContext];
    if (ctx) {
      [ctx flushBuffer];
      LOG_TO_FILE("flush buffer");
    }
}

void PluginWindowPeer_mac::getWindowSize(int& w, int& h) const {
    NSView* view = (__bridge NSView*)handle;
    NSRect bounds = [view bounds];
    w = bounds.size.width;
    h = bounds.size.height;
}

void PluginWindowPeer_mac::setTitle(const std::string&) {}

void PluginWindowPeer_mac::setUserSize(int, int) {}
void PluginWindowPeer_mac::getUserSize(int& w, int& h) const {
    w = -1; h = -1;
}

void PluginWindowPeer_mac::getDrawableSize(int& w, int& h) const {
    NSView* view = (__bridge NSView*)handle;
    NSSize size = [view convertRectToBacking:view.bounds].size;
    w = size.width;
    h = size.height;
}

void PluginWindowPeer_mac::setSize(int, int) {}
bool PluginWindowPeer_mac::isMaximized() const { return false; }
void PluginWindowPeer_mac::setMaximized(bool) {}
bool PluginWindowPeer_mac::getIsProgrammaticResize() { return false; }
