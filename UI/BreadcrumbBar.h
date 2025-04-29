#pragma once

#include "../UI_ToolKit/Component.h"

class GraphManager;
class BreadcrumbBar final : public pptk::Component {
public:
    struct Segment {
        GraphManager* mgr;
        std::string name;
        pptk::Rect bounds;
    };

    void setViewedGraph(GraphManager* g);
    void updateLayout();
    void render(NVGcontext* nvg, const pptk::Theme& theme) override;
    void mouseButtonDown(pptk::CompEvent& e) override;
    void mouseLeave(pptk::CompEvent& e) override;
    void mouseMove(const pptk::Point& position) override;

    std::function<void(GraphManager*)> onClick;

private:
    GraphManager* currentGraph = nullptr;
    std::vector<Segment> segments;

    static std::vector<GraphManager*> getBreadcrumbTrail(GraphManager* mgr);

    pptk::Point mousePos;
    int hoveredIndex = -1;
};
