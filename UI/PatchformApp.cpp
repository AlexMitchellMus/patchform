#include <memory>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "../Glad/gl.h"

#if __WIN32__
#include <Windows.h>
#endif

#include "SDL3/SDL.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include "PatchformApp.h"
#include "../UI_ToolKit/WindowPeer.h"
#include "../GitInfo.h"

PatchformApp* PatchformApp::instance = nullptr;

PatchformApp::PatchformApp(int sampleRate, unsigned long frameCount)
    : sampleRate(sampleRate)
    , frameCount(frameCount)
{
    instance = this;

    nodeManager.loadAll("Objects");
}

PatchformApp::~PatchformApp()
{
}

bool PatchformApp::initialize()
{
    std::cout << "Patchform version: " << patchform_git_version  << " hash: " << patchform_git_hash << std::endl;

    settings.load();

    selectedApiIndex = settings.selectedApiIndex;
    selectedInputDeviceIndex = settings.selectedInputDeviceIndex;
    selectedOutputDeviceIndex = settings.selectedOutputDeviceIndex;

    if (!initMidi())
    {
        std::cerr << "Failed to initialize MIDI" << std::endl;
        // Not having MIDI is OK (but MIDI nodes won't work obviously!)
    }

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

void PatchformApp::shutdown()
{
    settings.selectedApiIndex = getSelectedApiIndex();
    settings.selectedInputDeviceIndex = getSelectedInputDeviceIndex();
    settings.selectedOutputDeviceIndex = getSelectedOutputDeviceIndex();

    settings.windowIsMaximized = window->isMaximized();
    window->getUserSize(settings.windowWidth, settings.windowHeight);

    graphSystem.closeAll();

    editor.reset();
    eventManager.reset();
    window.reset();

    shutdownAudio();

    // Clear node factories BEFORE unloading the DLLs
    NodeRegistry::getInstance().clearAll();
    nodeManager.unloadAll();

    if (invalidFB) {
        nanoVGDeleteFramebuffer(invalidFB);
        invalidFB = nullptr;
    }

    regularFont = semiBoldFont = iconFont = objectIconFont = -1;

    if (nvg) {
        destroyNanoVGContext(nvg);
        nvg = nullptr;
    }

    SDL_Quit();

    settings.save();
}

void PatchformApp::run()
{
    bool running = true;
    SDL_Event event;

    while (running)
    {
        // Check if audio device was disconnected
        static bool restarting = false;
        if (!restarting && (Pa_IsStreamStopped(stream) || !Pa_IsStreamActive(stream))) {
            restarting = true;
            std::cerr << "Audio stream stopped unexpectedly. Restarting..." << std::endl;
            reinitAudio();
            restarting = false;
        }

        Uint32 currentFrameTime = SDL_GetTicks();
        Uint32 elapsedTime = currentFrameTime - lastFrameTime;
        Uint32 waitTime = (elapsedTime < targetFrameTime) ? (targetFrameTime - elapsedTime) : 0;

        if (SDL_WaitEventTimeout(nullptr, waitTime))
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
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
                // Touch events go only to gesture layer
                case SDL_EVENT_FINGER_DOWN:
                    eventManager->handleFingerDown(event.tfinger);
                    break;
                case SDL_EVENT_FINGER_UP:
                    eventManager->handleFingerUp(event.tfinger);
                    break;
                case SDL_EVENT_FINGER_MOTION:
                    eventManager->handleFingerMotion(event.tfinger);
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    {
                        newWidth = event.window.data1;
                        newHeight = event.window.data2;

                        editor->setBounds(0, 0, newWidth, newHeight);
                        Uint32 flags = SDL_GetWindowFlags(window->getSDLWindow());
                        if ((flags & SDL_WINDOW_MAXIMIZED) == 0 && !window->getIsProgrammaticResize()) {
                            window->setUserSize(newWidth, newHeight); // user resize only
                        }
                    }
                    break;
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    window->setMaximized(true);
                    break;
                case SDL_EVENT_WINDOW_RESTORED:
                    window->setMaximized(false);
                    break;
                case SDL_EVENT_WINDOW_MOVED:
                    // If editor was moved from outside screen bound the framebuffer will not repaint
                    // the out of bounds region, so force a repaint when moved has finished
                    editor->repaint();
                    break;
                default:
                    break;
                }
            }
        }

        editor->updateObjectsFromDSP();
        editor->handleTime(currentFrameTime, std::min(elapsedTime, targetFrameTime));
        editor->updateFrameBuffers(nvg);

        if (!editor->needsRepaint())
        {
            SDL_Delay(1);
            continue;
        }

        //static int c = 0;
        //std::cout << c++ << " repainting" << std::endl;
        int drawableW, drawableH;
        SDL_GetWindowSizeInPixels(window->getSDLWindow(), &drawableW, &drawableH);

        if (!invalidFB || drawableW != windowWidth || drawableH != windowHeight) {
            windowWidth = drawableW;
            windowHeight = drawableH;

            if (invalidFB)
                nanoVGDeleteFramebuffer(invalidFB);

            invalidFB = nanoVGCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);
        }

        render();

        lastFrameTime = currentFrameTime;
    }
}

int PatchformApp::audioCallback(const void* input, void* output,
                                unsigned long frameCount,
                                const PaStreamCallbackTimeInfo*,
                                PaStreamCallbackFlags statusFlags,
                                void* userData)
{
    PlatformHelpers::disableDenormalsOncePerThread();

    auto* app = static_cast<PatchformApp*>(userData);

    if (app->shuttingDownAudio.load(std::memory_order_acquire))
        return paAbort;

    auto inputChannels = app->inputChannels.load(std::memory_order_relaxed);
    auto outputChannels = app->outputChannels.load(std::memory_order_relaxed);

    auto bypassMode = app->bypassMode.load(std::memory_order_relaxed);

    if (!bypassMode && outputChannels == 0)
        return paContinue;

    const float* const* in = reinterpret_cast<const float* const*>(input);
    float** out = static_cast<float**>(output);

    // Clear output
    if (out && outputChannels > 0)
    {
        for (int ch = 0; ch < outputChannels; ++ch)
        {
            if (out[ch])
                std::fill(out[ch], out[ch] + frameCount, 0.0f);
        }
    }

    // MIDI
    std::vector<MidiMessage> midiMessages;
    MidiMessage midiMsg;
    while (app->midiQueue.try_dequeue(midiMsg))
        midiMessages.push_back(midiMsg);

    thread_local static float bypassBuffer[2048] = {0};// hard coded to max buffer size TODO: Set max buffer size!

    float* outputChannel0 = (out && outputChannels > 0 && out[0] && !bypassMode) ? out[0] : bypassBuffer;
    const float* inputChannel0 = (in && inputChannels > 0 && in[0] && !bypassMode) ? in[0] : bypassBuffer;

    app->graphSystem.processAll(inputChannel0, outputChannel0, frameCount, midiMessages);

    if (statusFlags & (paOutputUnderflow | paInputOverflow))
        std::cerr << "Audio underflow or overflow detected" << std::endl;

    return paContinue;
}

bool PatchformApp::initAudio() {
    if (Pa_Initialize() != paNoError)
        return false;

    int numApis = Pa_GetHostApiCount();
    if (numApis <= 0)
        return false;

    int audioDriver = selectedApiIndex >= 0 ? selectedApiIndex : Pa_GetDefaultHostApi();
    std::cout << "Setting audio driver to: " << audioDriver << std::endl;
    const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(audioDriver);

    std::cout << "\n==== Audio Driver Info ====\n";
    for (int i = 0; i < numApis; ++i) {
        const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
        if (info)
            std::cout << (audioDriver == i ? "Active" : "Inactive") << " API " << i << " " << info->name << std::endl;
    }

    int inputDeviceIndex = selectedInputDeviceIndex;
    int outputDeviceIndex = selectedOutputDeviceIndex;

    const PaDeviceInfo* inputInfo = (inputDeviceIndex >= 0) ? Pa_GetDeviceInfo(inputDeviceIndex) : nullptr;
    const PaDeviceInfo* outputInfo = (outputDeviceIndex >= 0) ? Pa_GetDeviceInfo(outputDeviceIndex) : nullptr;

    if (!inputInfo && !outputInfo) {
        std::cout << "No input or output device selected.\n";
        return true; // Not an error, just no audio stream
    }

    PaStreamParameters inputParams{}, outputParams{};
    PaStreamParameters* inputParamsPtr = nullptr;
    PaStreamParameters* outputParamsPtr = nullptr;

    if (inputInfo) {
        std::cout << "Using input: " << inputInfo->name << "\n";
        inputParams.device = inputDeviceIndex;
        inputParams.channelCount = std::min(2, inputInfo->maxInputChannels);
        inputParams.sampleFormat = paFloat32 | paNonInterleaved;
        inputParams.suggestedLatency = inputInfo->defaultLowInputLatency;
        inputParamsPtr = &inputParams;
    }

    if (outputInfo) {
        std::cout << "Using output: " << outputInfo->name << "\n";
        outputParams.device = outputDeviceIndex;
        outputParams.channelCount = std::min(2, outputInfo->maxOutputChannels);
        outputParams.sampleFormat = paFloat32 | paNonInterleaved;
        outputParams.suggestedLatency = outputInfo->defaultLowOutputLatency;
        outputParamsPtr = &outputParams;

        bypassMode.store(false);
    }

    if (!outputParamsPtr) {
        // Open with 1 output channel to ensure callback runs, but set bypass mode to true,
        // which connects the callback in/out to a disconnected buffer
        outputParams.device = Pa_GetDefaultOutputDevice();
        const PaDeviceInfo* dummyInfo = Pa_GetDeviceInfo(outputParams.device);

        outputParams.channelCount = 1;
        outputParams.sampleFormat = paFloat32 | paNonInterleaved;
        outputParams.suggestedLatency = dummyInfo ? dummyInfo->defaultHighOutputLatency : 0.1;
        outputParamsPtr = &outputParams;

        // We don't actually use this — just force PortAudio to run callback
        bypassMode.store(true);
    }

    inputChannels.store(inputParamsPtr ? inputParams.channelCount : 0);
    outputChannels.store(outputParamsPtr ? outputParams.channelCount : 0);

    auto err = Pa_OpenStream(&stream, inputParamsPtr, outputParamsPtr, sampleRate, frameCount, 0, audioCallback, this);
    if (err != paNoError) {
        std::cerr << "Pa_OpenStream failed: " << Pa_GetErrorText(err) << "\n";
        std::cerr << "Falling back to default devices...\n";

        int defIn = Pa_GetDefaultInputDevice();
        int defOut = Pa_GetDefaultOutputDevice();

        const PaDeviceInfo* inDev = defIn != paNoDevice ? Pa_GetDeviceInfo(defIn) : nullptr;
        const PaDeviceInfo* outDev = defOut != paNoDevice ? Pa_GetDeviceInfo(defOut) : nullptr;

        PaStreamParameters inParams{}, outParams{};
        PaStreamParameters* inParamsPtr = nullptr;
        PaStreamParameters* outParamsPtr = nullptr;

        if (inDev && inDev->maxInputChannels > 0) {
            inParams.device = defIn;
            inParams.channelCount = std::min(2, inDev->maxInputChannels);
            inParams.sampleFormat = paFloat32 | paNonInterleaved;
            inParams.suggestedLatency = inDev->defaultLowInputLatency;
            inParams.hostApiSpecificStreamInfo = nullptr;
            inParamsPtr = &inParams;
        }

        if (outDev && outDev->maxOutputChannels > 0) {
            outParams.device = defOut;
            outParams.channelCount = std::min(2, outDev->maxOutputChannels);
            outParams.sampleFormat = paFloat32 | paNonInterleaved;
            outParams.suggestedLatency = outDev->defaultLowOutputLatency;
            outParams.hostApiSpecificStreamInfo = nullptr;
            outParamsPtr = &outParams;
        }

        err = Pa_OpenStream(&stream, inParamsPtr, outParamsPtr, sampleRate, frameCount, paClipOff, audioCallback, this);


        if (err != paNoError) {
            std::cerr << "Default Pa_OpenStream also failed: " << Pa_GetErrorText(err) << "\n";
            return false;
        }
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "Pa_StartStream failed: " << Pa_GetErrorText(err) << "\n";
        return false;
    }

    return true;
}

void PatchformApp::reinitAudio() {
    // FULL teardown
    shutdownAudio();

    // Brief delay to let drivers release
    SDL_Delay(100);

    if (Pa_Initialize() != paNoError) {
        std::cerr << "Failed to reinitialize PortAudio\n";
        return;
    }

    initAudio();
}

void PatchformApp::shutdownAudio() {
    shuttingDownAudio.store(true, std::memory_order_release);
    inputChannels.store(0, std::memory_order_relaxed);
    outputChannels.store(0, std::memory_order_relaxed);
    bypassMode.store(false, std::memory_order_relaxed);

    if (stream) {
        Pa_AbortStream(stream);  // force exit if Stop hangs
        Pa_CloseStream(stream);
        stream = nullptr;
    }

    Pa_Terminate();
    shuttingDownAudio.store(false, std::memory_order_release);
}

std::vector<std::string> PatchformApp::getAvailableAudioApis() {
    std::vector<std::string> apis;
    int count = Pa_GetHostApiCount();
    for (int i = 0; i < count; ++i) {
        if (const auto* info = Pa_GetHostApiInfo(i))
            apis.push_back(info->name);
    }
    return apis;
}

std::vector<std::string> PatchformApp::getAvailableDevices(int apiIndex) {
    std::vector<std::string> devices;
    int count = Pa_GetDeviceCount();
    for (int i = 0; i < count; ++i) {
        const PaDeviceInfo* dev = Pa_GetDeviceInfo(i);
        if (dev && Pa_GetDeviceInfo(i)->hostApi == apiIndex)
            devices.push_back(dev->name);
    }
    return devices;
}

bool PatchformApp::setAudioDriver(int apiIndex) {
    selectedApiIndex = apiIndex;
    selectedDeviceIndex = -1; // reset device
    return reinitAudio(), true;
}

bool PatchformApp::initMidi()
{
    try {
        // Create the RtMidiIn instance
        midiIn = std::make_unique<RtMidiIn>();

        int nPorts = midiIn->getPortCount();
        if (nPorts == 0) {
            std::cerr << "No MIDI input ports available." << std::endl;
            // Depending on your design, you might return false or continue
            return true;
        }

        // Set the callback. The lambda captures no variables and uses the provided userData.
        midiIn->setCallback([](double timeStamp, std::vector<unsigned char>* message, void* userData) {
            // Cast userData back to PatchformApp
            auto* app = static_cast<PatchformApp*>(userData);
            if (message && !message->empty()) {
                MidiMessage midiMsg;
                midiMsg.timestamp = timeStamp;
                midiMsg.message = *message;
                // Push the message into the lock-free queue
                app->midiQueue.enqueue(std::move(midiMsg));
            }
        }, this);

        // Open first port
        int portIndex = std::min(nPorts - 1, 100);
        midiIn->openPort(portIndex);

        // Optionally, set the types of MIDI messages to ignore:
        midiIn->ignoreTypes(true, true, true);

        // Get API name
        RtMidi::Api api = midiIn->getCurrentApi();
        std::string apiName = RtMidi::getApiDisplayName(api);

        // Get port name
        std::string portName = midiIn->getPortName(portIndex);

        std::cout << "\n==== Midi Driver Info ====" << std::endl;

        std::cout << "MIDI initialized (" << nPorts << " port(s) found)\n"
                  << "Using API: " << apiName << "\n"
                  << "Opened port: " << portName << std::endl;
    }
    catch (RtMidiError &error) {
        error.printMessage();
        return false;
    }
    return true;
}

void PatchformApp::shutdownMidi()
{
    if (midiIn) {
        // Close the MIDI port if open
        midiIn->closePort();
        // Release the object
        midiIn.reset();
    }
}

bool PatchformApp::initUI()
{
    newWidth = windowWidth = settings.windowWidth;
    newHeight = windowHeight = settings.windowHeight;
    isFullscreen = settings.windowIsMaximized;

    if (newWidth == -1 || newHeight == -1)
    {
        newWidth = windowWidth = 1000;
        newHeight = windowHeight = 700;
    }

    window = std::make_unique<WindowPeer>("Patchform", windowWidth, windowHeight, isFullscreen);
    if (!window) return false;

    nvg = createNanoVGContext(0);
    if (!nvg) return false;

    if (!loadFonts()) {
        std::cerr << "Failed to load fonts!" << std::endl;
        return false;
    }

    editor = std::make_unique<Editor>(window.get());

    std::vector<std::string> fonts = { "Regular", "SemiBold", "icons", "object_icons" };

    editor->cacheFontMetrics(nvg, fonts, { 14.0f, 16.0f, 100.0f });

    editor->init(&graphSystem);

    eventManager = std::make_unique<pptk::EventManager>(editor.get());

    editor->setBounds(0, 0, windowWidth, windowHeight);
    SDL_GetWindowSizeInPixels(window->getSDLWindow(), &windowWidth, &windowHeight);
    invalidFB = nanoVGCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);

    return true;
}

void PatchformApp::render()
{
    int drawableW, drawableH;
    int windowW, windowH;
    SDL_GetWindowSize(window->getSDLWindow(), &windowW, &windowH);
    SDL_GetWindowSizeInPixels(window->getSDLWindow(), &drawableW, &drawableH);
    float pixelRatio = (float)drawableW / (float)windowW;

    nanoVGBindFramebuffer(invalidFB);

    nvgViewport(0, 0, drawableW, drawableH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(nvg, windowW, windowH, pixelRatio);  // correct scaling
    editor->renderFrame(nvg);
    nvgGlobalScissor(nvg, 0, 0, drawableW, drawableH);
    nvgEndFrame(nvg);

    nanoVGBindFramebuffer(nullptr);
    nanoVGBlitFramebuffer(nvg, invalidFB, 0, 0, drawableW, drawableH);

    window->swapBuffers();
}

bool PatchformApp::loadFonts()
{
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

