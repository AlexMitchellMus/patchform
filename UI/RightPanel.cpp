#include "RightPanel.h"
#include "../Nodes/AudioNodeBase.h"
#include "../UI_ToolKit/Resizer.h"

ParamItem::ParamItem(const std::string& name, Parameter* itemParam)
    : paramName(name)
    , param(itemParam)
{
#ifdef PATCHFORM_WITH_GUI
    editor = param->createEditorComponent();
    if (editor)
        addComponent(editor.get());
#endif
}

void ParamItem::resized()
{
    if (editor)
        param->resizeEditorComponent(editor.get(), getWidth(), getHeight());
}

RightPanel::RightPanel(Canvas* cnv)
{
    cnv->addObjectChangedListener([this, cnv]() {
        setSelectedObjects(cnv->getSelectedObjects());
    });

    setMinMaxSize(150, 350, 0, 0);
    setResizable(pptk::Resizer::ResizerMode::Left);
    updateUI();
}

void RightPanel::setSelectedObjects(std::vector<Object*> objs)
{
    selectedNode = objs.empty() ? nullptr : objs.front()->audioNode;
    numSelected = objs.size();
    updateUI();
    repaint();
}

void RightPanel::resized()
{
    int yOffset = 50;
    for (auto& item : paramItems) {
        item->setBounds(8, yOffset, width - 16, 30);
        yOffset += 30;
    }
    getResizer().setBounds(getBounds());
}

void RightPanel::updateUI()
{
    parameterName = selectedNode
        ? "Parameters: " + (numSelected > 1 ? "(" + std::to_string(numSelected) + ") " : "") + selectedNode->getName()
        : "Parameters: (empty)";

    paramItems.clear();
    if (!selectedNode) return;

    for (auto& param : selectedNode->getParameters()) {
        auto paramItem = std::make_unique<ParamItem>(param->getName(), param.get());
        addComponent(paramItem.get());
        paramItems.push_back(std::move(paramItem));
    }

    resized();
}

void RightPanel::render(NVGcontext* vg)
{
    nvgFillColor(vg, nvgRGB(33, 33, 33));
    nvgFillRect(vg, 0, 0, width, height);

    nvgBeginPath(vg);
    nvgMoveTo(vg, 0.5f, 0);
    nvgLineTo(vg, 0.5f, height);
    nvgStrokeColor(vg, nvgRGB(53, 53, 53));
    nvgStrokeWidth(vg, 1.0f);
    nvgStroke(vg);

    nvgFillColor(vg, nvgRGB(220, 220, 220));
    nvgFontSize(vg, 14.0f);
    nvgFontFace(vg, "SemiBold");
    nvgTextAlign(vg, NVG_ALIGN_LEFT);
    nvgText(vg, 24, 40, parameterName.c_str(), nullptr);
}
