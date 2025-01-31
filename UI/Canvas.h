/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "Port.h"
#include "../UI_Toolkit/Component.h"

class Lasso;
class Object;
class Connection;
class Port;
class CanvasItem;

class Canvas : public pptk::Component {
public:
    std::function<void(float)> onScaleChange = [](float){};

    using ObjectChangedListeners = std::vector<std::function<void()>>;

    Canvas();

    std::vector<Object*> getObjects() const;

    void mouseMove(const pptk::Point& position) override
    {
    }

    void mouseButtonDown(SDL_Event& e) override;
    void mouseButtonUp(SDL_Event& e) override;
    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, const pptk::Button button) override;
    void mouseWheel(SDL_Event& e) override;
    void keyPressed(SDL_Event& e) override;

    void deleteSelectedObjects();
    void setSelected(CanvasItem* obj);
    bool areMultiObjectsSelected();
    void setMultiObjectPosition(pptk::Point pos);
    void addToSelection(Object* obj);
    void removeFromSelection(Object* obj);

    void render(NVGcontext* nvg) override;
    void renderAll(NVGcontext* nvg) override;

    void addObject(Object* object, pptk::Point position = pptk::Point(canvasOrigin, canvasOrigin));

    std::unique_ptr<Connection> newConnection = nullptr;
    void addConnection(Port* port, Port* otherPort);
    void updateConnectionsPosition() const;
    void removeConnectionsFor(Object*);

    void addObjectChangedListener(std::function<void()> callback);
    void removeObjectChangedListener(std::function<void()> callback);
    void callOjbectChangedListeners();

    void setScale(float scale);
    void resetScale();

    static constexpr int infinteCanvasSize = 120000;
    static constexpr int canvasOrigin = 64000;

private:
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Connection>> connections;
    std::vector<CanvasItem*> selected;

    ObjectChangedListeners objectChangedListeners;

    std::unique_ptr<Lasso> lasso;

    void clearSelection();
};