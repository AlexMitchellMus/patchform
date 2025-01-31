#include <string>
#include <sstream>
#include <iostream>

#define GLAD_GL_IMPLEMENTATION
#include "Glad/gl.h"

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include "UI/App.h"
#include "UI_ToolKit/EventManager.h"

// Remove window titlebar- more work to do
void AdjustWindowSize(SDL_Window *window) {
    HWND hwnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (hwnd) {
        // Modify window style to remove the title bar
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style &= ~(WS_SYSMENU | WS_CAPTION); // Remove the title bar
        SetWindowLong(hwnd, GWL_STYLE, style);

        // Calculate the new window size to match the client area
        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);

        RECT rect = {-1, -1, width, height};

        AdjustWindowRect(&rect, style, FALSE);

        int newWidth = rect.right - rect.left;
        int newHeight = rect.bottom - rect.top;

        // Set the new window size
        SetWindowPos(hwnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

        // Refresh the window to apply changes
        SDL_SetWindowSize(window, newWidth, newHeight);
    } else {
        std::cerr << "HWND pointer is invalid" << std::endl;
    }
}

bool resizingEventWatcher(void* data, SDL_Event* event) {
    // TODO: put callback to our own thread safe queue here - moody camel
    // This event watcher will be called from the system/OS thread itself!
    // https://stackoverflow.com/questions/32294913/getting-continuous-window-resize-event-in-sdl-2

    // This is fine, as we will want to have our own event queue anyway to make PlugPatch portable
    switch (event->type)
    {
    case SDL_EVENT_WINDOW_MOVED:
        std::cout << "window moved" << std::endl;
        return true;
    case SDL_EVENT_WINDOW_RESIZED:
        std::cout << "window resized" << std::endl;
        return true;
    default:
        return false;
    }
}

static Uint32 timerEventType = 0;

int main(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to initialize SDL2: %s", SDL_GetError());
        return -1;
    }

    // Reserve a unique event type for timer notifications
    timerEventType = SDL_RegisterEvents(1);
    if (timerEventType == (Uint32)-1) {
        SDL_Log("Failed to register custom event type.\n");
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL,      1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,            1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 		1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 		8);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3); // OpenGL 3.x
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    int windowWidth = 1920;
    int windowHeight = 1080;

    int newWidth = windowWidth;
    int newHeight = windowHeight;

    SDL_Window* window = SDL_CreateWindow("PlugPatch", windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_SetWindowMinimumSize(window, 800, 600);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // Create an OpenGL context
    const auto glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        SDL_Log("Failed to create OpenGL context: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGL(SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Set swap interval to v-sync
    // 0 for immediate updates, 1 for updates synchronized with the vertical retrace, -1 for adaptive vsync
    if (SDL_GL_SetSwapInterval(0) == 0)
        std::cerr << "Failed to set swap interval" << std::endl;

    SDL_ShowWindow(window);
    //AdjustWindowSize(window);

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    NVGcontext* nvg = nvgCreateContext(0);
    if (!nvg) {
        SDL_Log("Failed to initialize NanoVG");
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto app = std::make_unique<App>();
    // FIXME: hack to make the app have a starting size!
    app->setBounds(0, 0, newWidth, newHeight);

    pptk::MouseEventManager eventManager(app.get());

    bool running = true;
    SDL_Event event;

    SDL_AddEventWatch(resizingEventWatcher, nullptr);

    auto* invalidFB = nvgluCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);;

    int regularFont = nvgCreateFont(nvg, "Regular", "Assets/Fonts/Inter_18pt-Regular.ttf");
    if (regularFont == -1) {
        std::cerr << "Failed to load inter font!" << std::endl;
    }

    int semiBold = nvgCreateFont(nvg, "SemiBold", "Assets/Fonts/Inter_18pt-SemiBold.ttf");
    if (semiBold == -1) {
        std::cerr << "Failed to load inter font!" << std::endl;
    }


    int iconFontHandle = nvgCreateFont(nvg, "icons", "Assets/Icons/IconFontPlugPatch_google.ttf");
    if (iconFontHandle == -1) {
        std::cerr << "Failed to load icon font!" << std::endl;
    }

    int objectIconFontHandle = nvgCreateFont(nvg, "object_icons", "Assets/Icons/ObjectIconFont.ttf");
    if (objectIconFontHandle == -1) {
        std::cerr << "Failed to load icon font!" << std::endl;
    }

    const int targetFPS = 120;                       // Desired frame rate
    const int targetFrameTime = 1000 / targetFPS;   // Time per frame in milliseconds

    Uint32 lastFrameTime = 0;                       // Time at the start of the previous frame

    while (running)
    {
        Uint32 currentFrameTime = SDL_GetTicks();

        while (SDL_PollEvent(&event)) {
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                {
                    running = false;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                eventManager.handleMouseButtonDown(event);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                eventManager.handleMouseButtonUp(event);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                eventManager.handleMouseMove(event);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                eventManager.handleMouseWheel(event);
                break;
            case SDL_EVENT_KEY_DOWN:
                eventManager.handleKeyDown(event);
                break;
                //case SDL_EVENT_KEY_UP:
                //    eventManager.handleKeyUp(app.get(), event);
                //    break;
            case SDL_EVENT_WINDOW_RESIZED:
                {
                    newWidth = event.window.data1;
                    newHeight = event.window.data2;
                    app->setBounds(0, 0, newWidth, newHeight);
                }
                break;
            case SDL_EVENT_WINDOW_MOVED:
                std::cout << "----> window moved" << std::endl;
                break;
            default:
                break;
            }
        }

        // Check if the frame time has elapsed
        const bool timeout = (currentFrameTime - lastFrameTime) >= targetFrameTime;

        if (!timeout) {
            // If not enough time has passed, skip drawing
            continue;
        }

        if (!app->needsRepaint()){
            SDL_Delay(1);
            continue;
        }

        app->handleTime(currentFrameTime);

        // Update last frame time for the next frame
        lastFrameTime = currentFrameTime;

        // If we resize the window recreate the framebuffer
        if (newWidth != windowWidth || newHeight != windowHeight)
        {
            windowWidth = newWidth;
            windowHeight = newHeight;

            if (invalidFB)
                nvgluDeleteFramebuffer(invalidFB);

            invalidFB = nvgluCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);
        }

        nvgBindFramebuffer(invalidFB);

        nvgViewport(0, 0, windowWidth, windowHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Begin NanoVG frame
        nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);

        app->renderAll(nvg);

        nvgGlobalScissor(nvg, 0, 0, windowWidth, windowHeight);

        // End NanoVG frame
        nvgEndFrame(nvg);

        nvgBindFramebuffer(nullptr);
        nvgBlitFramebuffer(nvg, invalidFB, 0, 0, windowWidth, windowHeight);

        // Swap the SDL buffers to display the frame
        SDL_GL_SwapWindow(window);

        // DO we even need this delay? Doesn't sound like a good idea to have it!
        //SDL_Delay(1);
    }
    nvgDeleteGL3(nvg);
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
