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

#include "Utility/ppl_string.hpp"

#include "external/linenoise-ng/include/linenoise.h"

#include "Graph/AudioGraph.h"

// PortAudio Callback
static int audioCallback(const void* input, void* output,
                         unsigned long frameCount,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void* userData) {
    auto* graphs = static_cast<GraphManager*>(userData);
    float* out = (float*)output;

    std::fill(out, out+frameCount, 0.0);

    graphs->process(out, frameCount);  // Process the audio graph

    if ((statusFlags & paOutputUnderflow) || (statusFlags & paInputOverflow)) {
        std::cout << "under of over flow" << std::endl;
    }

    return paContinue;
}

std::atomic<bool> running(true); // Flag to control the loop

void repl(GraphManager& graphs) {
    while (running) {
        // Display REPL prompt
        auto rawLine = linenoise("\x1b[1;32mPlugPatch\x1b[0m>> ");
        auto line = ppl::string(rawLine);
        if (line.isEmpty()) {
            continue; // Skip if no input
        }

        // Tokenize input
        auto tokens = line.tokenize(" ");

        // Handle empty input
        if (tokens.empty()) {
            free(rawLine);
            continue;
        }

        const auto& command = tokens[0]; // First token is the command

        switch (hash(command))
        {
        case hash("exit"):
        case hash("quit"):
        case hash("q"):
            {
                std::cout << "Exiting..." << std::endl;
                running = false;
            }
            break;
        case hash("add"):
            if (tokens.size() > 1) {
                auto addType = tokens[1].str();
                switch (hash(addType))
                {
                    case hash("con"):
                    case hash("conn"):
                    case hash("connection"):
                    if (tokens.size() == 6) {
                        graphs.connect(tokens[2].str(), stoi(tokens[3].str()), tokens[4].str(), stoi(tokens[5].str()));
                    } else
                    {
                        std::cout << "Error: needs: <outObj> <outPort> <inObj> <inPort>" << std::endl;
                    }
                        break;
                    default:
                        std::cout << "Error: unknown command: " << addType << std::endl;
                }
            }
            break;
        case hash("rem"):
        case hash("del"):
        case hash("delete"):
        case hash("remove"):
            if (tokens.size() > 1) {
                auto addType = tokens[1].str();
                switch (hash(addType))
                {
                case hash("con"):
                case hash("connection"):
                if (tokens.size() == 6) {
                    graphs.disconnect(tokens[2].str(), stoi(tokens[3].str()), tokens[4].str(), stoi(tokens[5].str()));
                } else
                {
                    std::cout << "Error: needs: <outObj> <outPort> <inObj> <inPort>" << std::endl;
                }
                    break;
                }
            }
            break;
        case hash("load"):
            if (tokens.size() > 1) {
                // Command starts with "load" and has a filename
                auto filename = tokens[1];
                std::cout << "Loading graph from file: " << filename << "..." << std::endl;

                // Handle file loading
                char buffer[MAX_PATH];
                DWORD length = GetCurrentDirectoryA(MAX_PATH, buffer);
                if (length == 0) {
                    std::cerr << "Error getting current directory." << std::endl;
                    break;
                }

                auto fullPath = filename + ".json";
                std::ifstream file(fullPath.str());

                if (!file.is_open()) {
                    // Try loading JSON5 file
                    fullPath = filename + ".json5";
                    file.open(fullPath.str());
                    if (!file.is_open())
                    {
                        std::cerr << "Could not open file: " << fullPath << std::endl;
                        break;
                    }
                }

                std::cout << fullPath << " loaded successfully" << std::endl;

                std::string fileContent((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
                file.close();

                try {
                    nlohmann::json patch = nlohmann::json::parse(fileContent, nullptr, true, true);
                    if (!patch.empty()) {
                        bool logVerbose = tokens.size() > 2 && (tokens[2] == "v" || tokens[2] == "verbose");
                        graphs.setActiveGraph(patch, logVerbose);
                    }
                }
                catch (const nlohmann::json::parse_error& ex)
                {
                    std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
                }
            }
            break;
        case hash("list"):
             if (tokens.size() > 1) {
                switch (hash(tokens[1]))
                {
                case hash("graph"):
                    graphs.printGraph();
                    break;
                case hash("obj"):
                case hash("objects"):
                    for (const auto& name : NodeRegistry::getInstance().getNodeNames()) {
                        std::cout << "- " << name << std::endl;
                    }
                    break;
                case hash("con"):
                case hash("conn"):
                case hash("connections"):
                    graphs.printAdjacencyList();
                    break;
                default:
                    std::cout << "Unknown list action: " << tokens[1] << std::endl;
                }
            }
            break;
        case hash("h"):
        case hash("help"):
            {
                constexpr std::string_view helpText = R"(
PlugPatch is an audio environment that uses JSON file format to describe an audio graph of nodes and connections.

Commands:
[quit]          Exit the application.
                Aliases: [q], [exit]

[load]          Load a graph file. Example: "load graph"
                Options:
                [verbose]    Print the connection layout.
                             Alias: [v]

[list]          List the currently loaded graph.
                Options:
                [connection] Print the connection layout.
                             Alias: [conn]
                [nodes]      Print available nodes that can be added.

[add]           Add to the currently loaded patch:
                Options:
                [connection] Add a connection <outObj> <outPort> <inObj> <inPort>. Example: "add con 0 1 1 0"
                             Alias: [conn] [con]

[about]         Print credits / OSS libraries

[help]          Print this help text
                Alias: [h]
                )";

                std::cout << helpText << std::endl;
            }
            break;
        case hash("about"):
            {
                constexpr std::array<std::string_view, 5> credits = {{
                    R"(linenoise-ng (CLI REPL)
    Martijn van Steenbergen
    BSD-3-Clause License
    https://github.com/arangodb/linenoise-ng)",

                    R"(moodycamel ConcurrentQueue (Lockfree queue)
    Cameron Desrochers
    Simplified BSD License
    https://github.com/cameron314/concurrentqueue)",

                    R"(nlohmann/json (JSON file parsing)
    Niels Lohmann
    MIT License
    https://github.com/nlohmann/json)",

                    R"(PortAudio (CLI Audio I/O)
    PortAudio Team
    MIT License
    https://github.com/PortAudio/portaudio)",

                    R"(unordered_dense (Replacement for std::unordered_map)
    Martin Ankerl
    MIT License
    https://github.com/martinus/unordered_dense)"
                }};

                // Copy to a runtime array and sort by the first letter of each string
                auto sortedCredits = credits;
                std::sort(sortedCredits.begin(), sortedCredits.end(), [](std::string_view a, std::string_view b) {
                    char firstA = std::tolower(static_cast<unsigned char>(a[0]));
                    char firstB = std::tolower(static_cast<unsigned char>(b[0]));
                    return firstA < firstB;
                });

                std::cout << "Credits in alphabetical order:" << "\n\n";

                // Print each entry
                for (const auto& credit : sortedCredits) {
                    std::cout << credit << "\n\n";
                }
            }
            break;
        default:
            std::cout << "Invalid command" << std::endl;
        }

        // Add to command history and free memory
        linenoiseHistoryAdd(rawLine);
        free(rawLine);
    }
}

int main() {
    PaError err;
    unsigned long frameCount = 64;
    float sampleRate = 44100.0f;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio initialization failed: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    auto context = std::make_unique<NodeContext>(sampleRate, frameCount);

    GraphManager graphs(context.get());

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
