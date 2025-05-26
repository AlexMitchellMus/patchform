#include "UI/PatchformApp.h"
#import "PatchformGLViewInternal.h"
#import "PatchformGLView.h"

#include "Glad/gl.h"
#include "PluginLogger.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_keycode.h"
#import <CoreVideo/CoreVideo.h>


SDL_Keycode mapMacKeyCodeToSDLKeycode(unsigned short macKeyCode)
{
    switch (macKeyCode) {
        case 0: return SDLK_A;
        case 1: return SDLK_S;
        case 2: return SDLK_D;
        case 3: return SDLK_F;
        case 4: return SDLK_H;
        case 5: return SDLK_G;
        case 6: return SDLK_Z;
        case 7: return SDLK_X;
        case 8: return SDLK_C;
        case 9: return SDLK_V;
        case 11: return SDLK_B;
        case 12: return SDLK_Q;
        case 13: return SDLK_W;
        case 14: return SDLK_E;
        case 15: return SDLK_R;
        case 16: return SDLK_Y;
        case 17: return SDLK_T;
        case 18: return SDLK_1;
        case 19: return SDLK_2;
        case 20: return SDLK_3;
        case 21: return SDLK_4;
        case 22: return SDLK_6;
        case 23: return SDLK_5;
        case 24: return SDLK_EQUALS;
        case 25: return SDLK_9;
        case 26: return SDLK_7;
        case 27: return SDLK_MINUS;
        case 28: return SDLK_8;
        case 29: return SDLK_0;
        case 30: return SDLK_RIGHTBRACKET;
        case 31: return SDLK_O;
        case 32: return SDLK_U;
        case 33: return SDLK_LEFTBRACKET;
        case 34: return SDLK_I;
        case 35: return SDLK_P;
        case 36: return SDLK_RETURN;
        case 37: return SDLK_L;
        case 38: return SDLK_J;
        case 39: return SDLK_APOSTROPHE;
        case 40: return SDLK_K;
        case 41: return SDLK_SEMICOLON;
        case 42: return SDLK_BACKSLASH;
        case 43: return SDLK_COMMA;
        case 44: return SDLK_SLASH;
        case 45: return SDLK_N;
        case 46: return SDLK_M;
        case 47: return SDLK_PERIOD;
        case 49: return SDLK_SPACE;
        case 50: return SDLK_ESCAPE;
        case 51: return SDLK_BACKSPACE;
        case 53: return SDLK_ESCAPE;
        case 123: return SDLK_LEFT;
        case 124: return SDLK_RIGHT;
        case 125: return SDLK_DOWN;
        case 126: return SDLK_UP;
        default: return SDLK_UNKNOWN;
    }
}

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

- (void)keyDown:(NSEvent *)event {
    if (!app) return;

    SDL_Event sdlEvent = {};
    sdlEvent.type = SDL_EVENT_KEY_DOWN;
    sdlEvent.key.timestamp = SDL_GetTicks();
    sdlEvent.key.windowID = 0;
    sdlEvent.key.which = 0;
    sdlEvent.key.scancode = (SDL_Scancode)event.keyCode;
    sdlEvent.key.key = mapMacKeyCodeToSDLKeycode(event.keyCode);
    sdlEvent.key.mod = SDL_GetModState();
    sdlEvent.key.raw = event.keyCode;
    sdlEvent.key.down = true;
    sdlEvent.key.repeat = event.isARepeat;
    LOG_TO_FILE("event.keyCode: " << event.keyCode);

    app->pendingEvents.enqueue(sdlEvent);
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
        sdlEvent.button.clicks = (Uint8)event.clickCount;
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


