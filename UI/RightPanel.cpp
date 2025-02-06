//
// Created by alexw on 4/02/2025.
//

#include "RightPanel.h"
#include "../UI_ToolKit/TextEditor.h"
#include "../Nodes/AudioNodeBase.h"
#include "../UI_ToolKit/Resizer.h"

ParamItem::ParamItem(const std::string& name, Parameter* itemParam)
    : paramName(name)
    , param(itemParam)
{
    textBox = std::make_unique<pptk::TextEditor>();
    textBox->setText(param->getAsString());
    addComponent(textBox.get());

    // Hook TextBox updates to parameter
    textBox->onTextReturned = ([this]() {
        try {
            std::cout << "sending data to node??" << std::endl;
            param->setFromString(textBox->getText());
        } catch (...) {
            // Invalid input, ignore it
        }
    });
}

void ParamItem::resized()
{
    textBox->setBounds(100, 5, getWidth() - 100, 25); // Place TextBox next to label
}

RightPanel::RightPanel(Canvas* cnv)
{
    cnv->addObjectChangedListener([this, cnv]()
    {
        if (cnv->getSelectedObjects().empty())
            setSelectedNode(nullptr);
        else
            setSelectedNode(cnv->getSelectedObjects().front()->audioNode);
    });

    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Left);
}

void RightPanel::resized() {
    int yOffset = 50;
    for (auto& item : paramItems) {
        item->setBounds(10, yOffset, width - 20, 30);
        yOffset += 30;
    }

    getResizer().setBounds(getBounds());
}

void RightPanel::updateUI() {
    paramItems.clear();

    if (!selectedNode) return;

    for (auto& param : selectedNode->getParameters()) {
        auto paramItem = std::make_unique<ParamItem>(param->getName(), param.get());
        addComponent(paramItem.get());
        paramItems.push_back(std::move(paramItem));
    }

    RightPanel::resized();
}
