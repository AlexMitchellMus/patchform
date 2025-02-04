//
// Created by alexw on 4/02/2025.
//

#include "RightPanel.h"
#include "../UI_ToolKit/TextEditor.h"
#include "../Nodes/AudioNodeBase.h"

ParamItem::ParamItem(const std::string& name, Parameter* itemParam)
    : paramName(name)
    , param(itemParam)
{
    textBox = std::make_unique<TextEditor>();
    textBox->setText(std::to_string(param->getValue<float>()));
    addComponent(textBox.get());

    // Hook TextBox updates to parameter
    textBox->onTextChanged = ([this]() {
        try {
            float floatValue = std::stof(textBox->getText());
            param->setValue(floatValue);
            std::cout << "setting param to: " << floatValue << std::endl;
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
        std::cout << "updating right panel" << std::endl;
        if (!cnv->getSelectedObjects().empty())
        {
            setSelectedNode(cnv->getSelectedObjects().front()->audioNode);
        }
    });
}

void RightPanel::updateUI() {
    paramItems.clear(); // Clear all previous parameter UI elements

    if (!selectedNode) return;

    int yOffset = 20;
    for (auto& param : selectedNode->getParameters()) {
        auto paramItem = std::make_unique<ParamItem>(param->getName(), param.get());
        paramItem->setBounds(10, yOffset, width - 20, 30);
        addComponent(paramItem.get());
        paramItems.push_back(std::move(paramItem)); // Move to vector
        yOffset += 40;
    }
}
