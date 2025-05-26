#pragma once
#import <AppKit/AppKit.h>

class PatchformApp;

@interface PatchformGLView : NSOpenGLView {
    CFRunLoopTimerRef timer;
    PatchformApp* app;
}

@property (nonatomic) CVDisplayLinkRef displayLink;

@property (nonatomic, assign) double lastFrameTime;

- (instancetype)initWithFrame:(NSRect)frame app:(PatchformApp*)pfApp;
- (void)startTimer;
- (void)stopTimer;

@end