#include <memory>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "../Glad/gl.h"

#include <Windows.h>

#include "SDL3/SDL.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl.h>
#    include <nanovg_gl_utils.h>
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "PatchformApp.h"

PatchformApp::PatchformApp(int sampleRate, unsigned long frameCount)
    : graphManager(sampleRate, frameCount), sampleRate(sampleRate), frameCount(frameCount),
      windowWidth(1920), windowHeight(1080){}

PatchformApp::~PatchformApp() {
    shutdown();
}

bool PatchformApp::initialize() {
    if (!initAudio())
    {
        std::cerr << "Failed to initialize Audio" << std::endl;
        return false;
    }
    if (!initUI())
    {
        std::cerr << "Failed to initialize UI" << std::endl;
        return false;
    }
    return true;
}

void PatchformApp::shutdown() {
    shutdownAudio();

    if (invalidFB) {
        nvgDeleteFramebuffer(invalidFB);
        invalidFB = nullptr;
    }

    regularFont = semiBoldFont = iconFont = objectIconFont = -1;

    if (nvg) {
        nvgDeleteContext(nvg);
        nvg = nullptr;
    }

    if (glContext) {
        SDL_GL_DestroyContext(glContext);
        glContext = nullptr;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    SDL_Quit();
}

void PatchformApp::run() {
    bool running = true;
    SDL_Event event;

    while (running) {
        Uint32 currentFrameTime = SDL_GetTicks();
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    running = false;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    eventManager->handleMouseButtonDown(event);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    eventManager->handleMouseButtonUp(event);
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    eventManager->handleMouseMove(event);
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    eventManager->handleMouseWheel(event);
                    break;
                case SDL_EVENT_KEY_DOWN:
                    eventManager->handleKeyDown(event);
                    break;
                case SDL_EVENT_KEY_UP:
                    eventManager->handleKeyUp(event);
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    newWidth = event.window.data1;
                    newHeight = event.window.data2;
                    editor->setBounds(0, 0, newWidth, newHeight);
                    break;
                case SDL_EVENT_WINDOW_MOVED:
                    std::cout << "----> window moved" << std::endl;
                    break;
                default:
                    break;
            }
        }

        if ((currentFrameTime - lastFrameTime) < targetFrameTime) {
            continue;
        }

        editor->updateObjectsFromDSP();
        editor->handleTime(currentFrameTime);
        editor->updateFrameBuffers(nvg);

        if (!editor->needsRepaint()) {
            SDL_Delay(1);
            continue;
        }

        lastFrameTime = currentFrameTime;

        if (!invalidFB || newWidth != windowWidth || newHeight != windowHeight) {
            windowWidth = newWidth;
            windowHeight = newHeight;

            if (invalidFB) {
                nvgDeleteFramebuffer(invalidFB);
                invalidFB = nullptr;
            }

            invalidFB = nvgCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);
        }

        render();
    }
}

void PatchformApp::reinitializeAudio() {
    shutdownAudio();
    initAudio();
}

int PatchformApp::audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graphs = static_cast<GraphManager*>(userData);
    float* out = static_cast<float*>(output);

    std::fill(out, out + frameCount, 0.0f);

    graphs->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cerr << "Audio underflow or overflow detected" << std::endl;
    }

    return paContinue;
}


bool PatchformApp::initAudio() {
    if (Pa_Initialize() != paNoError) return false;

    int deviceIndex = Pa_GetHostApiInfo(2)->defaultOutputDevice;
    if (deviceIndex == paNoDevice) return false;

    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(deviceIndex);

    PaStreamParameters outputParams{};
    outputParams.device = deviceIndex;
    outputParams.channelCount = 1;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;

    if (Pa_OpenStream(&stream, nullptr, &outputParams, sampleRate, frameCount, paClipOff, audioCallback, &graphManager) != paNoError)
        return false;

    return (Pa_StartStream(stream) == paNoError);
}

void PatchformApp::shutdownAudio() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    Pa_Terminate();
}

bool PatchformApp::initUI() {
    if (SDL_Init(SDL_INIT_VIDEO) == 0)
    {
        std::cerr << "Failed to initialize SDL" << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("Patchform", windowWidth, windowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window) return false;

    SDL_SetWindowMinimumSize(window, 800, 600);

    glContext = SDL_GL_CreateContext(window);
    if (!glContext) return false;

    SDL_GL_MakeCurrent(window, glContext);
    if (!gladLoadGL(SDL_GL_GetProcAddress)) return false;

    SDL_GL_SetSwapInterval(0);

    SDL_ShowWindow(window);

    nvg = nvgCreateContext(0);
    if (!nvg) return false;

    if (!loadFonts()) {
        std::cerr << "Failed to load fonts!" << std::endl;
        return false;
    }

    editor = std::make_unique<Editor>();

    std::vector<std::string> fonts = { "Regular", "SemiBold", "icons", "object_icons" };
    std::vector<float> sizes = { 14.0f, 16.0f, 100.0f };

    editor->cacheFontMetrics(nvg, fonts, sizes);

    editor->init(&graphManager);

    eventManager = std::make_unique<pptk::EventManager>(editor.get());

    invalidFB = nvgCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);

    editor->setBounds(0, 0, windowWidth, windowHeight);

    return true;
}

void PatchformApp::render() {
    nvgBindFramebuffer(invalidFB);

    nvgViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(nvg, windowWidth, windowHeight, 1.0f);
    editor->renderAll(nvg);
    nvgGlobalScissor(nvg, 0, 0, windowWidth, windowHeight);
    nvgEndFrame(nvg);

    nvgBindFramebuffer(nullptr);
    nvgBlitFramebuffer(nvg, invalidFB, 0, 0, windowWidth, windowHeight);

    SDL_GL_SwapWindow(window);
}

bool PatchformApp::loadFonts() {
    regularFont = nvgCreateFont(nvg, "Regular", "Assets/Fonts/Inter_18pt-Regular.ttf");
    if (regularFont == -1) {
        std::cerr << "Failed to load Regular font!" << std::endl;
        return false;
    }

    semiBoldFont = nvgCreateFont(nvg, "SemiBold", "Assets/Fonts/Inter_18pt-SemiBold.ttf");
    if (semiBoldFont == -1) {
        std::cerr << "Failed to load SemiBold font!" << std::endl;
        return false;
    }

    iconFont = nvgCreateFont(nvg, "icons", "Assets/Icons/IconFontPlugPatch.ttf");
    if (iconFont == -1) {
        std::cerr << "Failed to load Icons font!" << std::endl;
        return false;
    }

    objectIconFont = nvgCreateFont(nvg, "object_icons", "Assets/Icons/ObjectIconFont.ttf");
    if (objectIconFont == -1) {
        std::cerr << "Failed to load Object Icons font!" << std::endl;
        return false;
    }

    return true;
}

