#include "UI/PatchformApp.h"
#import "PatchformGLViewInternal.h"
#import "PatchformGLView.h"

#include "Glad/gl.h"
#include "PluginLogger.h"
#include "SDL3/SDL.h"

void* NSGLGetProcAddress(const char* name) {
    CFStringRef symbol = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    void* addr = CFBundleGetFunctionPointerForName(bundle, symbol);
    CFRelease(symbol);
    return addr;
}

@implementation PatchformGLView {
    NSTrackingArea* trackingArea;
}

- (instancetype)initWithFrame:(NSRect)frame app:(PatchformApp*)pfApp {
    NSOpenGLPixelFormatAttribute attrs[] = {
        NSOpenGLPFAAccelerated,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFAColorSize, 24,
        NSOpenGLPFADepthSize, 16,
        0
    };

    NSOpenGLPixelFormat* pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
    self = [super initWithFrame:frame pixelFormat:pixelFormat];

    if (self) {
        app = pfApp;
        [self.openGLContext makeCurrentContext];

        if (!gladLoadGL((GLADloadfunc)NSGLGetProcAddress)) {
            LOG_TO_FILE("Failed to load OpenGL with GLAD");
        } else {
            const char* version = (const char*)glGetString(GL_VERSION);
            LOG_TO_FILE(std::string("GLAD initialized, OpenGL version: ") + version);
        }
    }

    return self;
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    [[self window] setAcceptsMouseMovedEvents:YES];
    [self updateTrackingAreas];
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    if (trackingArea)
        [self removeTrackingArea:trackingArea];

    NSTrackingAreaOptions options =
        NSTrackingMouseMoved |
        NSTrackingActiveInKeyWindow |
        NSTrackingInVisibleRect;

    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    float mouseX = location.x;
    float mouseY = self.bounds.size.height - location.y;

    LOG_TO_FILE("mouse pos: " + std::to_string(mouseX) + ", " + std::to_string(mouseY));

    if (app) {
        SDL_Event sdlEvent = {};
        sdlEvent.type = SDL_EVENT_MOUSE_MOTION;
        sdlEvent.motion.timestamp = SDL_GetTicks();
        sdlEvent.motion.which = 0;
        sdlEvent.motion.x = (Sint32)mouseX;
        sdlEvent.motion.y = (Sint32)mouseY;
        sdlEvent.motion.xrel = 0;
        sdlEvent.motion.yrel = 0;

        app->pendingEvents.enqueue(sdlEvent);
    }
}

- (void)mouseDown:(NSEvent *)event { }
- (void)mouseDragged:(NSEvent *)event { }
- (void)mouseUp:(NSEvent *)event { }

- (void)drawRect:(NSRect)dirtyRect {
    LOG_TO_FILE("drawRect called");
    [self.openGLContext makeCurrentContext];

    if (app)
        app->nextFrame();
}

+ (void)makeCurrentContext:(NSView*)view {
    NSOpenGLContext* context = [(NSOpenGLView*)view openGLContext];
    [context makeCurrentContext];
}

void timerCallback(CFRunLoopTimerRef t, void* info) {
    PatchformGLView* view = (__bridge PatchformGLView*)info;
    [view setNeedsDisplay:YES];
}

- (void)startTimer {
    CFRunLoopTimerContext ctx = {0, self, NULL, NULL, NULL};
    CFAbsoluteTime fire = CFAbsoluteTimeGetCurrent() + (1.0 / 60.0);
    timer = CFRunLoopTimerCreate(NULL, fire, 1.0 / 90.0, 0, 0, timerCallback, &ctx);
    CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
}

- (void)stopTimer {
    if (timer) {
        CFRunLoopRemoveTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
        CFRelease(timer);
        timer = nil;
    }
}

@end

void makeGLViewCurrent(void* viewPtr) {
    NSView* view = (__bridge NSView*)viewPtr;
    [[(NSOpenGLView*)view openGLContext] makeCurrentContext];
}
