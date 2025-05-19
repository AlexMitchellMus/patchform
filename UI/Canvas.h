/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_Toolkit/Component.h"

#include "Connection.h"

#include <NanoVGWrapper.h>

class Lasso;
class Object;
class Port;
class CanvasItem;
class GraphSystem;
class GraphManager;
class CanvasInteractionLayer;
class Edge;

//#define GENERATE_TEST_OBJECTS

class Canvas : public pptk::Component {
public:
    enum class DisplayMode { Edit, Lock };

    std::function<void(float)> onScaleChange = [](float){};
    std::function<void()> onPatchChanged = [](){};

    using ObjectChangedListeners = std::vector<std::function<void()>>;

    Canvas(GraphSystem* gm);

    ~Canvas() override = default;

    std::vector<Object*> getObjects() const;
    std::vector<Object*> getSelectedObjects() const;

    void mouseButtonDown(pptk::CompEvent& e) override;
    void mouseButtonUp(pptk::CompEvent& e) override;
    void mouseDrag(const pptk::Point& position, const pptk::Point& delta, const pptk::Button button) override;
    void mouseWheel(pptk::CompEvent& e) override;
    void keyPressed(pptk::CompEvent& e) override;
    void gesture(GestureEvent& e) override;
    void focusLost() override;
    void focusGained() override;

    bool consumeEvent(pptk::CompEvent& e) override;

    void deleteSelectedObjects();
    void setSelected(CanvasItem* obj);
    bool areMultiObjectsSelected();
    void setMultiObjectPosition(pptk::Point pos);
    void addToSelection(Object* obj);
    void removeFromSelection(Object* obj);

    void render(NVGcontext* nvg, const pptk::Theme& theme) override;

    void loadGraph(GraphManager* newGraph);

    void addObject(Object* object, pptk::Point position = pptk::Point(canvasOrigin, canvasOrigin));
    void reloadAllCanvasObjects(std::vector<Object*> objects);
    void addFromDnDMenu(Object* object, pptk::Point position = pptk::Point(canvasOrigin, canvasOrigin));
    void reloadConnections(const std::vector<Edge*>& edges);

    void copySelectionToClipboard() const;
    void pasteFromClipboard();
    void duplicateSelection();

    void selectAll();

    void setPatchName(const std::string& name);
    const std::string& getPatchName() { return patchName; };

    void updateGraphValuesIfNeeded();

    std::vector<std::unique_ptr<Connection>> newConnections;
    void addMultipleConnections(std::vector<std::tuple<Port*, Port*>> portConns);

    void updateConnectionsPosition() const;

    void addObjectChangedListener(std::function<void()> callback);
    void removeObjectChangedListener(std::function<void()> callback);
    void callObjectChangedListeners();

    void setScale(float scale);
    void resetScale();

    void setMode(DisplayMode newMode);

    bool isInEditMode() const { return mode == DisplayMode::Edit; };
    bool isInLockedMode() const { return mode == DisplayMode::Lock; };

    static constexpr int infinteCanvasSize = 120000;
    static constexpr int canvasOrigin = 64000;

    // Non graphical components that hold objects/connections
    // This is so we can move the connections infront/below objects
    Component objectsLayer;
    Component connectionsLayer;

    void updateFrameBuffer(NVGcontext* nvg);

    pptk::Point canvasOffset;

    Port* findPort(int x, int y, Port::Direction direction);

private:
    float targetScale = 1.0f;
    bool zooming = false;
    pptk::Point zoomAnchor = {0, 0};
    bool frameTimerRunning = false;
    float zoomMouseX;
    float zoomMouseY;
    float logTarget = std::log(1.0f);

    void dragCanvas(const pptk::Point&);
    pptk::Point getMousePositionOnCanvas();

    // TODO: Move graph manager outside of canvas!
    GraphSystem* graphSystem;

    void resized() override;

    std::vector<Object*> objects;
    std::vector<std::unique_ptr<Connection>> connections;
    std::vector<CanvasItem*> selected;

    ObjectChangedListeners objectChangedListeners;

    std::unique_ptr<Lasso> lasso;

    DisplayMode mode = DisplayMode::Edit;

    bool inDragMode = false;

    std::string patchName;

    NVGLUframebuffer* tileFB = nullptr;
    bool frameBufferRepaint = true;

    void clearSelection();
};