#pragma once

#include "Component.h"
#include "SDL3/SDL.h"
#include "CompEvent.h"
#include <chrono>

namespace pptk {

class TextEditor : public Component
{
public:

    std::function<void()> onTextChanged = [](){};
    std::function<void()> onTextReturned = [](){};

    TextEditor(bool isNumber = true) : cursorPos(0), isNumber(isNumber){}

    void setText(const std::string& newText)
    {
        text = newText;
        convertArrowsToChar();
        repaint();
    }

    std::string getText() const
    {
        return text;
    }

    void render(NVGcontext* vg) override
    {
        if (!editorActive && isHovered)
        {
            auto col = nvgRGB(45, 45, 45);
            nvgDrawRoundedRect(vg, 0, 0, width, height, col, col, 6.0f);
        }
        // Draw text
        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "Regular");
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        // Draw cursor if active
        if (editorActive)
        {
            // We get the textbounds from the NVG text
            float fullTextWidth = nvgTextBounds(vg, 0, 0, text.c_str(), nullptr, nullptr);
            float textBounds[4] = { 0.0f };
            float textWidth = nvgTextBounds(vg, 0, 0, text.substr(0, cursorPos).c_str(), nullptr, textBounds);
            // This is the format of the text bounds
            //std::cout << "xmin" << textBounds[0] << " ymin " << textBounds[1] << " xmax " << textBounds[2] << " ymax " << textBounds[3] << std::endl;
            float cursorX = 10 + textWidth;

            // We add the ymin (-) and ymax (+) from vertical alignment
            auto yMin = height * 0.5f + textBounds[1];
            auto yMax = height * 0.5f + textBounds[3];

            if (editorFirstActive)
            {
                auto blue = nvgRGB(28, 73, 119);
                nvgDrawRoundedRect(vg, 10, yMin, 10 + fullTextWidth, yMax - yMin, blue, blue, 0.0f);
            }

            auto carrotCol = nvgRGBA(200, 200, 200, 255);
            nvgDrawRoundedRect(vg, cursorX, yMin, 2, yMax - yMin, carrotCol, carrotCol, 0);
        }

        nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
        std::cout << text << " : " << displayText << std::endl;
        nvgText(vg, 10, height * 0.5f, editorActive ? text.c_str() : displayText.c_str(), nullptr);
    }
/*
    void focusGained() override
    {
        std::cout << "editor focus gained" << std::endl;
        editorActive = true;
        repaint();
    }
*/
    void focusLost() override
    {
        std::cout << "editor focus lost" << std::endl;
        editorActive = false;
        onTextReturned();
        repaint();
    }

    bool hitTest(float px, float py) override
    {
        if (isInteractable)
            return Component::hitTest(px, py);

        return false;
    }

    /*
    bool consumeEvent(CompEvent& e) override
    {
        return editorActive;
    }
    */

    void keyPressed(CompEvent& e) override
    {
        if (!editorActive) return;

        const auto keycode = e.sdlEvent.key.key;

        switch (keycode)
        {
        case SDLK_BACKSPACE:
            editorFirstActive = false;
            if (!text.empty() && cursorPos > 0)
            {
                text.erase(cursorPos - 1, 1);
                onTextChanged();
                cursorPos--;
                repaint();
            }
            break;
        case SDLK_LEFT:
            editorFirstActive = false;
            if (cursorPos > 0)
            {
                cursorPos--;
                repaint();
            }
            break;
        case SDLK_RIGHT:
            editorFirstActive = false;
            if (cursorPos < text.length())
            {
                cursorPos++;
                repaint();
            }
            break;
        case SDLK_RETURN:
        case SDLK_RETURN2:
            editorActive = false;
            onTextReturned();
            loseFocus();
            repaint();
            break;
        default:
            onCharInput(keycode);
            onTextChanged();
        }

        convertArrowsToChar();
        repaint();
    }

    void onCharInput(unsigned int codepoint)
    {
        if (!editorActive) return;

        SDL_Keymod modState = SDL_GetModState(); // Get active key modifiers
        if (codepoint >= 32 && codepoint <= 126) // Printable characters
        {
            char character = static_cast<char>(codepoint);
            bool validChar = true;

            if (isNumber)
            {
                // Allow only digits and a single '.'
                if (!((character >= '0' && character <= '9') || character == '.'))
                {
                    validChar = false;
                }
                // Prevent entering more than one '.'
                if (character == '.' && text.find('.') != std::string::npos)
                {
                    validChar = false;
                }
            }
            else if (modState & SDL_KMOD_SHIFT)
            {
                // Handle letters and symbols when shift is pressed.
                if (character >= 'a' && character <= 'z') {
                    character = character - ('a' - 'A');
                } else {
                    character = getShiftedSymbol(character);
                }
            }

            // Only insert character and clear text if the character is valid.
            if (validChar)
            {
                // Clear the text only on the first valid key press.
                if (editorFirstActive)
                {
                    text.clear();
                    cursorPos = 0;
                    editorFirstActive = false;
                }
                text.insert(cursorPos, 1, character);
                cursorPos++;
            }
        }
    }

    void mouseEnter(CompEvent& e) override
    {
        if (isHovered != true)
        {
            isHovered = true;
            repaint();
        }
    }

    void mouseLeave(CompEvent& e) override
    {
        if (isHovered == true)
        {
            isHovered = false;
            repaint();
        }
    }

    void mouseDrag(const Point& position, const Point& delta, Button button) override
    {
        if (!editorActive && !isNumber)
            return;

        wasDragged = true;
        draggedNumValue -= delta.y * 0.5f;
        setText(std::to_string(static_cast<int>(draggedNumValue)));
        onTextReturned();
    }

    void mouseButtonDown(CompEvent& e) override
    {
        wasDragged = false;
        if (e.sdlEvent.button.clicks == 2)
        {
            editorActive = true;
            editorFirstActive = true;
            gainFocus();
            repaint();
        } else if (isNumber && e.sdlEvent.button.clicks == 1)
        {
            draggedNumValue = std::stof(getText());
        }
    }

    void mouseButtonUp(CompEvent& e) override
    {
        wasDragged = false;
    }

    void setInteractable(bool shouldInteract)
    {
        std::cout << "setting text editor interactable: " << std::boolalpha << shouldInteract << std::endl;
        if (isInteractable != shouldInteract)
        {
            isInteractable = shouldInteract;
            if (isInteractable)
                focusGained();
            else
                focusLost();
            repaint();
        }
    }

private:
    bool wasDragged = false;
    float draggedNumValue = 0.0f;
    std::string text;
    std::string displayText;
    int cursorPos;
    bool isNumber;
    bool editorActive = false;
    bool editorFirstActive = false;

    bool isHovered = false;

    bool isInteractable = true;

    static char getShiftedSymbol(const char c) {
        switch(c) {
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case ';': return ':';
        case '\'': return '"';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        case '\\': return '|';
        case '`': return '~';
        default: return c;
        }
    }

    void convertArrowsToChar()
    {
        displayText.clear();
        size_t len = text.size();
        for (size_t i = 0; i < len; ++i) {
            // Check for "<-" sequence.
            if (i + 1 < len && text[i] == '<' && text[i + 1] == '-') {
                // Determine if there is a valid preceding character:
                // Valid if it's the first character, or the preceding character is a space.
                bool validLeft = (i == 0) || (text[i - 1] == ' ');
                // Determine if there is a valid following character:
                // Valid if the sequence ends at the end of the string, or if the character after '-' is a space.
                bool validRight = (i + 2 >= len) || (text[i + 2] == ' ');
                if (validLeft && validRight) {
                    displayText += "←";
                    i++; // Skip the '-' character.
                    continue;
                }
            }
            // Check for "->" sequence.
            if (i + 1 < len && text[i] == '-' && text[i + 1] == '>') {
                bool validLeft = (i == 0) || (text[i - 1] == ' ');
                bool validRight = (i + 2 >= len) || (text[i + 2] == ' ');
                if (validLeft && validRight) {
                    displayText += "→";
                    i++; // Skip the '>' character.
                    continue;
                }
            }
            // Otherwise, just copy the current character.
            displayText.push_back(text[i]);
        }
    }

};

} // namespace pptk
