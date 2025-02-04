#pragma once

#include "SDL3/SDL.h"
#include "CompEvent.h"
#include "Component.h"

namespace pptk {

class TextEditor : public Component
{
public:
    TextEditor(std::string textToEdit)
        : text(textToEdit)
        , cursorPos(0)
        , editorActive(true)
    {}

    void render(NVGcontext* vg) override
    {
        //nvgBeginPath(vg);
        //nvgRoundedRect(vg, 0, 0, width, height, 0.0f);
        //auto orange = nvgRGB(120, 74, 28);
        //nvgFillColor(vg, orange); // Blue fill for ports
        //nvgFill(vg);

        // Draw text
        nvgFillColor(vg, nvgRGBA(255, 0, 0, 255));
        nvgFontSize(vg, 20.0f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        nvgText(vg, 10, height, text.c_str(), nullptr);

        // Draw cursor if active
        if (editorActive)
        {
            float textWidth = nvgTextBounds(vg, 0, 0, text.substr(0, cursorPos).c_str(), nullptr, nullptr);
            float cursorX = 10 + textWidth;

            nvgBeginPath(vg);
            nvgMoveTo(vg, cursorX, 5);
            nvgLineTo(vg, cursorX, height - 5);
            nvgStrokeColor(vg, nvgRGBA(0, 255, 0, 255));
            nvgStrokeWidth(vg, 2.0f);
            nvgLineStyle(vg, NVG_SOLID);
            nvgStroke(vg);
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

        if (keycode == SDLK_BACKSPACE && !text.empty() && cursorPos > 0)
        {
            text.erase(cursorPos - 1, 1);
            cursorPos--;
            repaint();
        }
        else if (keycode == SDLK_LEFT && cursorPos > 0)
        {
            cursorPos--;
            repaint();
        }
        else if (keycode == SDLK_RIGHT && cursorPos < text.length())
        {
            cursorPos++;
            repaint();
        }
        else
        {
            onCharInput(keycode);
        }
    }

    void onCharInput(unsigned int codepoint)
    {
        if (!editorActive) return;

        if (codepoint >= 32 && codepoint <= 126) // Printable characters
        {
            text.insert(cursorPos, 1, static_cast<char>(codepoint));
            cursorPos++;
            auto parent = getParent();
            auto pB = parent->getBounds();
            parent->setBounds(pB.x, pB.y, text.length() * 13, pB.h);
            repaint();
        }
    }

    void mouseButtonDown(CompEvent& e) override
    {
        auto mPos = Point(e.sdlEvent.button.x, e.sdlEvent.button.y);
        //editorActive = (mPos.x >= 0 && mPos.y <= 0 + width && mPos.y >= 0 && mPos.y <= 0 + height);
        if (!editorActive)
            parent->mouseButtonDown(e);
    }

private:
    std::string text;
    int cursorPos;
    bool editorActive;
};

} // namespace pptk
