#include "BreadcrumbBar.h"
#include <filesystem>
#include "../Graph/GraphManager.h"
#include "../UI_ToolKit/FontMetrics.h"
#include <ranges>

void BreadcrumbBar::setViewedGraph(GraphManager* g) {
    currentGraph = g;
    updateLayout();
    repaint();
}

std::vector<GraphManager*> BreadcrumbBar::getBreadcrumbTrail(GraphManager* mgr) {
    std::vector<GraphManager*> trail;
    while (mgr) {
        trail.push_back(mgr);
        mgr = mgr->parentGraph;
    }
    std::ranges::reverse(trail);
    return trail;
}

void BreadcrumbBar::updateLayout() {
    segments.clear();
    if (!currentGraph) return;

    auto trail = getBreadcrumbTrail(currentGraph);

    float x = 10.0f;
    float y = 5.0f;
    float totalWidth = x;

    for (size_t i = 0; i < trail.size(); ++i)
    {
        auto* mgr = trail[i];
        std::string name = std::filesystem::path(mgr->getPatchFile()).stem().string();
        const float w = getTextWidthForFont("Regular", 14.0f, name);

        pptk::Rect bounds = {x, y, w, 20};
        segments.push_back({mgr, name, bounds});
        x += w;

        if (i + 1 < trail.size()) {
            constexpr float gap = 5.0f;
            const float arrowWidth = getTextWidthForFont("Regular", 14.0f, " › ");
            const float nextX = x + gap + arrowWidth + gap;

            const float center = (x + nextX - arrowWidth) * 0.5f;
            pptk::Rect arrowBounds = {center, y, arrowWidth, 20};
            segments.push_back({nullptr, " › ", arrowBounds}); // nullptr = not clickable

            x = nextX + gap;
        }
    }

    totalWidth = x + 10.0f; // padding right
    setSize(totalWidth, 30.0f); // or setBounds(...) if needed
}

void BreadcrumbBar::mouseLeave(pptk::CompEvent& e)
{
    hoveredIndex = -1;
    repaint();
}

void BreadcrumbBar::mouseMove(const pptk::Point& position) {
    // FIXME: This isn't correct- it's offset for some reason!
    mousePos = position;
    hoveredIndex = -1;

    for (size_t i = 0; i < segments.size(); ++i) {
        if (segments[i].bounds.contains(mousePos.x, mousePos.y)) {
            hoveredIndex = static_cast<int>(i);
            break;
        }
    }

    repaint();
}

void BreadcrumbBar::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    //nvgDrawRoundedRect(nvg, 0, 0, width, height, nvgRGBA(255,0,0,60), nvgRGBA(255,0,0,60), 4.0f);
    for (size_t i = 0; i < segments.size(); ++i) {
        const auto& seg = segments[i];

        /* FIXME: Something is wrong with the mouse move, and layout position
        if (i == hoveredIndex) {
            float padding = 4.0f;
            float rx = seg.bounds.x - padding;
            float ry = seg.bounds.y;
            float rw = seg.bounds.w + 2 * padding;
            float rh = seg.bounds.h;

            nvgDrawRoundedRect(nvg, rx, ry, rw, rh, theme.app.general_border, theme.app.general_border, 4.0f);
        }
        */

        nvgFillColor(nvg, theme.app.general_text);
        nvgFontFace(nvg, "Regular");
        nvgFontSize(nvg, 14.0f);
        nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

        nvgText(nvg, seg.bounds.x, seg.bounds.y + 15, seg.name.c_str(), nullptr);
    }
}

void BreadcrumbBar::mouseButtonDown(pptk::CompEvent& e) {
    std::cout << "mouse button down: " << e.sdlEvent.button.x << ", " << e.sdlEvent.button.y << std::endl;
    for (const auto& seg : segments) {
        if (seg.bounds.contains(e.sdlEvent.button.x, e.sdlEvent.button.y)) {
            if (onClick)
                onClick(seg.mgr);
            break;
        }
    }
}