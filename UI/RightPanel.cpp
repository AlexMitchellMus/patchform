//
// Created by alexw on 4/02/2025.
//

#include "RightPanel.h"
#include "../UI_ToolKit/TextEditor.h"

RightPanel::RightPanel()
{
    textEditorA = std::make_unique<pptk::TextEditor>();
    addComponent(textEditorA.get());

    textEditorA->setText("default");

    textEditorA->onTextChanged = [this]()
    {
        std::cout << textEditorA->getText() << std::endl;
    };

    textEditorB = std::make_unique<pptk::TextEditor>();
    addComponent(textEditorB.get());

    textEditorB->setText("thing2");

    RightPanel::resized();
}

void RightPanel::resized()
{
    textEditorA->setBounds(5, 5, 180, 50);
    textEditorB->setBounds(5, 5 + 55, 180, 50);
}
