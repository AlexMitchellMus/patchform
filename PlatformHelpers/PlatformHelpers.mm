#import <Cocoa/Cocoa.h>
#include "PlatformHelpers.h"
#include "SDL3/SDL.h"
#include "UI_ToolKit/WindowPeer.h"

namespace PlatformHelpers {

std::string OpenFileChooserDialog(const WindowPeer*) {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            return std::string([[url path] UTF8String]);
        }
        return {};
    }
}

std::string SaveFileChooserDialog(const WindowPeer*, const std::string& existingPath) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];
        if (!existingPath.empty())
            [panel setNameFieldStringValue:[NSString stringWithUTF8String:existingPath.c_str()]];

        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [panel URL];
            return std::string([[url path] UTF8String]);
        }
        return {};
    }
}

} // namespace PlatformHelpers
