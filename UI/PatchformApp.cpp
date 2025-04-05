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
#include "../UI_ToolKit/WindowPeer.h"

PatchformApp::PatchformApp(int sampleRate, unsigned long frameCount)
    : graphManager(sampleRate, frameCount)
    , sampleRate(sampleRate)
    , frameCount(frameCount)
    ,windowWidth(1920)
    , windowHeight(1080)
{

}

PatchformApp::~PatchformApp()
{
    shutdown();
}

bool PatchformApp::initialize()
{
    if (!initMidi())
    {
        std::cerr << "Failed to initialize MIDI" << std::endl;
        return false;
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

    if (window) {
        window.reset();
    }

    SDL_Quit();
}

void PatchformApp::run()
{
    bool running = true;
    SDL_Event event;

    while (running)
    {
        // Check if audio device was disconnected
        if (Pa_IsStreamStopped(stream) || !Pa_IsStreamActive(stream))
        {
            std::cerr << "Audio stream stopped unexpectedly. Restarting..." << std::endl;
            reinitializeAudio();
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
                case SDL_EVENT_WINDOW_RESIZED:
                    newWidth = event.window.data1;
                    newHeight = event.window.data2;
                    editor->setBounds(0, 0, newWidth, newHeight);
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

        if (!invalidFB || newWidth != windowWidth || newHeight != windowHeight)
        {
            windowWidth = newWidth;
            windowHeight = newHeight;

            if (invalidFB)
            {
                nvgDeleteFramebuffer(invalidFB);
                invalidFB = nullptr;
            }

            invalidFB = nvgCreateFramebuffer(nvg, windowWidth, windowHeight, NVG_IMAGE_PREMULTIPLIED);
        }

        render();

        lastFrameTime = currentFrameTime;
    }
}

void PatchformApp::reinitializeAudio()
{
    shutdownAudio();
    initAudio();
}

int PatchformApp::audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData)
{
    PlatformHelpers::disableDenormalsOncePerThread();

    if (!output) {
        return paAbort; // Prevent crashing if output buffer is invalid
    }

    auto* app = static_cast<PatchformApp*>(userData);

    auto* in = static_cast<const float*>(input);
    auto* out = static_cast<float*>(output);

    std::fill(out, out + frameCount, 0.0f);

    // Process any pending MIDI messages from the queue
    MidiMessage midiMsg;
    std::vector<MidiMessage> midiMessages;
    while (app->midiQueue.try_dequeue(midiMsg)) {
        // Pass the MIDI data and timestamp to your graph manager
        midiMessages.push_back(midiMsg);
    }

    app->graphManager.process(in, out, frameCount, midiMessages);  // Process the audio graph

    if (statusFlags & (paOutputUnderflow | paInputOverflow)) {
        std::cerr << "Audio underflow or overflow detected" << std::endl;
        //return paAbort; // Force PortAudio to restart stream
    }

    return paContinue;
}

bool PatchformApp::initAudio() {
    if (Pa_Initialize() != paNoError)
        return false;

    int numApis = Pa_GetHostApiCount();
    if (numApis < 0)
        return numApis; // error

    int audioDriver = std::min(numApis - 1, 3);

    std::cout << "\n==== Audio Driver Info ====" << std::endl;

    for (int i = 0; i < numApis; ++i) {
        const PaHostApiInfo* info = Pa_GetHostApiInfo(i);
        if (info)
            std::cout << (audioDriver == i ? "Active" : "Inactive") << " API " << i << " " << info->name << std::endl;
    }

    int inputDeviceIndex = Pa_GetHostApiInfo(audioDriver)->defaultInputDevice;
    int outputDeviceIndex = Pa_GetHostApiInfo(audioDriver)->defaultOutputDevice;

    if (inputDeviceIndex == paNoDevice || outputDeviceIndex == paNoDevice)
        return false;

    const PaDeviceInfo* inputInfo = Pa_GetDeviceInfo(inputDeviceIndex);
    const PaDeviceInfo* outputInfo = Pa_GetDeviceInfo(outputDeviceIndex);

    PaStreamParameters inputParams{};
    inputParams.device = inputDeviceIndex;
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency = inputInfo->defaultLowInputLatency;

    PaStreamParameters outputParams{};
    outputParams.device = outputDeviceIndex;
    outputParams.channelCount = 1;
    outputParams.sampleFormat = paFloat32;
    outputParams.suggestedLatency = outputInfo->defaultLowOutputLatency;

    if (Pa_OpenStream(&stream, &inputParams, &outputParams, sampleRate, frameCount, paClipOff, audioCallback, this) != paNoError)
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

bool PatchformApp::initUI() {
    window = std::make_unique<WindowPeer>("Patchform", windowWidth, windowHeight);
    if (!window) return false;

    nvg = nvgCreateContext(0);
    if (!nvg) return false;

    if (!loadFonts()) {
        std::cerr << "Failed to load fonts!" << std::endl;
        return false;
    }

    editor = std::make_unique<Editor>(window.get());

    std::vector<std::string> fonts = { "Regular", "SemiBold", "icons", "object_icons" };

    editor->cacheFontMetrics(nvg, fonts, { 14.0f, 16.0f, 100.0f });

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

    window->swapBuffers();
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

