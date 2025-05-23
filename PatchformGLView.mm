#import "PatchformGLView.h"
#include "Glad/gl.h"
#include "PluginLogger.h"

void* NSGLGetProcAddress(const char* name) {
    CFStringRef symbol = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);
    CFBundleRef bundle = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
    void* addr = CFBundleGetFunctionPointerForName(bundle, symbol);
    CFRelease(symbol);
    return addr;
}

@implementation PatchformGLView

- (instancetype)initWithFrame:(NSRect)frame {
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
        [self.openGLContext makeCurrentContext];
    }

    if (!gladLoadGL((GLADloadfunc)NSGLGetProcAddress)) {
       logToFile("Failed to load OpenGL with GLAD");
    } else {
       const char* version = (const char*)glGetString(GL_VERSION);
       logToFile(std::string("GLAD initialized, OpenGL version: ") + version);
    }

    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    [[self openGLContext] makeCurrentContext];
    glClearColor(0.5f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    [[self openGLContext] flushBuffer];
}

@end
