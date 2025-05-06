#pragma once

class ObjectResizer : public pptk::Component
{
public:
    enum class Edge
    {
        None = 0,
        Left = 1 << 0,
        Right = 1 << 1,
        Top = 1 << 2,
        Bottom = 1 << 3
    };

    std::function<void(int dx, int dy, Edge edge)> onResize;

    void setAspectRatio(float ratio)
    {
        aspectRatio = ratio;
        resized();
    }

    void resized() override
    {
        repaint();
    }

    void mouseEnter(pptk::CompEvent& e) override {
        isHovered = true;
        updateCursor();
    }

    void mouseLeave(pptk::CompEvent& e) override {
        isHovered = false;
        hoveredEdge = Edge::None;
        updateCursor();

        startFrameTimer([this](uint32_t time, uint32_t deltaTime)
        {
            float speed = 0.001f;
            bool anyActive = false;

            for (int i = 0; i < 4; ++i)
            {
                hoverAlpha[i] = std::max(0.0f, hoverAlpha[i] - deltaTime * speed);
                if (hoverAlpha[i] > 0.0f)
                    anyActive = true;
            }

            repaint();
            if (!anyActive)
                stopFrameTimer();
        });
    }

    void mouseMove(const pptk::Point& pos) override
    {
        Edge newEdge = hitTestEdge(pos);
        hoveredEdge = newEdge;
        updateCursor();

        startFrameTimer([this](uint32_t time, uint32_t deltaTime)
        {
            float speed = 0.001f;
            bool anyActive = false;

            for (int i = 0; i < 4; ++i)
            {
                if ((int(hoveredEdge) & (1 << i)) != 0)
                    hoverAlpha[i] = std::min(1.0f, hoverAlpha[i] + deltaTime * speed);
                else
                    hoverAlpha[i] = std::max(0.0f, hoverAlpha[i] - deltaTime * speed);

                if (hoverAlpha[i] > 0.0f && hoverAlpha[i] < 1.0f)
                    anyActive = true;
            }

            repaint();
            if (!anyActive)
                stopFrameTimer();
        });
    }

    void render(NVGcontext* vg, const pptk::Theme& theme) override
    {
        const float w = getWidth();
        const float h = getHeight();
        const float t = 4.0f;

        const NVGcolor base = theme.app.general_accent;

        auto drawEdge = [&](Edge edge, float alpha) {
            if (alpha <= 0.0f) return;
            NVGcolor col = base;
            col.a *= alpha * 0.5f;

            switch (edge)
            {
            case Edge::Top:
                nvgDrawRoundedRect(vg, margin, 0, w - margin * 2, t, col, col, 3.0f);
                break;
            case Edge::Bottom:
                nvgDrawRoundedRect(vg, margin, h - t, w - margin * 2, t, col, col, 3.0f);
                break;
            case Edge::Left:
                nvgDrawRoundedRect(vg, 0, margin, t, h - margin * 2, col, col, 3.0f);
                break;
            case Edge::Right:
                nvgDrawRoundedRect(vg, w - t, margin, t, h - margin * 2, col, col, 3.0f);
                break;
            default:
                break;
            }
        };

        drawEdge(Edge::Right,   hoverAlpha[1]);
        drawEdge(Edge::Left,    hoverAlpha[0]);
        drawEdge(Edge::Top,  hoverAlpha[2]);
        drawEdge(Edge::Bottom, hoverAlpha[3]);
    }

    void mouseButtonDown(pptk::CompEvent& e) override
    {
        dragStartGlobal = localToGlobal(e.sdlEvent.button.x, e.sdlEvent.button.y);
        // use SDL_GetMouseState() or your engine's helper
        startBounds = getParent()->getBounds();
    }

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point&, pptk::Button) override
    {
        resizingActive = true;

        pptk::Point currentGlobal = localToGlobal(currentPosition.x, currentPosition.y);
        auto deltaPos = currentGlobal - dragStartGlobal;

        auto canvasScale = findParentOfClass<Canvas>()->scale;

        deltaPos.x /= canvasScale;
        deltaPos.y /= canvasScale;

        // Pass the delta to the resize handler
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

    float getMargin() const
    {
        return margin;
    }

    void setMargin(const float m)
    {
        margin = m;
    }

private:
    float aspectRatio = 0.0f; // 0 = no constraint

    bool resizingActive = false;

    bool isHovered = false;

    float hoverAlpha[4] = { 0.0f };

    pptk::Point dragStartGlobal;

    float margin = 8;

    Edge hoveredEdge = Edge::None;

    pptk::Point dragStart;
    pptk::Rect startBounds;
    pptk::Point currentMousePos;
    static constexpr int edgeThickness = 6;

    void updateCursor()
    {
        //std::cout << "hovered edge: " << edgeToString(hoveredEdge) << std::endl;
        SDL_SetCursor(SDL_CreateSystemCursor(edgeToCursor(hoveredEdge)));
        repaint();
    }

    bool hitTest(float px, float py) override
    {
        const auto w = getWidth();
        const auto h = getHeight();

        auto hit = hitTestEdge({px, py});

        bool inside = px >= 0 && px <= w &&
            py >= 0 && py <= h;

        //std::cout << "[hitTest]: " << inside << " hit edge: " << edgeToString(hit) << std::endl;

        return inside && hit != Edge::None;
    }

    Edge hitTestEdge(const pptk::Point pt) const
    {
        const auto w = getWidth();
        const auto h = getHeight();

        //std::cout << "[hitTestEdge] pt.x: " << pt.x << ", pt.y: " << pt.y
       //    << " | width: " << w << ", height: " << h << std::endl;

        int edge = static_cast<int>(Edge::None);
        if (pt.x < edgeThickness)
            edge |= static_cast<int>(Edge::Left);
        else if (pt.x > w - edgeThickness)
            edge |= static_cast<int>(Edge::Right);
        if (pt.y < edgeThickness)
            edge |= static_cast<int>(Edge::Top);
        else if (pt.y > h - edgeThickness)
            edge |= static_cast<int>(Edge::Bottom);

        return static_cast<Edge>(edge);
    }

    // FIXME: NOT HAPPY about doing this at all! We need to work out a better way to unify all of the bounds
    [[nodiscard]] pptk::Rect getLocalBounds() const override
    {
        return pptk::Rect(
            -edgeThickness,
            -edgeThickness,
            getWidth() + edgeThickness * 2,
            getHeight() + edgeThickness * 2
        );
    }

    SDL_SystemCursor edgeToCursor(Edge e)
    {
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

    static const char* edgeToString(Edge e)
    {
        static thread_local std::string str;
        str.clear();

        if (e == Edge::None) return "None";
        if (static_cast<int>(e) & static_cast<int>(Edge::Left)) str += "Left ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Right)) str += "Right ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Top)) str += "Top ";
        if (static_cast<int>(e) & static_cast<int>(Edge::Bottom)) str += "Bottom ";

        return str.c_str();
    }
};
