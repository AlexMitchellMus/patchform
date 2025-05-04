//
// Created by alexw on 27/02/2025.
//

#pragma once

#include "../UI_Toolkit/ComponentViewport.h"
#include "../GitInfo.h"

class LibraryList : public pptk::Component
{
public:
    LibraryList()
    {
    }

    int getLibraryHeight()
    {
        int height = 20;
        for (auto lib : libraries)
        {
            const int numLines = std::ranges::count(lib, '\n') + 1;
            height += numLines * 20 + 30; // match the +30 yPos bump after drawing
        }
        return height;
    }

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // text colour
        nvgFontSize(nvg, 14.0f);
        nvgFontFace(nvg, "Regular");

        int yPos = 20.0f;

        for (auto lib : libraries) {
            drawLibraryEntry(nvg, getWidth() * 0.5f, yPos, lib);
            yPos += 30;  // move down for the next entry
        }
    }
private:
    void drawLibraryEntry(NVGcontext* nvg, float x, int& y, const std::string_view entry) {
        auto tokenize = [](std::string_view str, char delimiter) -> std::vector<std::string> {
            std::vector<std::string> tokens;
            size_t pos = 0;
            // Loop until pos is greater than the string size.
            while (pos <= str.size()) {
                size_t next = str.find(delimiter, pos);
                if (next == std::string_view::npos) {
                    // Add the final token (or the entire string if no delimiter found)
                    tokens.push_back(std::string(str.substr(pos)));
                    break;
                } else {
                    // Add token from current pos up to the delimiter
                    tokens.push_back(std::string(str.substr(pos, next - pos)));
                    pos = next + 1;
                }
            }
            return tokens;
        };

        auto lines = tokenize(entry, '\n');
        for (auto line : lines) {
            nvgTextAlign(nvg, NVG_ALIGN_CENTER);
            nvgText(nvg, x, y, line.data(), nullptr);
            y += 20;
        }
    }

    static constexpr std::string_view libraries[] = {
// TODO: Not implemented yet for standalone, bring it back soon for CLI mode
//{R"(linenoise-ng (CLI REPL)
//Martijn van Steenbergen
//BSD-3-Clause License
//https://github.com/arangodb/linenoise-ng)"},

{R"(moodycamel ConcurrentQueue (Lockfree queue)
Cameron Desrochers
Simplified BSD License
https://github.com/cameron314/concurrentqueue)"},

{R"(readerwriterqueue (Fast single-producer/consumer lock-free queue)
Cameron Desrochers
Simplified BSD License
https://github.com/cameron314/readerwriterqueue)"},

{R"(nlohmann/json (JSON file parsing)
Niels Lohmann
MIT License
https://github.com/nlohmann/json)"},

{R"(libsamplerate (Sample Rate Conversion Library)
Erik de Castro Lopo
2-Clause BSD License
https://github.com/libsndfile/libsamplerate)"},

{R"(PortAudio (Standalone Audio I/O)
PortAudio Team
MIT License
https://github.com/PortAudio/portaudio)"},

{R"(NanoVG (Vector Graphics Rendering)
Mikko Mononen / Timothy Schoen
Zlib License
https://github.com/timothyschoen/nanovg)"},

{R"(PFFFT (Fast Fourier Transform)
Julien Pommier
BSD-Like License
https://bitbucket.org/jpommier/pffft/src/master/)"},

{R"(SDL2 (Simple DirectMedia Layer)
SDL Team
zlib License
https://github.com/libsdl-org/SDL)"},

{R"(unordered_dense (Replacement for std::unordered_map)
Martin Ankerl
MIT License
https://github.com/martinus/unordered_dense)"},

{R"(CHOC (C++ Utility Library)
Tracktion / Julian Storer
ISC License
https://github.com/Tracktion/choc)"},

// We are not using glaze yet
//{R"(glaze (Extremely fast, in-memory, JSON and interface library for modern C++)
//Stephen Berry
//MIT License
//https://github.com/stephenberry/glaze)"},

{R"(RTMidi (Realtime MIDI I/O library for cross‐platform MIDI support)
Gary P. Scavone
MIT License
https://www.music.mcgill.ca/~gary/rtmidi/)"},

{R"(SIMDe (SIMD Everywhere – portable SIMD support for C/C++)
Evan Nemerson and contributors
MIT License
https://github.com/simd-everywhere/simde)"},
    };
};


class LibraryListView : public pptk::ComponentViewport
{
public:
    LibraryListView()
    {
        auto viewedComp = std::make_unique<LibraryList>();
        viewedComp->setBounds(0, 0, getWidth(), viewedComp->getLibraryHeight());
        setViewport(std::move(viewedComp));
    }

    void renderViewportBackground(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, 0, 0, width, height, theme.app.dialog_level_1, theme.app.general_border, 6.0f);
    }

    void resized() override
    {
        if (auto viewed = dynamic_cast<LibraryList*>(getViewedComponent()))
            viewed->setBounds(0, 0, getWidth(), viewed->getLibraryHeight());

        ComponentViewport::resized();
    }
};

class AboutDialog : public pptk::Component
{
    public:
    AboutDialog()
    {
        librariesPanel = std::make_unique<LibraryListView>();
        addComponent(librariesPanel.get());

        AboutDialog::resized();
    }

    void render(NVGcontext* nvg, const pptk::Theme& theme) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), theme.app.dialog_background, theme.app.general_border, 10.0f);

        nvgFontSize(nvg, 24.0f);
        nvgFontFace(nvg, "SemiBold");
        nvgTextAlign(nvg, NVG_ALIGN_CENTER);
        nvgFillColor(nvg, theme.app.general_text); // Text color

        int yPos = 50;
        nvgText(nvg, getWidth() * 0.5f, yPos, "Patchform", nullptr);
        yPos += 20;

        nvgFontSize(nvg, 14.0f);
        nvgFontFace(nvg, "Regular");
        nvgText(nvg, getWidth() * 0.5f, yPos, "created by Alex Mitchell", nullptr);
        yPos += 30;

        std::stringstream versionText;
        versionText << "Version: " << patchform_git_version << "      Git hash: " << patchform_git_hash;
        nvgText(nvg, getWidth() * 0.5f, yPos, versionText.str().c_str(), nullptr);
        yPos += 50;

        nvgFontSize(nvg, 24.0f);
        nvgFontFace(nvg, "SemiBold");
        nvgText(nvg, getWidth() * 0.5f, yPos, "Patreon supporters:", nullptr);
        yPos += 30;

        nvgFontSize(nvg, 14.0f);
        nvgFontFace(nvg, "Regular");
        std::string creditsLine;
        for (const char* credit : patreonCredits) {
            if (!creditsLine.empty()) {
                creditsLine += "    ";
            }
            creditsLine += credit;
        }

        nvgTextBox(nvg, 0, yPos, getWidth(), creditsLine.c_str(), nullptr);
        yPos += 50;

        nvgFontSize(nvg, 24.0f);
        nvgFontFace(nvg, "SemiBold");
        nvgText(nvg, getWidth() * 0.5f, yPos, "Libraries:", nullptr);
        yPos += 30;
    }

    void resized() override
    {
        //std::cout << "resizing the viewport" << std::endl;
        librariesPanel->setBounds(20, getHeight() * 0.4f, getWidth() - 40, getHeight() * 0.6f - 20);
    }

private:
    std::unique_ptr<LibraryListView> librariesPanel;

    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);

    static constexpr std::array<const char*, 4> patreonCredits = { "Nasko", "Joshua A.C.Newman", "Polarity", "el mono" };
};
