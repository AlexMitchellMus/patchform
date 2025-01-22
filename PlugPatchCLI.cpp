/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <iostream>
#include <functional>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <conio.h>

#include "PortAudio.h"
#include "json.hpp"
using json = nlohmann::json;

#include "Utility/ppl_string.hpp"

#include "external/linenoise-ng/include/linenoise.h"

#include "Graph/AudioGraph.h"

json toJsonValue(const std::string& s)
{
    // 1) Try integer
    try {
        std::size_t pos = 0;
        long intVal = std::stol(s, &pos, 10);  // base 10
        // Only accept if we consumed the entire string
        if (pos == s.size()) {
            return intVal;
        }
    }
    catch (const std::invalid_argument&) {
        // Could not parse as integer
    }
    catch (const std::out_of_range&) {
        // Number outside long range
    }

    // 2) Try floating-point
    try {
        std::size_t pos = 0;
        double doubleVal = std::stod(s, &pos);
        // Only accept if we consumed the entire string
        if (pos == s.size()) {
            return doubleVal;
        }
    }
    catch (const std::invalid_argument&) {
        // Could not parse as double
    }
    catch (const std::out_of_range&) {
        // Number outside double range
    }

    // 3) Fallback: store as string
    return s;
}

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
        std::cerr << "Audio under of over flow" << std::endl;
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
        case hash("quit"):
        case hash("q"):
            {
                std::cout << "Exiting..." << std::endl;
                running = false;
            }
            break;
        case hash("a"):
        case hash("add"):
            if (tokens.size() > 1)
            {
                auto addType = tokens[1].str();
                switch (hash(addType))
                {
                case hash("o"):
                case hash("obj"):
                    if (tokens.size() > 1)
                    {
                        json j;
                        j["obj"] = tokens[2].str();
                        for (size_t i = 3; i + 1 < tokens.size(); i += 2)
                        {
                            const std::string& tag   = tokens[i].str();

                            // Convert "value" to an integer/double if possible
                            j[tag] = toJsonValue(tokens[i + 1].str());
                        }
                        std::cout << j << std::endl;
                        graphs.addObject(j);
                    }
                    break;
                case hash("c"):
                case hash("connection"):
                    if (tokens.size() == 6)
                    {
                        graphs.connect(tokens[2].str(), stoi(tokens[3].str()), tokens[4].str(), stoi(tokens[5].str()));
                    }
                    else
                    {
                        std::cout << "Error: needs: <outObj> <outPort> <inObj> <inPort>" << std::endl;
                    }
                    break;
                default:
                    std::cout << "Error: unknown command: " << addType << std::endl;
                }
            }
            break;
        case hash("rm"):
        case hash("remove"):
            if (tokens.size() > 1) {
                auto addType = tokens[1].str();
                switch (hash(addType))
                {
                case hash("o"):
                case hash("obj"):
                    if (tokens.size() == 3)
                    {
                        int idToRemove;
                        try {
                            idToRemove = stoi(tokens[2].str());
                            graphs.removeObject(idToRemove);
                        } catch (...) {
                            std::cerr << "Error: unknown command: " << tokens[2].str() << std::endl;
                        }
                    }
                    break;
                case hash("c"):
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
                std::filesystem::path fullPath = std::filesystem::current_path() / (filename.str() + ".json");
                std::ifstream file(fullPath);

                if (!file.is_open()) {
                    // Try loading JSON5 file
                    fullPath = std::filesystem::current_path() / (filename.str() + ".json5");
                    file.open(fullPath);
                    if (!file.is_open())
                    {
                        std::cerr << "Could not open file: " << fullPath << std::endl;
                        break;
                    }
                }

                std::cout << fullPath << " loaded successfully" << std::endl;

                std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                file.close();

                try {
                    nlohmann::json patch = nlohmann::json::parse(fileContent, nullptr, true, true);
                    if (!patch.empty()) {
                        bool logVerbose = tokens.size() > 2 && (tokens[2] == "v" || tokens[2] == "verbose");
                        auto filePath = std::filesystem::absolute(fullPath).string();
                        graphs.setActiveGraph(filePath, patch, logVerbose);
                    }
                }
                catch (const nlohmann::json::parse_error& ex)
                {
                    std::cerr << "Parse error in JSON file: " << ex.what() << std::endl;
                }
            }
            break;
        case hash("saveas"):
            if (tokens.size() > 1) {
                auto filePath = tokens[1].str() + ".json";
                auto jsonOutput = graphs.graphToJSON();

                try {
                    // Open a file stream
                    std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);

                    if (!outputFile.is_open()) {
                        throw std::ios_base::failure("Failed to open the file for writing.");
                    }

                    // Write the JSON to the file with pretty formatting
                    outputFile << jsonOutput.dump(4);
                    outputFile.close();

                    std::cout << "Graph successfully saved to " << std::filesystem::absolute(filePath).string() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error saving graph: " << e.what() << std::endl;
                }

                if (tokens.size() > 2 && tokens[2] == "v") {
                    std::cout << graphs.graphToJSON().dump(4) << std::endl;
                }
            }

            break;
        case hash("save"):
            if (auto filePath = graphs.getPatchFile(); !filePath.empty()) {
                auto jsonOutput = graphs.graphToJSON();

                try {
                    // Open a file stream
                    std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);

                    if (!outputFile.is_open()) {
                        throw std::ios_base::failure("Failed to open the file for writing.");
                    }

                    // Write the JSON to the file with pretty formatting
                    outputFile << jsonOutput.dump(4);
                    outputFile.close();

                    std::cout << "Graph successfully saved to " << std::filesystem::absolute(filePath).string() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error saving graph: " << e.what() << std::endl;
                }

                if (tokens.size() > 2 && tokens[2] == "v") {
                    std::cout << graphs.graphToJSON().dump(4) << std::endl;
                }
            }

            break;
        case hash("ls"):
        case hash("list"):
             if (tokens.size() > 1) {
                switch (hash(tokens[1]))
                {
                case hash("p"):
                case hash("patch"):
                    {
                        if (auto file = graphs.getPatchFile(); !file.empty())
                            std::cout << file << std::endl;
                        else
                            std::cerr << "No patch file loaded" << std::endl;
                    }
                    break;
                case hash("g"):
                case hash("graph"):
                    graphs.printGraph();
                    break;
                case hash("o"):
                case hash("object"):
                    for (const auto& name : NodeRegistry::getInstance().getNodeNames()) {
                        std::cout << "- " << name << std::endl;
                    }
                    break;
                case hash("c"):
                case hash("connection"):
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
PlugPatch is live audio environment, that allows the user to create an audio graph, with sample accurate events.

Commands:
[quit]       Exit the application.
             Alias: [q]

[load]       Load a graph file. Example: "load graph"
             Options:
             [verbose]       Print the connection layout.
                             Alias: [v]

[save]       Save a graph as a file. Example: "save <filename>"

[clear]      Clear the active graph

[list]       Alias: [ls]
             Options:
             [patch]         Print directly of loaded patch (if there is one)
                             Alias: [p]
             [graph]         Print the currently loaded graph objects
                             Alias: [g]
             [connection]    Print the connection layout.
                             Alias: [c]
             [obj]           Print available objects available to be added.
                             Alias: [o]

[add]        Add to the currently loaded patch:
             Alias: [a]
             Options:
             [obj]           Add an object. Object <name> and <key><value> pairs
                             Note: Missing <key><value> pairs will be init per object defaults
                             Example: "add obj osc" (default osc: freq 440, waveform sine)
                             Example: "add obj osc waveform tri"
                             Alias: [o]
             [connection]    Add a connection <outObj> <outPort> <inObj> <inPort>. Example: "add con 0 1 1 0"
                             Alias: [c]

[remove]     Remove from the currently loaded patch:
             Alias: [rm]
             Options:
             [obj]       Add an object. After object is key:value pairs
                             Example: "add obj osc"
                             Example: "add obj osc waveform tri"
                             Alias: [o]
             [connection]    Remove a connection <outObj> <outPort> <inObj> <inPort>.
                             Example: "remove connection 0 1 1 0"
                             Alias: [c]

[about]      Print credits / OSS libraries

[help]       Print this help text
             Alias: [h]
                )";

                std::cout << helpText << std::endl;
            }
            break;
        case hash("about"):
            {
                constexpr std::array<std::string_view, 6> credits = {{
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
    https://github.com/martinus/unordered_dense)",

                    R"(glaze (Extremely fast, in-memory, JSON and interface library for modern C++)
    Stephen Berry
    MIT License
    https://github.com/stephenberry/glaze)",
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
