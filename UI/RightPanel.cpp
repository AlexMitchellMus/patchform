//
// Created by alexw on 4/02/2025.
//

#include "RightPanel.h"
#include "../UI_ToolKit/TextEditor.h"
#include "../Nodes/AudioNodeBase.h"
#include "../UI_ToolKit/Resizer.h"
#include "../UI_ToolKit/ToggleSwitch.h"

ParamItem::ParamItem(const std::string& name, Parameter* itemParam)
    : paramName(name)
    , param(itemParam)
{
    if (auto* boolParam = dynamic_cast<BoolParameter*>(itemParam)) {
        auto toggle = std::make_unique<ToggleSwitch>();
        toggle->setState(boolParam->getValue());
        toggle->onToggle = [boolParam](bool state) {
            boolParam->setValue(state);
        };
        addComponent(toggle.get());
        textBox = nullptr; // not used
        toggleSwitch = std::move(toggle); // store it
    } else {
        bool isParamString = dynamic_cast<StringParameter*>(itemParam);
        textBox = std::make_unique<pptk::TextEditor>(!isParamString);
        textBox->setText(param->getAsString());
        addComponent(textBox.get());

        textBox->onTextReturned = ([this]() {
            try {
                param->setFromString(textBox->getText());
            } catch (...) {}
        });
    }
}

void ParamItem::resized()
{
    //auto textwidth = getTextWidthForFont("Regular", 14.0f, textBox->getText());
    //std::cout << "text width from cache: " << textwidth << std::endl;
    if (textBox)
        textBox->setBounds(100, 3, getWidth() - 100, 25); // Place TextBox next to label
    else if (toggleSwitch)
        toggleSwitch->setBounds(100 + 10, 6, 30, 18);
}

RightPanel::RightPanel(Canvas* cnv)
{
    cnv->addObjectChangedListener([this, cnv]()
    {
        setSelectedObjects(cnv->getSelectedObjects());
    });

    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Left);

    updateUI();
}

void RightPanel::setSelectedObjects(std::vector<Object*> objs)
{
    if (objs.empty())
    {
        selectedNode = nullptr;
        paramItems.clear();
        numSelected = 0;
    } else
    {
        selectedNode = objs.front()->audioNode;
        numSelected = objs.size();
    }

    updateUI();
    repaint();
}

void RightPanel::resized() {
    int yOffset = 50;
    for (auto& item : paramItems) {
        item->setBounds(8, yOffset, width - 8 - 8, 30);
        yOffset += 30;
    }

    getResizer().setBounds(getBounds());
}

void RightPanel::updateUI() {
    parameterName = "Parameters: (empty)";

    if (selectedNode)
    {
        parameterName = "Parameters: " + (numSelected > 1 ? "(" + std::to_string(numSelected) + ") " : "") +  selectedNode->getName();
    }
    paramItems.clear();

    if (!selectedNode) return;

    for (auto& param : selectedNode->getParameters()) {
        auto paramItem = std::make_unique<ParamItem>(param->getName(), param.get());
        addComponent(paramItem.get());
        paramItems.push_back(std::move(paramItem));
    }

    RightPanel::resized();
}
