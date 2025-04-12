#pragma once

#include <memory>
#include <iostream>
#include <vector>

#include "SDL3/SDL.h"
#include "portaudio.h"
#include "rtmidi.h"

#include "../Graph/AudioGraph.h"
#include "Editor.h"
#include "../UI_ToolKit/EventManager.h"
#include "Settings.h"

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
    static PatchformApp* instance;

    static PatchformApp* getApp() {
        return instance;
    }

    PatchformApp(int sampleRate, unsigned long frameCount);
    ~PatchformApp();

    bool initialize();
    void shutdown();
    void run();

    std::vector<std::string> getAvailableAudioApis();
    std::vector<std::string> getAvailableDevices(int apiIndex);
    bool setAudioDriver(int apiIndex);

    const PaDeviceInfo* setAudioInputDevice(int apiIndex, int deviceIndex) {
        selectedApiIndex = apiIndex;
        selectedInputDeviceIndex = deviceIndex;
        reinitAudio();
        return Pa_GetDeviceInfo(deviceIndex);
    }

    const PaDeviceInfo* setAudioOutputDevice(int apiIndex, int deviceIndex) {
        selectedApiIndex = apiIndex;
        selectedOutputDeviceIndex = deviceIndex;
        reinitAudio();
        return Pa_GetDeviceInfo(deviceIndex);
    }

    int getSelectedApiIndex() const { return selectedApiIndex; }
    int getSelectedInputDeviceIndex() const { return selectedInputDeviceIndex; }
    int getSelectedOutputDeviceIndex() const { return selectedOutputDeviceIndex; }

private:
    Settings settings;

    int sampleRate;
    unsigned long frameCount;
    PaStream* stream = nullptr;

    std::unique_ptr<RtMidiIn> midiIn;
    moodycamel::ConcurrentQueue<MidiMessage> midiQueue;

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
    const uint32_t targetFrameTime = 1000 / targetFPS;   // Time per frame in milliseconds

    bool initAudio();
    void reinitAudio();
    void shutdownAudio();

    std::atomic<int> inputChannels = 0;
    std::atomic<int> outputChannels = 0;
    std::atomic<bool> bypassMode = false;
    std::atomic<bool> shuttingDownAudio = false;

    int selectedApiIndex = -1;
    int selectedDeviceIndex = -1;
    int selectedInputDeviceIndex = -1;
    int selectedOutputDeviceIndex = -1;

    bool initMidi();
    void shutdownMidi();

    bool initUI();
    void render();

    static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData);

};
