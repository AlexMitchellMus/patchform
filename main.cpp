/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <stack>
#include <functional>
#include <fstream>
#include <windows.h>
#include <thread>
#include <atomic>
#include <conio.h>

#include <PortAudio.h>
#include "external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include "Graph/AudioGraph.h"

// PortAudio Callback
static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graphs = static_cast<Graphs*>(userData);
    float* out = (float*)output;

    graphs->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cout << "under of over flow" << std::endl;
    }

    return paContinue;
}

std::atomic<bool> running(true); // Flag to control the loop

// Function to handle user input for commands and Escape key detection
void commandListener(std::function<void(std::string& patchToLoad)> callback) {
    std::string input;
    std::cout << "Press Escape to close app, type \"load file\" to load graph" << std::endl;
    while (running) {
        // Check if Escape key (VK_ESCAPE) is pressed
        if (GetAsyncKeyState(VK_ESCAPE)) {
            std::cout << "Escape key pressed. Exiting..." << std::endl;
            running = false;
            break;
        }

        // Non-blocking check for keyboard input
        if (_kbhit()) {
            char ch = _getch();
            if (ch == '\r') {
                if (input.rfind("load ", 0) == 0) { // Check if the command starts with "load "
                    std::string filename = input.substr(5); // Get the file name after "load "
                    std::cout << "\nLoading graph from file: " << filename << "..." << std::endl;
                    callback(filename);
                    Sleep(500);
                } else {
                    std::cout << "\nInvalid command!" << std::endl;
                }
                input.clear();
            } else {
                input += ch;
                std::cout << ch;
            }
        }

        Sleep(10);
    }
}
int main() {
    PaError err;
    unsigned long frameCount = 512;
    float sampleRate = 44100.0f;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto context = std::make_unique<NodeContext>(sampleRate, frameCount);

    Graphs graphs(context.get());

    // Set up PortAudio stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, sampleRate, frameCount, audioCallback, &graphs);
    if (err != paNoError) {
        std::cerr << "PortAudio stream setup failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // Start stream
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream start failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto streamInfo = Pa_GetStreamInfo(stream);
    if (streamInfo != nullptr) {
        std::cout << "Sample Rate: " << streamInfo->sampleRate << std::endl;
        std::cout << "input latency: " << streamInfo->inputLatency << " output latency: " << streamInfo->outputLatency << std::endl;
    }

    auto callback = [&graphs](std::string& patchToLoad) {
        char buffer[MAX_PATH];
        DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
        if (length == 0) {
            std::cerr << "Error getting current directory." << std::endl;
        } else {
            std::cout << "Current working directory: " << buffer << std::endl;
        }

        std::string filename = patchToLoad + ".json";
        std::ifstream file(filename);

        if (!file.is_open()) {
            // First try json5 alternative
            filename = patchToLoad + ".json5";
            file.open(filename);
            if (!file.is_open()) {
                std::cerr << "Could not open the file!" << std::endl;
                return;
            }
        }

        std::cout << filename << " loaded successfully!" << std::endl;

        std::string input((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        // Parse the cleaned JSON string
        try {
            nlohmann::json patch = nlohmann::json::parse(input, nullptr, false, true); // Allow comments in JSONlo
            graphs.setActiveGraph(patch);
        } catch (const nlohmann::json::parse_error& ex) {
            std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
        }
    };

    // Start a thread for command input and pass a callback using std::bind
    std::thread commandThread(std::bind(commandListener, callback));

    // Wait for the command thread to finish
    if (commandThread.joinable()) {
        commandThread.join();
    }

    // Stop and clean up
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream stop failed: " << Pa_GetErrorText(err) << std::endl;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio stream close failed: " << Pa_GetErrorText(err) << std::endl;
    }

    Pa_Terminate();

    return 0;
}
