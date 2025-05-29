#import <Cocoa/Cocoa.h>

#include "PlatformHelpers.h"
#include "SDL3/SDL.h"
#include "UI_ToolKit/WindowPeer.h"
#include "../PluginLogger.h"
#include "../PatchformEnvironment.h"

namespace PlatformHelpers {

void OpenFileChooserDialog(const WindowPeer* peer, std::function<void(std::string)> callback) {
    dispatch_async(dispatch_get_main_queue(), ^{
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

            if (peer) {
                if (PatchformEnvironment::isPlugin()) {
                    NSView* view = peer->getNativeHandleAs<NSView>();
                    [[view window] makeKeyAndOrderFront:nil];
                } else {
                    SDL_Window* sdlWindow = peer->getNativeHandleAs<SDL_Window>();
                    auto props = SDL_GetWindowProperties(sdlWindow);
                    NSWindow* nsWindow = static_cast<NSWindow*>(
                    SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr));
                    [[[nsWindow contentView] window] makeKeyAndOrderFront:nil];
                }
            }

            callback(std::move(result));
        }
    });
}

std::string SaveFileChooserDialog(const WindowPeer* peer, const std::string& existingPath) {
    @autoreleasepool {
        NSSavePanel* panel = [NSSavePanel savePanel];

        if (!existingPath.empty()) {
            NSString* nsPath = [NSString stringWithUTF8String:existingPath.c_str()];
            BOOL isDir = NO;
            if ([[NSFileManager defaultManager] fileExistsAtPath:nsPath isDirectory:&isDir]) {
                if (isDir) {
                    [panel setDirectoryURL:[NSURL fileURLWithPath:nsPath]];
                } else {
                    [panel setDirectoryURL:[NSURL fileURLWithPath:[nsPath stringByDeletingLastPathComponent]]];
                    [panel setNameFieldStringValue:[nsPath lastPathComponent]];
                }
            }
        }

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
