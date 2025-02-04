#pragma once

#include "Component.h"
#include "SDL3/SDL.h"
#include "CompEvent.h"
#include <chrono>
#include <utility>

namespace pptk {

class TextEditor : public Component
{
public:

    std::function<void()> onTextChanged = [](){};

    TextEditor() : cursorPos(0), editorActive(false){}

    void setText(const std::string& newText)
    {
        text = newText;
        repaint();
    }

    std::string getText() const
    {
        return text;
    }

    void render(NVGcontext* vg) override
    {
        auto bgCol = nvgRGB(23, 23, 23);
        auto outlineCol = nvgRGB(55, 55, 55);
        nvgDrawRoundedRect(vg, 0, 0, width, height, bgCol, outlineCol, 8.0f);

        // Draw text
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
        nvgFontSize(vg, 20.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, 10, height * 0.5f, text.c_str(), nullptr);

        // Draw cursor if active
        if (editorActive)
        {
            // We get the textbounds from the NVG text
            float textBounds[4] = { 0.0f };
            float textWidth = nvgTextBounds(vg, 0, 0, text.substr(0, cursorPos).c_str(), nullptr, textBounds);
            // This is the format of the text bounds
            //std::cout << "xmin" << textBounds[0] << " ymin " << textBounds[1] << " xmax " << textBounds[2] << " ymax " << textBounds[3] << std::endl;
            float cursorX = 10 + textWidth;

            // We add the ymin (-) and ymax (+) from vertical alignment
            auto yMin = height * 0.5f + textBounds[1];
            auto yMax = height * 0.5f + textBounds[3];
            auto carrotCol = nvgRGBA(200, 200, 200, 255);
            nvgDrawRoundedRect(vg, cursorX, yMin, 2, yMax - yMin, carrotCol, carrotCol, 0);
        }
    }
/*
    void focusGained() override
    {
        std::cout << "editor focus gained" << std::endl;
        editorActive = true;
        repaint();
    }

    void focusLost() override
    {
        std::cout << "editor focus lost" << std::endl;
        editorActive = false;
        repaint();
    }
*/
    void keyPressed(CompEvent& e) override
    {
        if (!editorActive) return;

        const auto keycode = e.sdlEvent.key.key;

        switch (keycode)
        {
        case SDLK_BACKSPACE:
            if (!text.empty() && cursorPos > 0)
            {
                text.erase(cursorPos - 1, 1);
                onTextChanged();
                cursorPos--;
                repaint();
            }
            break;
        case SDLK_LEFT:
            if (cursorPos > 0)
            {
                cursorPos--;
                repaint();
            }
            break;
        case SDLK_RIGHT:
            if (cursorPos < text.length())
            {
                cursorPos++;
                repaint();
            }
            break;
        case SDLK_RETURN:
        case SDLK_RETURN2:
            editorActive = false;
            repaint();
            break;
        default:
            onCharInput(keycode);
            onTextChanged();
        }
    }

    void onCharInput(unsigned int codepoint)
    {
        if (!editorActive) return;

        SDL_Keymod modState = SDL_GetModState(); // Get active key modifiers (Shift, Ctrl, etc.)

        if (codepoint >= 32 && codepoint <= 126) // Printable characters
        {
            char character = static_cast<char>(codepoint);

            // Convert to uppercase if Shift is pressed
            if ((modState & SDL_KMOD_SHIFT) && character >= 'a' && character <= 'z')
            {
                character = character - ('a' - 'A'); // Convert lowercase to uppercase
            }

            text.insert(cursorPos, 1, character);
            cursorPos++;
            repaint();
        }
    }

    void mouseButtonDown(CompEvent& e) override
    {
        if (e.sdlEvent.button.clicks == 2)
        {
            editorActive = true;
            repaint();
        }
    }

private:
    std::string text;
    int cursorPos;
    bool editorActive;
};

} // namespace pptk
