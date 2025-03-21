/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "../UI_ToolKit/TextEditor.h"

// Display comment text only (no i/o, no processing)
class Comment : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Comment", "com");

    std::string commentText;

    StringParameter* commentTextParameter;

public:
    Comment(NodeContext* context, const json& objParams) : AudioNode(context, AudioPort::PortType::None, objParams)
    {
        commentText = objParams.value("text", "comment");

        //commentTextParameter = addParameter<StringParameter>("Text:");
    }

    json getSerializedNode() override
    {
        nodeCreationData["text"] = commentText;
        return nodeCreationData;
    }

    bool isDefaultUI() const override { return false; };

    bool isGuiOnly() const override { return true; };

    class UI final : public AudioNode::UI
    {

        bool isInit = false;

        std::unique_ptr<pptk::TextEditor> textEditor;

    public:
        explicit UI(AudioNode* node) : AudioNode::UI(node)
        {
            setGuiIsTransparent(true);

            textEditor = std::make_unique<pptk::TextEditor>(false);
            addComponent(textEditor.get());

            textEditor->setInteractable(false);

            auto commentNode = reinterpret_cast<Comment*>(audioNode);
            textEditor->setText(commentNode->commentText);

            textEditor->onTextChanged = [this, ed = textEditor.get()]()
            {
                auto commentNode = reinterpret_cast<Comment*>(audioNode);
                commentNode->commentText = ed->getText();
                updateWidth();
            };

            textEditor->onTextReturned = [this, ed = textEditor.get()]()
            {
                ed->setInteractable(false);
            };
        };

        void mouseButtonDown(pptk::CompEvent& e) override
        {
            std::cout << "mouse button down on comment" << std::endl;
            if (auto cnv = findParentOfClass<Canvas>())
            {
                if (!cnv->isInLockedMode())
                {
                    if (e.sdlEvent.button.clicks == 2)
                    {
                        textEditor->setInteractable(true);
                    } else
                        textEditor->setInteractable(false);

                    AudioNode::UI::mouseButtonDown(e);
                }
            }
        }

        //void mouseLeave(pptk::CompEvent& e) override
        //{
        //    std::cout << "mouse leaving comment" << std::endl;
        //    textEditor->setInteractable(false);
        //}

        void resized() override
        {
            textEditor->setBounds(0, 0, getWidth(), getHeight());
        }

        void updateGraphValues() override
        {
            if (isInit)
                return;

            isInit = true;
            updateWidth();
        }

        void updateWidth()
        {
            auto commentNode = reinterpret_cast<Comment*>(audioNode);
            auto textWidth = getTextWidthForFont("Regular", 14, commentNode->commentText) + 20;
            setSize(textWidth, getHeight());
        }

        void drawGUI(NVGcontext* nvg) override
        {
            return;

            nvgBeginPath(nvg);
            auto bgCol = nvgRGB(33, 33, 33);
            auto outLineCol = nvgRGB(45, 45, 45);
            if (getIsHovered()) bgCol = outLineCol;
            if (getIsSelected()) outLineCol = nvgRGB(28, 73, 119);
            //nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);

            nvgFontSize(nvg, 16.0f);
            nvgFontFace(nvg, "Regular");
            nvgFillColor(nvg, nvgRGB(190, 190, 190));
            nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

            float xOffset = 10.0f;

            auto commentNode = reinterpret_cast<Comment*>(audioNode);

            nvgText(nvg, xOffset, height / 2, commentNode->commentText.c_str(), nullptr);
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };
};
