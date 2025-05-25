#import <Cocoa/Cocoa.h>
#include "PlatformHelpers.h"
#include "SDL3/SDL.h"
#include "UI_ToolKit/WindowPeer.h"
#include <iostream>

namespace PlatformHelpers {

std::string OpenFileChooserDialog(const WindowPeer* peer) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];

        std::string result;
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            result = std::string([[url path] UTF8String]);
        }

        SDL_RaiseWindow(peer->getNativeHandleAs<SDL_Window>());
        return result;
    }
}

std::string SaveFileChooserDialog(const WindowPeer* peer, const std::string& existingPath) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        if (!existingPath.empty())
            [panel setNameFieldStringValue:[NSString stringWithUTF8String:existingPath.c_str()]];

        std::string result;
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            result = std::string([[url path] UTF8String]);
        }

        SDL_RaiseWindow(peer->getNativeHandleAs<SDL_Window>());
        return result;
    }
}

} // namespace PlatformHelpers
