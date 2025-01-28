/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_Toolkit/Component.h"

class Lasso;
class Object;
class Connection;

class Canvas : public pptk::Component {
public:
    using ObjectChangedListeners = std::vector<std::function<void()>>;

    Canvas();

    std::vector<Object*> getObjects() const;

    void mouseMove(const pptk::Point& position) override
    {
    }

    void mouseButtonDown(SDL_Event& e) override;

    void mouseButtonUp(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, const pptk::Button button) override;

    void keyPressed(SDL_Event& e) override;

    void deleteSelectedObjects();

    void setSelected(Object* obj);

    bool areMultiObjectsSelected();

    void setMultiObjectPosition(pptk::Point pos);

    void addToSelection(Object* obj);

    void removeFromSelection(Object* obj);

    void render(NVGcontext* nvg) override;

    void renderAll(NVGcontext* nvg) override;

    std::unique_ptr<Connection> newConnection = nullptr;

    void addObjectChangedListener(std::function<void()> callback);

    void removeObjectChangedListener(std::function<void()> callback);

    void callOjbectChangedListeners();

    void updateLayout() override;

    const int infinteCanvasSize = 120000;
    const int canvasOrigin = 64000;

private:
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Connection>> connections;
    std::vector<Object*> selected;

    ObjectChangedListeners objectChangedListeners;

    std::unique_ptr<Lasso> lasso;

    void clearSelection();
};