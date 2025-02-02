/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_Toolkit/Component.h"

#include "Connection.h"

class Lasso;
class Object;
class Port;
class CanvasItem;
class GraphManager;

//#define GENERATE_TEST_OBJECTS

class Canvas : public pptk::Component {
public:
    enum class DisplayMode { Edit, Lock };

    std::function<void(float)> onScaleChange = [](float){};

    using ObjectChangedListeners = std::vector<std::function<void()>>;

    Canvas(GraphManager* gm);

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
    void addFromDnDMenu(Object* object, pptk::Point position = pptk::Point(canvasOrigin, canvasOrigin));

    std::unique_ptr<Connection> newConnection = nullptr;
    void addConnection(Port* port, Port* otherPort);
    void updateConnectionsPosition() const;
    void removeConnectionsFor(Object*);

    void addObjectChangedListener(std::function<void()> callback);
    void removeObjectChangedListener(std::function<void()> callback);
    void callOjbectChangedListeners();

    void setScale(float scale);
    void resetScale();

    void setMode(DisplayMode newMode)
    {
        if (mode != newMode)
        {
            mode = newMode;
            repaint();
        }
    };

    bool isInEditMode() const { return mode == DisplayMode::Edit; };
    bool isInLockedMode() const { return mode == DisplayMode::Lock; };

    static constexpr int infinteCanvasSize = 120000;
    static constexpr int canvasOrigin = 64000;

private:
    GraphManager* graphManager;

    std::vector<Object*> objects;
    std::vector<std::unique_ptr<Connection>> connections;
    std::vector<CanvasItem*> selected;

    ObjectChangedListeners objectChangedListeners;

    std::unique_ptr<Lasso> lasso;

    DisplayMode mode = DisplayMode::Edit;

#ifdef GENERATE_TEST_OBJECTS
    // ONLY FOR TESTING! These objects are not connected to the DSP system
    std::vector<std::unique_ptr<Object>> testObjects;
#endif

    void clearSelection();
};