#pragma once
#import <AppKit/AppKit.h>

class PatchformApp;

@interface PatchformGLView : NSOpenGLView {
    CFRunLoopTimerRef timer;
    PatchformApp* app;
}

- (instancetype)initWithFrame:(NSRect)frame app:(PatchformApp*)pfApp;
- (void)startTimer;
- (void)stopTimer;

@end