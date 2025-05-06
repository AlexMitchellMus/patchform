#pragma once

class ObjectResizer : public pptk::Component
{
public:
    enum class Edge {
        None   = 0,
        Left   = 1 << 0,
        Right  = 1 << 1,
        Top    = 1 << 2,
        Bottom = 1 << 3
    };

    std::function<void(int dx, int dy, Edge edge)> onResize;

    void resized() override
    {
        if (!resizingActive) {
            setBounds(0, 0, getParent()->getWidth(), getParent()->getHeight());
        }
        repaint();
    }

    void mouseLeave(pptk::CompEvent& e) override
    {
        hoveredEdge = Edge::None;
        updateCursor();
    }

    void mouseMove(const pptk::Point& pos) override
    {
        hoveredEdge = hitTestEdge(pos);
        updateCursor();
    }

    void render(NVGcontext* vg, const pptk::Theme& theme) override
    {
        nvgBeginPath(vg);
        nvgRect(vg, 0, 0, getWidth(), getHeight());
        nvgFillColor(vg, nvgRGBA(255, 0, 0, 100));
        nvgFill(vg);
    }

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        dragStart = { e.sdlEvent.button.x, e.sdlEvent.button.y };
        startBounds = getParent()->getBounds();
        std::cout << "mouse down at: " << dragStart.x << ", " << dragStart.y << std::endl;
    }

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point&, pptk::Button) override
    {
        resizingActive = true;
        auto deltaPos = currentPosition - dragStart;
        currentMousePos = currentPosition;
        std::cout << "drag pos: " << currentPosition.x << ", " << currentPosition.y
                  << " | delta: " << deltaPos.x << ", " << deltaPos.y << std::endl;

        if (hoveredEdge != Edge::None && onResize)
            onResize(static_cast<int>(deltaPos.x), static_cast<int>(deltaPos.y), hoveredEdge);

        resizingActive = false;
    }

    pptk::Rect getStartBounds() const
    {
        return startBounds;
    }

    pptk::Point getCurrentMousePos() const
    {
        return currentMousePos;
    }

    bool getResizingActive() const
    {
        return resizingActive;
    }

private:
    bool resizingActive = false;

    pptk::Point dragStartGlobal;

    Edge hoveredEdge = Edge::None;
    pptk::Point dragStart;
    pptk::Rect startBounds;
    pptk::Point currentMousePos;
    static constexpr int edgeThickness = 6;

    void updateCursor()
    {
        std::cout << "hovered edge: " << edgeToString(hoveredEdge) << std::endl;
        SDL_SetCursor(SDL_CreateSystemCursor(edgeToCursor(hoveredEdge)));
    }

    Edge hitTestEdge(const pptk::Point pt) const
    {
        int edge = static_cast<int>(Edge::None);
        if (pt.x < edgeThickness)
            edge |= static_cast<int>(Edge::Left);
        else if (pt.x > getWidth() - edgeThickness)
            edge |= static_cast<int>(Edge::Right);
        if (pt.y < edgeThickness)
            edge |= static_cast<int>(Edge::Top);
        else if (pt.y > getHeight() - edgeThickness)
            edge |= static_cast<int>(Edge::Bottom);

        return static_cast<Edge>(edge);
    }

    SDL_SystemCursor edgeToCursor(Edge e) {
        switch (e)
        {
        case Edge::Left:
        case Edge::Right:
            return SDL_SYSTEM_CURSOR_EW_RESIZE;

        case Edge::Top:
        case Edge::Bottom:
            return SDL_SYSTEM_CURSOR_NS_RESIZE;

        case Edge(int(Edge::Top) | int(Edge::Left)):
        case Edge(int(Edge::Bottom) | int(Edge::Right)):
            return SDL_SYSTEM_CURSOR_NWSE_RESIZE;

        case Edge(int(Edge::Top) | int(Edge::Right)):
        case Edge(int(Edge::Bottom) | int(Edge::Left)):
            return SDL_SYSTEM_CURSOR_NESW_RESIZE;

        default: return SDL_SYSTEM_CURSOR_DEFAULT;
        }
    }

    static const char* edgeToString(Edge e) {
        static thread_local std::string str;
        str.clear();

        if (e == Edge::None) return "None";
        if (static_cast<int>(e) & static_cast<int>(Edge::Left))   str += "Left ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Right))  str += "Right ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Top))    str += "Top ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Bottom)) str += "Bottom ";

        return str.c_str();
    }

};
