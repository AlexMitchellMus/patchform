#include "UI/PatchformApp.h"
#import "PatchformGLViewInternal.h"
#import "PatchformGLView.h"

#include "Glad/gl.h"
#include "PluginLogger.h"
#include "SDL3/SDL.h"
#import <CoreVideo/CoreVideo.h>

void* NSGLGetProcAddress(const char* name) {
    CFStringRef symbol = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    void* addr = CFBundleGetFunctionPointerForName(bundle, symbol);
    CFRelease(symbol);
    return addr;
}

@implementation PatchformGLView {
    NSTrackingArea* trackingArea;
    CVDisplayLinkRef displayLink;
    float lastMouseX;
    float lastMouseY;
}

@synthesize lastFrameTime;

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
    if (!self) return nil;

    app = pfApp;
    [self.openGLContext makeCurrentContext];
    GLint swapInt = 0;
    [[self openGLContext] setValues:&swapInt forParameter:NSOpenGLContextParameterSwapInterval];


    if (!gladLoadGL((GLADloadfunc)NSGLGetProcAddress)) {
        LOG_TO_FILE("Failed to load OpenGL with GLAD");
    } else {
        const char* version = (const char*)glGetString(GL_VERSION);
        LOG_TO_FILE(std::string("GLAD initialized, OpenGL version: ") + version);
    }

    [self setNeedsDisplay:YES]; // Ensure first draw
    return self;
}

static CVReturn displayLinkCallback(CVDisplayLinkRef,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags,
                                    CVOptionFlags*,
                                    void* userInfo)
{
    auto view = (__bridge PatchformGLView*)userInfo;
    if (!view || ![view window])
        return kCVReturnSuccess;

    dispatch_async(dispatch_get_main_queue(), ^{
        [view setNeedsDisplay:YES];
    });

    // Static state for delta time
    static uint64_t lastTime = 0;
    static double timebase = 0.0;

    if (timebase == 0.0) {
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);
        timebase = (double)info.numer / (double)info.denom / 1e9;
    }

    uint64_t thisTime = now->hostTime;
    double deltaMs = lastTime ? (thisTime - lastTime) * timebase * 1000.0 : 0.0;
    lastTime = thisTime;

    view.lastFrameTime = deltaMs;

    //LOG_TO_FILE("Frame time: " + std::to_string(deltaMs) + " ms");

    return kCVReturnSuccess;
}

- (void)startDisplayLinkIfNeeded {
    if (displayLink) return;

    CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
    CVDisplayLinkSetOutputCallback(displayLink, &displayLinkCallback, (__bridge void*)self);
    CVDisplayLinkStart(displayLink);
}

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeFirstResponder:self];
    [self updateTrackingAreas];

    [self setNeedsDisplay:YES];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)dealloc {
    [self stopTimer];
}

- (void)stopTimer {
    if (displayLink) {
        CVDisplayLinkStop(displayLink);
        CVDisplayLinkRelease(displayLink);
        displayLink = nil;
    }
}

- (void)updateTrackingAreas {
    [super updateTrackingAreas];

    if (trackingArea)
        [self removeTrackingArea:trackingArea];

    NSTrackingAreaOptions options =
            NSTrackingMouseMoved |
            NSTrackingInVisibleRect |
            NSTrackingActiveAlways |
            NSTrackingEnabledDuringMouseDrag;

    trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds
                                                 options:options
                                                   owner:self
                                                userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseDown:(NSEvent *)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    float mouseX = location.x;
    float mouseY = self.bounds.size.height - location.y;

    LOG_TO_FILE("mouse down at: " + std::to_string(mouseX) + ", " + std::to_string(mouseY));

    if (app) {
        SDL_Event sdlEvent = {};
        sdlEvent.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        sdlEvent.button.timestamp = SDL_GetTicks();
        sdlEvent.button.which = 0;
        sdlEvent.button.button = SDL_BUTTON_LEFT;
        sdlEvent.button.x = mouseX;
        sdlEvent.button.y = mouseY;

        app->pendingEvents.enqueue(sdlEvent);
    }
}

- (void)mouseUp:(NSEvent *)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    float mouseX = location.x;
    float mouseY = self.bounds.size.height - location.y;

    LOG_TO_FILE("mouse up at: " + std::to_string(mouseX) + ", " + std::to_string(mouseY));

    if (app) {
        SDL_Event sdlEvent = {};
        sdlEvent.type = SDL_EVENT_MOUSE_BUTTON_UP;
        sdlEvent.button.timestamp = SDL_GetTicks();
        sdlEvent.button.which = 0;
        sdlEvent.button.button = SDL_BUTTON_LEFT;
        sdlEvent.button.x = mouseX;
        sdlEvent.button.y = mouseY;

        app->pendingEvents.enqueue(sdlEvent);
    }
}

- (void)mouseMoved:(NSEvent *)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    float mouseX = location.x;
    float mouseY = self.bounds.size.height - location.y;

    float xrel = mouseX - lastMouseX;
    float yrel = mouseY - lastMouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    LOG_TO_FILE("mouse pos: " + std::to_string(mouseX) + ", " + std::to_string(mouseY));

    if (app) {
        SDL_Event sdlEvent = {};
        sdlEvent.type = SDL_EVENT_MOUSE_MOTION;
        sdlEvent.motion.timestamp = SDL_GetTicks();
        sdlEvent.motion.which = 0;
        sdlEvent.motion.x = mouseX;
        sdlEvent.motion.y = mouseY;
        sdlEvent.motion.xrel = xrel;
        sdlEvent.motion.yrel = yrel;

        app->pendingEvents.enqueue(sdlEvent);
    }
}

// macOS blocks mouse move when inside a drag
// so give the mousemove the mousedrag
- (void)mouseDragged:(NSEvent *)event
{
    [self mouseMoved:event];
}

- (void)drawRect:(NSRect)dirtyRect {
    LOG_TO_FILE("drawRect called");
    [self.openGLContext makeCurrentContext];
    if (app)
        app->nextFrame();
    else
      LOG_TO_FILE("app not valid");
}

+ (void)makeCurrentContext:(NSView*)view {
    NSOpenGLContext* context = [(NSOpenGLView*)view openGLContext];
    [context makeCurrentContext];
}

@end

void makeGLViewCurrent(void* viewPtr) {
    NSView* view = (__bridge NSView*)viewPtr;
    [[(NSOpenGLView*)view openGLContext] makeCurrentContext];
}
