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
#include "json.hpp"
using json = nlohmann::json;

#include "external/linenoise-ng/include/linenoise.h"

#include "Graph/AudioGraph.h"

std::vector<std::string> tokenize(const std::string& input) {
    std::istringstream stream(input);
    std::vector<std::string> tokens;
    std::string token;

    // Read words, skipping extra spaces
    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

// PortAudio Callback
static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graphs = static_cast<Graphs*>(userData);
    float* out = (float*)output;

    std::fill(out, out+frameCount, 0.0);

    graphs->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cout << "under of over flow" << std::endl;
    }

    return paContinue;
}

std::atomic<bool> running(true); // Flag to control the loop

void repl(Graphs& graphs) {
    while (running) {
        // Display REPL prompt
        char* line = linenoise("\x1b[1;32mPlugPatch\x1b[0m>> ");
        if (line == nullptr) {
            continue; // Skip if no input
        }

        // Tokenize input
        auto tokens = tokenize(line);

        // Handle empty input
        if (tokens.empty()) {
            free(line);
            continue;
        }

        const std::string& command = tokens[0]; // First token is the command

        if (command == "exit" || command == "quit" || command == "q") {
            std::cout << "Exiting..." << std::endl;
            running = false;
            break;
        } else if (command == "load" && tokens.size() > 1) {
            // Command starts with "load" and has a filename
            std::string filename = tokens[1];
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
                if (!patch.empty()) {
                    bool logVerbose = tokens.size() > 2 && (tokens[2] == "-v" || tokens[2] == "-verbose");
                    graphs.setActiveGraph(patch, logVerbose);
                }
            } catch (const nlohmann::json::parse_error& ex) {
                std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
            }
        } else if (command == "list" && tokens.size() > 1) {
            if (tokens[1] == "nodes") {
                for (const auto& name : NodeRegistry::getInstance().getNodeNames()) {
                    std::cout << "- " << name << std::endl;
                }
            } else {
                std::cout << "Unknown list command!" << std::endl;
            }
        } else if (command == "h" || command == "help") {
            std::string text =
                "\n"
                "PlugPatch is an audio environment that uses JSON file format to describe an audio graph of nodes and connections.\n"
                "\n"
                "Commands:\n"
                "exit, quit, q   Exit the application.\n"
                "load            Load a graph file. Example: load graph\n"
                "load -v         Print the adjacency list\n"
                "list            List the currently loaded graph.\n"
                "list sort       List the currently loaded sorted graph.\n"
                "list nodes      List available nodes that can be added.\n"
                "add             Add a node to the graph. Example: add metro\n"
                "connect         Connect nodes together. Example: connect 0.0 1.0\n"
                "credits         List credits / OSS libraries\n";

            std::cout << text << std::endl;
        } else {
            std::cout << "Invalid command!" << std::endl;
        }

        // Add to command history and free memory
        linenoiseHistoryAdd(line);
        free(line);
    }
}

int main() {
    PaError err;
    unsigned long frameCount = 1024;
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
        std::string text = R"(    ____  __            ____        __       __
   / __ \/ /_  ______ _/ __ \____ _/ /______/ /_
  / /_/ / / / / / __ `/ /_/ / __ `/ __/ ___/ __ \
 / ____/ / /_/ / /_/ / ____/ /_/ / /_/ /__/ / / /
/_/   /_/\__,_/\__, /_/    \__,_/\__/\___/_/ /_/
              /____/
)";
        std::cout << text << std::endl;
        std::cout << "(c) 2025 Alexander Mitchell" << std::endl;
        std::cout << std::endl;
        std::cout << "Sample rate: " << streamInfo->sampleRate << std::endl;
        std::cout << "Buffer size: " << frameCount << std::endl;
        std::cout << "Input latency: " << streamInfo->inputLatency << " Output latency: " << streamInfo->outputLatency << std::endl;
        std::cout << std::endl;
        std::cout << "type \"h\" or \"help\" for help" << std::endl;
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
