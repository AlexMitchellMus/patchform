/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "AudioNodeBase.h"
#include "../UI_ToolKit/TextEditor.h"

// Display comment text only (no i/o, no processing)
class Comment final : public AudioNode
{
    DEFINE_AND_REGISTER_NODE("Comment", "com", false);
    DEFINE_NODE_ALIASES("comment");

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
            // No need to draw any GUI, as comment uses a text editor
        }
    };

    std::unique_ptr<AudioNode::UI> makeUI() override
    {
        return std::make_unique<UI>(this);
    };

    bool shouldProcess(unsigned int frameCount) override
    {
        // Currently this object is the only one that is UI only, no processing
        return false;
    }
};

REGISTER(Comment);
