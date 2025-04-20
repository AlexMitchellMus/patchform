#pragma once

#include <UI_ToolKit/Component.h>
#include <UI_ToolKit/Label.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct PatchInfo {
    std::string fullPath;
    std::string fileName;
    std::function<void()> onClose;
    bool isDirty = false;

    PatchInfo(const std::string& path, std::function<void()> close, const bool inDirtyState)
        : fullPath(path)
        , onClose(std::move(close))
        , isDirty(inDirtyState)
    {
        size_t slash = path.find_last_of("/\\");
        std::string base = (slash != std::string::npos) ? path.substr(slash + 1) : path;

        size_t dot = base.find_last_of('.');
        fileName = (dot != std::string::npos) ? base.substr(0, dot) : base;

        isDirty = path.find("virtual") != std::string::npos ? true : isDirty;
    }
};

class PatchItem : public pptk::Component {
public:
    std::function<void()> onClick = [](){};

    explicit PatchItem(const PatchInfo& info);

    void mouseButtonDown(pptk::CompEvent& e) override;
    void mouseEnter(pptk::CompEvent& e) override;
    void mouseLeave(pptk::CompEvent& e) override;
    void resized() override;
    void render(NVGcontext* vg, const pptk::Theme& theme) override;

    std::string& getPatchName();
    std::string& getPatchPath();

    bool isSelected = false;

private:
    std::unique_ptr<pptk::Label> label;

    PatchInfo patchInfo;
    std::string patchName;
    bool isHovered = false;
};

class Editor;
class LoadedPatchesPanel : public pptk::Component {
public:
    explicit LoadedPatchesPanel(Editor* ed);

    void updateTabs(const std::vector<std::tuple<std::string, bool>>& tabs);

    void setPatches(const std::vector<PatchInfo>& patches);
    void resized() override;

    void setPatchSelected(const std::string& selectedPatch);

private:
    std::vector<std::unique_ptr<PatchItem>> patchItems;
    Editor* editor;

    std::string currentlySelectedPatch;
};
