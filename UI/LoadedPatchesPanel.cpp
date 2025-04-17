#include "LoadedPatchesPanel.h"
#include "Editor.h"
#include <iostream>
#include <nanovg.h>

PatchItem::PatchItem(const PatchInfo& info)
    : patchInfo(info)
    , patchName(info.fileName)
{
    label = std::make_unique<pptk::Label>(patchName, true);
    addComponent(label.get());
    label->setInterceptsMouseClicks(false, false);
}

void PatchItem::mouseButtonDown(pptk::CompEvent& e)
{
    onClick();
}

void PatchItem::mouseEnter(pptk::CompEvent& e)
{
    isHovered = true;
    repaint();
}

void PatchItem::mouseLeave(pptk::CompEvent& e)
{
    isHovered = false;
    repaint();
}

void PatchItem::resized()
{
    label->setBounds(24, 0, width - 32, height); // leave margin for padding
}

void PatchItem::render(NVGcontext* vg, const pptk::Theme& theme)
{
    if (isSelected || isHovered)
    {
        NVGcolor bg = isSelected
            ? nvgRGB(43, 43, 43)
            : nvgRGBA(43, 43, 43, static_cast<unsigned char>(255 * 0.4f));

        nvgDrawRoundedRect(vg, 8, 4, width - 16, height - 8, bg, bg, 6.0f);
    }
}

std::string& PatchItem::getPatchName()
{
    return patchName;
}

std::string& PatchItem::getPatchPath()
{
    return patchInfo.fullPath;
}

LoadedPatchesPanel::LoadedPatchesPanel(Editor* ed) : editor(ed)
{
}

void LoadedPatchesPanel::updateTabs(const std::vector<std::string>& tabs)
{
    std::vector<PatchInfo> patchList;
    for (auto& name : tabs)
    {
        patchList.push_back({ name, []() {} });
    }
    setPatches(patchList);
}

void LoadedPatchesPanel::setPatches(const std::vector<PatchInfo>& patches)
{
    removeAllChildren();
    patchItems.clear();

    for (const auto& p : patches) {
        auto item = std::make_unique<PatchItem>(p);
        addComponent(item.get());
        item->onClick = [rawItem = item.get(), this]()
        {
            editor->loadFile(rawItem->getPatchPath());
            setSelected(rawItem->getPatchName());
        };
        patchItems.push_back(std::move(item));
    }

    resized();
    repaint();
}

void LoadedPatchesPanel::setSelected(const std::string& selectedPatch)
{
    for (auto& item : patchItems)
    {
        item->isSelected = item->getPatchName() == selectedPatch;
        repaint();
    }
}

void LoadedPatchesPanel::resized()
{
    setSize(getParent()->getWidth(), static_cast<int>(patchItems.size()) * 32 + 10);
    int offsetY = 0;
    for (auto& item : patchItems) {
        item->setBounds(0, offsetY, getWidth(), 32);
        offsetY += 32;
    }
}