#pragma once

#include <memory>
#include <iostream>
#include <vector>

#include "SDL3/SDL.h"
#include <PortAudio.h>

#include "../Graph/AudioGraph.h"
#include "Editor.h"
#include "../UI_ToolKit/EventManager.h"

#include "../Glad/gl.h"

#include <nanovg.h>
#ifdef NANOVG_GL_IMPLEMENTATION
#    undef NANOVG_GL_IMPLEMENTATION
#    include <nanovg_gl_utils.h>
#    define NANOVG_GL_IMPLEMENTATION 1
#endif

class WindowPeer;

class PatchformApp {
public:
    PatchformApp(int sampleRate, unsigned long frameCount);
    ~PatchformApp();

    bool initialize();
    void shutdown();
    void run();
    void reinitializeAudio();

private:
    int sampleRate;
    unsigned long frameCount;
    PaStream* stream = nullptr;
    GraphManager graphManager;
    std::unique_ptr<Editor> editor;
    std::unique_ptr<pptk::EventManager> eventManager;
    uint32_t lastFrameTime = 0;
    NVGcontext* nvg = nullptr;
    std::unique_ptr<WindowPeer> window;

    int windowWidth;
    int windowHeight;
    int newWidth = 1920;
    int newHeight = 1080;

    int regularFont = -1;
    int semiBoldFont = -1;
    int iconFont = -1;
    int objectIconFont = -1;

    bool loadFonts();

    NVGframebuffer* invalidFB = nullptr;

    // Capping to 60fps reduces CPU time by a factor of 10 (for now before invalidation)
    // But even with invalidation - there will still be a worst case (when everything is updating on canvas drag)
    const float targetFPS = 60;                       // Desired frame rate
    const float targetFrameTime = 1000 / targetFPS;   // Time per frame in milliseconds

    bool initAudio();
    void shutdownAudio();
    bool initUI();
    void render();

    static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData);

};
