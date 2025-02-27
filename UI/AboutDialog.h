//
// Created by alexw on 27/02/2025.
//

#pragma once

#include "../UI_Toolkit/Component.h"
#include "../GitInfo.h"

class AboutDialog : public pptk::Component
{
    public:
    AboutDialog()
    {
        setWantsFocus(true);
    }

    void render(NVGcontext* nvg) override
    {
        nvgBeginPath(nvg);
        nvgDrawRoundedRect(nvg, - 3,  - 3, getWidth() + 6, getHeight() + 6, dropShadowCol, dropShadowCol, 13);
        nvgDrawRoundedRect(nvg, 0, 0, getWidth(), getHeight(), bg, outline, 10.0f);

        nvgFontSize(nvg, 24.0f);
        nvgFontFace(nvg, "SemiBold");
        nvgTextAlign(nvg, NVG_ALIGN_CENTER);
        nvgFillColor(nvg, nvgRGB(220, 220, 220)); // Text color

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

        nvgFontSize(nvg, 14.0f);
        nvgFontFace(nvg, "Regular");
        for (auto lib : libraries) {
            drawLibraryEntry(nvg, getWidth() * 0.5f, yPos, lib);
            yPos += 30.0f;  // move down for the next entry
        }
    }

    void resized() override
    {
        setBounds(getRootComponent()->getWidth() * 0.5f - 400, getRootComponent()->getHeight() * 0.5f - 300, 800, 600);
    }

private:
    NVGcolor bg = nvgRGB(43, 43, 43);
    NVGcolor outline = nvgRGB(53, 53, 53);
    NVGcolor dropShadowCol = nvgRGBA(0, 0, 0, 30);

    static constexpr std::array<const char*, 4> patreonCredits = { "Nasko", "Joshua A.C.Newman", "Polarity", "el mono" };
    static constexpr std::array<std::string_view, 6> libraries = {{
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

#include <vector>
#include <string_view>


    void drawLibraryEntry(NVGcontext* nvg, float x, int& y, const std::string_view& entry) {
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
};
