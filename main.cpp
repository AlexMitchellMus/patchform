/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include <functional>
#include <fstream>
#include <windows.h>
#include <thread>
#include <atomic>
#include <conio.h>

#include <PortAudio.h>
#include "external/json/single_include/nlohmann/json.hpp"
using json = nlohmann::json;

#include "external/linenoise-ng/include/linenoise.h"

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

// Function to process commands
void repl(Graphs& graphs) {
    while (running) {
        // Display REPL prompt
        char* line = linenoise(">> ");
        if (line == nullptr) {
            continue; // Skip if no input
        }

        std::string input(line);
        linenoiseHistoryAdd(line); // Add command to history
        linenoiseHistoryFree(); // Free memory allocated by linenoise

        if (input == "exit" || input == "quit") {
            std::cout << "Exiting..." << std::endl;
            running = false;
            break;
        } else if (input.rfind("load ", 0) == 0) { // Command starts with "load "
            std::string filename = input.substr(5); // Get file name
            std::cout << "Loading graph from file: " << filename << "..." << std::endl;

            // Handle file loading
            char buffer[MAX_PATH];
            DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
            if (length == 0) {
                std::cerr << "Error getting current directory." << std::endl;
                continue;
            }

            std::string fullPath = filename + ".json";
            std::ifstream file(fullPath);

            if (!file.is_open()) {
                // Try loading JSON5 file
                fullPath = filename + ".json5";
                file.open(fullPath);
                if (!file.is_open()) {
                    std::cerr << "Could not open file: " << fullPath << std::endl;
                    continue;
                }
            }

            std::cout << fullPath << " loaded successfully!" << std::endl;

            std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();

            try {
                nlohmann::json patch = nlohmann::json::parse(fileContent, nullptr, false, true);
                graphs.setActiveGraph(patch);
            } catch (const nlohmann::json::parse_error& ex) {
                std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
            }
        } else {
            std::cout << "Invalid command!" << std::endl;
        }
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

    repl(graphs);

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
