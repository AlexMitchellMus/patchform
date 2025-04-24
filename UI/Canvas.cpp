/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <cstdint>
#include <sstream>
#include <unordered_map>

#ifndef GLAD_GL_H_
#include "../Glad/gl.h"
#endif

#include "Canvas.h"

#include <glaze/core/common.hpp>
#include <Graph/GraphSystem.h>

#include "Editor.h"
#include "Object.h"
#include "Connection.h"
#include "Lasso.h"
#include "CanvasItem.h"
#include "../Graph/GraphManager.h"
#include "../Graph/Edge.h"

Canvas::Canvas(GraphSystem* gm) : graphSystem(gm)
{
    objectsLayer.setSize(infinteCanvasSize, infinteCanvasSize);
    connectionsLayer.setSize(infinteCanvasSize, infinteCanvasSize);

    addComponent(&objectsLayer);
    addComponent(&connectionsLayer);

    objectsLayer.setInterceptsMouseClicks(false, true);
    connectionsLayer.setInterceptsMouseClicks(false, true);

    setWantsFocus(true);

    if (auto* activeGraph = gm->getActiveGraph())
    {
        setPatchName(activeGraph->getPatchFile());
    }
}

std::vector<Object*> Canvas::getObjects() const
{
    std::vector<Object*> objs;
    objs.reserve(objects.size());

    for (auto& obj : objects)
    {
        objs.push_back(obj);
    }

    return objs;
};

void Canvas::resized()
{
    //std::cout << "resizing canvas" << std::endl;
}

std::vector<Object*> Canvas::getSelectedObjects() const
{
    std::vector<Object*> selObjects;
    selObjects.reserve(selected.size());

    for (auto& canvasItem : selected)
    {
        if (auto obj = dynamic_cast<Object*>(canvasItem))
        selObjects.push_back(obj);
    }

    return selObjects;
}

void Canvas::updateGraphValuesIfNeeded()
{
    for (auto* obj : objects)
    {
        obj->updateGraphValues();
    }
}


void Canvas::mouseButtonDown(pptk::CompEvent& e)
{
    // TODO: Lets clear for now if clicked on empty space, however we will load canvas (patch) parameter when clicked on
    clearSelection();

    // If ctrl is down and single click on graph, toggle graph edit/lock mode
    if ((SDL_GetModState() & SDL_KMOD_CTRL) != 0 && e.sdlEvent.button.button == SDL_BUTTON_LEFT)
    {
        if (auto* ed = findParentOfClass<Editor>())
        {
            auto newState = mode == DisplayMode::Edit ? DisplayMode::Lock : DisplayMode::Edit;
            ed->updateTooldockModeButton(newState == DisplayMode::Lock);
            setMode(newState);
        }
    }
}

void Canvas::mouseButtonUp(pptk::CompEvent& e)
{
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    lasso.reset();
}

void Canvas::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button)
{
    if (!lasso && button == pptk::Button::LEFT)
    {
        lasso = std::make_unique<Lasso>(position);
        addComponent(lasso.get());
    }
    if (isDragging || button == pptk::Button::MIDDLE)
    {
        dragCanvas(delta);
    }
    else if (button == pptk::Button::LEFT)
    {
        if (lasso)
        {
            lasso->update(position);

            auto lassoBounds = lasso->getLassoBounds();

            for (const auto& obj : objects)
            {
                if (lassoBounds.intersects(obj->getBounds()))
                {
                    addToSelection(obj);
                }
                else
                {
                    removeFromSelection(obj);
                }
            }
        }
    }
}

void Canvas::dragCanvas(const pptk::Point& delta)
{
    auto scale = getAccumulatedScale();
    x += delta.x * scale;
    y += delta.y * scale;

    canvasOffset += delta * scale;

    repaint();
}

void Canvas::focusGained()
{
}

void Canvas::focusLost()
{
}

void Canvas::mouseWheel(pptk::CompEvent& e)
{
    // Get current mouse position
    SDL_GetMouseState(&zoomMouseX, &zoomMouseY);

    // Update logTarget cumulatively for smooth zoom steps
    float logDelta = e.sdlEvent.wheel.y * 0.3f;
    logTarget += logDelta;

    // Clamp log scale to avoid zoom extremes
    logTarget = std::clamp(logTarget, std::log(0.1f), std::log(3.0f));

    float currentScale = std::exp(logTarget - logDelta); // previous target before this scroll
    float newScale = std::exp(logTarget);

    // If we crossed 1.0 in either direction and the difference is small, snap
    if ((currentScale < 1.0f && newScale > 1.0f) || (currentScale > 1.0f && newScale < 1.0f)) {
        if (std::abs(newScale - 1.0f) < 0.2f)
            logTarget = 0.0f;
    }

    // Set anchor based on current scale
    zoomAnchor = {(zoomMouseX - x) / scale, (zoomMouseY - y) / scale};

    zooming = true;

    if (!frameTimerRunning) {
        frameTimerRunning = true;
        startFrameTimer([this](uint32_t, uint32_t) {
            if (!zooming) return;

            float target = std::exp(logTarget);
            float delta = target - scale;

            scale += delta * 0.25f;

            if (std::abs(delta) < 0.001f) {
                scale = target;
                zooming = false;
                frameTimerRunning = false;
                stopFrameTimer();
            }

            x = zoomMouseX - zoomAnchor.x * scale;
            y = zoomMouseY - zoomAnchor.y * scale;

            canvasOffset.x = x + canvasOrigin * scale;
            canvasOffset.y = y + canvasOrigin * scale;

            onScaleChange(scale);
            repaint();
            frameBufferRepaint = true;
        });
    }
}

void Canvas::setScale(float offset)
{
    float newScale = scale + (offset * 0.001f);
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);
    scale = newScale;

    onScaleChange(scale);
    frameBufferRepaint = true;
    repaint();
}

void Canvas::resetScale()
{
    scale = 1.0f;
    onScaleChange(scale);
    frameBufferRepaint = true;
    repaint();
}

void Canvas::keyPressed(pptk::CompEvent& e)
{
    const auto modKey = e.sdlEvent.key.mod;
    if ((modKey & SDL_KMOD_CTRL) != 0)
    {
        switch (e.sdlEvent.key.scancode)
        {
        case SDL_SCANCODE_C:
            {
                copySelectionToClipboard();
            }
            break;
        case SDL_SCANCODE_V:
            {
                pasteFromClipboard();
            }
            break;
        case SDL_SCANCODE_A:
            {
                selectAll();
            }
            break;
        case SDL_SCANCODE_D:
            {
                duplicateSelection();
            }
            break;
        case SDL_SCANCODE_E:
            {
                // Toggle canvas lock / edit mode
                if (auto* ed = findParentOfClass<Editor>())
                {
                    auto newState = mode == DisplayMode::Edit ? DisplayMode::Lock : DisplayMode::Edit;
                    ed->updateTooldockModeButton(newState == DisplayMode::Lock);
                    setMode(newState);
                }
            }
            break;
        default:
            break;
        }
    } else if(e.sdlEvent.key.key == SDLK_DELETE || e.sdlEvent.key.key == SDLK_BACKSPACE)
    {
        deleteSelectedObjects();
    }
}

void Canvas::selectAll()
{
    for (auto& obj : objects)
    {
        addToSelection(obj);
    }
}

bool Canvas::consumeEvent(pptk::CompEvent& e)
{
    const bool* keyboardState = SDL_GetKeyboardState(nullptr);
    if (e.sdlEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        // TODO: allow user to choose keyboard key for canvas drag
        if (e.sdlEvent.button.button == SDL_BUTTON_LEFT && keyboardState[SDL_SCANCODE_SPACE])
        {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));
            return isDragging = true;
        }
        if (e.sdlEvent.button.button == SDL_BUTTON_MIDDLE)
        {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE));
            return isDragging = true;
        }
    }

    return isDragging = false;
}

Port* Canvas::findPort(int x, int y, Port::Direction dragFrom)
{
    auto* comp = objectsLayer.findComponentAt(x, y);

    // We check ports first - as ports are in-front of the object layer
    if (auto* port = dynamic_cast<Port*>(comp))
        return port;

    auto* node = dynamic_cast<Object*>(comp);
    if (!node)
        return nullptr;

    const pptk::Point local = node->globalToLocal(x, y);

    if (dragFrom == Port::Direction::Output && node->getNumInputs() > 0)
    {
        float segmentWidth = node->getWidth() / node->getNumInputs();
        int index = std::clamp(static_cast<int>(local.x / segmentWidth), 0, node->getNumInputs() - 1);
        return node->getInPort(index);
    }

    if (dragFrom == Port::Direction::Input && node->getNumOutputs() > 0)
    {
        float segmentWidth = node->getWidth() / node->getNumOutputs();
        int index = std::clamp(static_cast<int>(local.x / segmentWidth), 0, node->getNumOutputs() - 1);
        return node->getOutPort(index);
    }

    return nullptr;
}


void Canvas::deleteSelectedObjects()
{
    if (isInLockedMode())
        return;

    objects.erase(std::remove_if(objects.begin(), objects.end(),
        [](const Object* obj) {
            return obj->getIsSelected(); // Only remove the objects that are currently selected
        }),
        objects.end());

    std::vector<int> idsToDelete;
    std::vector<uint64_t> edgeHashToDelete;

    for (auto* obj : selected)
    {
        if (auto* objPtr = dynamic_cast<Object*>(obj))
        {
            //std::cout << "deleting object which is called: " << objPtr->getName() << std::endl;
            idsToDelete.push_back(objPtr->nodeID);
            objPtr->audioNode->destroyUI();
        } else if (auto* connPtr = dynamic_cast<Connection*>(obj))
        {
            edgeHashToDelete.push_back(connPtr->getEdgeHash());
        }
    }

    auto graphManager = reinterpret_cast<Editor*>(getRootComponent())->graphSystem->getActiveGraph();

    auto newConnState = graphManager->removeObjects(idsToDelete, edgeHashToDelete);

    selected.clear();

    callObjectChangedListeners();

    reloadConnections(newConnState);

    repaint();
}

void Canvas::addToSelection(Object* obj)
{
    if (std::find(selected.begin(), selected.end(), obj) == selected.end()) // Avoid duplicates
    {
        obj->setSelected(true);
        selected.push_back(obj);

        callObjectChangedListeners();
    }

    repaint();
}

void Canvas::removeFromSelection(Object* obj)
{
    auto it = std::find(selected.begin(), selected.end(), obj);
    if (it != selected.end())
    {
        obj->setSelected(false);
        selected.erase(it);
        callObjectChangedListeners();
    }

    repaint();
}

bool Canvas::areMultiObjectsSelected()
{
    return selected.size() > 1;
}

void Canvas::setMultiObjectPosition(pptk::Point pos)
{
    if (isInLockedMode())
        return;

    for (const auto& cnvItem : selected)
    {
        auto newPosition = cnvItem->getPosition() + pos;
        cnvItem->setPosition(newPosition);

        if (const auto activeGraph = graphSystem->getActiveGraph())
            activeGraph->setDirty(true);

        if (const auto* objObject = dynamic_cast<Object*>(cnvItem))
        {
            objObject->audioNode->canvasPos = newPosition - pptk::Point(canvasOrigin, canvasOrigin);
        }
    }

    updateConnectionsPosition();
}

void Canvas::updateConnectionsPosition() const
{
    for (auto& con : connections)
    {
        con->updateConnectionGeometry();
    }
}

void Canvas::setSelected(CanvasItem* obj)
{
    clearSelection();

    obj->setSelected(true);

    selected.push_back(obj);

    callObjectChangedListeners();

    repaint();
}

void Canvas::clearSelection()
{
    for (auto& obj : selected)
    {
        obj->setSelected(false);
    }

    selected.clear();

    callObjectChangedListeners();

    repaint();
}

void Canvas::setMode(const DisplayMode newMode)
{
    if (mode != newMode)
    {
        mode = newMode;

        if (mode == DisplayMode::Edit)
            objectsLayer.toBack();
        else if (mode == DisplayMode::Lock)
            connectionsLayer.toBack();

        for (auto& obj : objects)
        {
            obj->updateCanvasMode(mode);
        }

        repaint();
    }
};


void Canvas::render(NVGcontext* nvg, const pptk::Theme& theme)
{
    // Draw Background color
    nvgBeginPath(nvg);
    nvgFillColor(nvg, nvgRGB(20, 20, 20));
    nvgFillRect(nvg, 0, 0, width, height);

    if (mode == Canvas::DisplayMode::Edit)
    {
        // Offset by canvasOrigin so the texture starts at canvas 0,0 point
        NVGpaint paint = nvgImagePattern(nvg, canvasOrigin, canvasOrigin, 512, 512, 0, tileFB->image, 1.0f);
        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, width, height);
        nvgFillPaint(nvg, paint);
        nvgFill(nvg);
    }

    // Draw dashed origin lines
    nvgBeginPath(nvg);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, infinteCanvasSize, canvasOrigin);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, canvasOrigin, infinteCanvasSize);

    nvgStrokeColor(nvg, nvgRGB(55, 55, 55)); // Set stroke color

    // When canvas is zoomed out (scaled smaller than 1.0f) we increase the size of the lines
    // So they don't disappear or create morie patterns
    auto scaledStroke = 2.0f / std::clamp(scale, 0.01f, 0.5f);
    nvgStrokeWidth(nvg, scaledStroke);
    nvgDashLength(nvg, 10.0f * std::clamp(scale, 0.01f, 0.5f) * 2.0f);
    nvgLineStyle(nvg, NVG_LINE_DASHED);
    nvgStroke(nvg);
}

void Canvas::updateFrameBuffer(NVGcontext* nvg)
{
    int tileSize = 1024;
    // TODO: We need to recreate when the opengl context is re-created
    if (tileFB == nullptr)
    {
        tileFB = nvgCreateFramebuffer(nvg, tileSize, tileSize, NVG_IMAGE_PREMULTIPLIED | NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY);
    }

    if (frameBufferRepaint)
    {
        frameBufferRepaint = false;

        nvgBindFramebuffer(tileFB); // Render to the tile framebuffer
        nvgViewport(0, 0, tileSize, tileSize);
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        nvgBeginFrame(nvg, tileSize, tileSize, 1.0f);
        nvgScissor(nvg, 0, 0, tileSize, tileSize);

        nvgBeginPath(nvg);
        nvgRect(nvg, 0, 0, tileSize, tileSize);
        nvgFillColor(nvg, nvgRGBA(18, 18, 18, 255));
        nvgFill(nvg);

        nvgBeginPath(nvg);
        // Grid spacing setup
        int majorSpacing = tileSize / 4;  // 4x4 major grid
        int minorSpacing = tileSize / 16; // Inner thin grid (4x4 within each major block)

        // When canvas is zoomed out (scaled smaller than 1.0f) we increase the size of the lines
        // So they don't disappear or create morie patterns
        auto scaledStroke = 2.0f / std::clamp(scale, 0.01f, 0.5f);

        // Draw minor grid lines
        nvgStrokeColor(nvg, nvgRGBA(22, 22, 22, 255));
        nvgStrokeWidth(nvg, scaledStroke);
        nvgBeginPath(nvg);
        for (int x = 0; x <= tileSize; x += minorSpacing) {
            nvgMoveTo(nvg, x, 0);
            nvgLineTo(nvg, x, tileSize);
        }
        for (int y = 0; y <= tileSize; y += minorSpacing) {
            nvgMoveTo(nvg, 0, y);
            nvgLineTo(nvg, tileSize, y);
        }
        nvgStroke(nvg);

        // Draw major grid lines (thicker)
        nvgStrokeColor(nvg, nvgRGBA(30, 30, 30, 255)); // Lighter gray for major grid
        nvgStrokeWidth(nvg, scaledStroke);
        nvgBeginPath(nvg);
        for (int x = 0; x <= tileSize; x += majorSpacing) {
            nvgMoveTo(nvg, x, 0);
            nvgLineTo(nvg, x, tileSize);
        }
        for (int y = 0; y <= tileSize; y += majorSpacing) {
            nvgMoveTo(nvg, 0, y);
            nvgLineTo(nvg, tileSize, y);
        }
        nvgStroke(nvg);

        nvgGlobalScissor(nvg, 0, 0, tileSize, tileSize);
        nvgEndFrame(nvg);
        nvgBindFramebuffer(nullptr);
    }
}

// TODO: We don't need to do this if we deal with it at the component level- remove soon!
void Canvas::renderAll(NVGcontext* nvg, const pptk::Theme& theme)
{
    nvgSave(nvg);

    nvgTranslate(nvg, x, y);
    nvgScale(nvg, scale, scale);

    // Render the background
    render(nvg, theme);

    // TODO: as we are ALSO dealing with this at the component level, don't ALSO do it here!
    if (mode == Canvas::DisplayMode::Edit)
    {
        renderAllObjects(nvg, theme);
        renderAllConnections(nvg, theme);
    } else if (mode == Canvas::DisplayMode::Lock)
    {
        renderAllConnections(nvg, theme);
        renderAllObjects(nvg, theme);
    }

    if (newConnections.size() > 0)
    {
        for (auto& conn : newConnections)
        {
            nvgSave(nvg);
            nvgTranslate(nvg, conn->getX(), conn->getY());

            conn->render(nvg, theme);

            nvgRestore(nvg);
        }
    }

    if (lasso)
    {
        nvgSave(nvg);
        nvgTranslate(nvg, lasso->getX(), lasso->getY());

        lasso->render(nvg, theme);

        nvgRestore(nvg);
    }

    // Restore previous transformation
    nvgRestore(nvg);
}

void Canvas::renderAllObjects(NVGcontext* nvg, const pptk::Theme& theme)
{
    for (auto const& obj : objects)
    {
        // Objects are widgets that have children
        // So we need to render child components
        obj->renderAll(nvg, theme);
    }
}

void Canvas::renderAllConnections(NVGcontext* nvg, const pptk::Theme& theme)
{
    for (auto const& con : connections)
    {
        // Connections have no child components
        nvgSave(nvg);
        nvgTranslate(nvg, con->getX(), con->getY());

        con->render(nvg, theme);

        nvgRestore(nvg);
    }
}

// This uses the DnD object that has already been constructed, so no need to add it to the audio engine, as it's already there.
void Canvas::addFromDnDMenu(Object* toAdd, pptk::Point position)
{
    if (toAdd == nullptr)
        return;

    // DnD objects were originally semi-transparent, reset that here
    toAdd->opacity = 1.0f;
    // Reset the scale to 1, as the canvas itself now takes care of the object's scale!
    toAdd->scale = 1.0f;

    objectsLayer.addComponent(toAdd);
    toAdd->setPosition(position);
    toAdd->audioNode->canvasPos = position - pptk::Point(canvasOrigin, canvasOrigin);

    setSelected(toAdd);

    objects.push_back(toAdd);

    toAdd->updateCanvasMode(mode);

    callObjectChangedListeners();

    gainFocus();
}

void Canvas::addObject(Object* toAdd, pptk::Point position)
{
    std::cout << "adding object into graph: " << toAdd->getObjectDefinition() << std::endl;

    if (!graphSystem->getActiveGraph())
        return;

    auto audioObject = graphSystem->getActiveGraph()->addObject(toAdd->getObjectDefinition());

    if (audioObject == nullptr)
        return;

    auto object = audioObject->getOrCreateUI();

    objectsLayer.addComponent(object);
    object->setPosition(position);

    setSelected(object);

    objects.push_back(object);

    object->updateCanvasMode(mode);

    callObjectChangedListeners();
}

void Canvas::reloadAllCanvasObjects(std::vector<Object*> newObjects)
{
    // Clear both objects and selected as selected could contain (if they were selected) dead objects
    // TODO: We could make selected objects SafePointers but ATM we can manage their lifetime
    for (auto* obj : objects)
    {
        obj->audioNode->destroyUI();
    }
    objects.clear();
    selected.clear();

    for (auto* obj : newObjects)
    {
        obj->updateCanvasMode(mode);
        objects.push_back(obj);
        objectsLayer.addComponent(obj);
        obj->setPosition(pptk::Point(obj->audioNode->canvasPos.x + canvasOrigin, obj->audioNode->canvasPos.y + canvasOrigin));
    }

    callObjectChangedListeners();
    repaint();
}

void Canvas::reloadConnections(std::vector<Edge*>& edges)
{
    connections.clear();

    // Optionally, build a mapping for faster lookup:
    // std::unordered_map<int, Object*> objectMap;
    // for (auto* obj : objects)
    //     objectMap[obj->nodeID] = obj;

    for (auto* edge : edges)
    {
        //std::cout << "Processing edge: " << edge->toString() << std::endl;

        // Look up the output object
        auto outputObjIt = std::find_if(objects.begin(), objects.end(),
            [edge](const Object* obj) {
                return obj->nodeID == edge->getoNode();
            });

        // Look up the input object
        auto inputObjIt = std::find_if(objects.begin(), objects.end(),
            [edge](const Object* obj) {
                return obj->nodeID == edge->getiNode();
            });

        if (outputObjIt == objects.end())
        {
            //std::cerr << "Warning: Output object with nodeID " << edge->getoNode() << " not found." << std::endl;
            continue;
        }

        if (inputObjIt == objects.end())
        {
            //std::cerr << "Warning: Input object with nodeID " << edge->getiNode() << " not found." << std::endl;
            continue;
        }

        Object* outputObj = *outputObjIt;
        Object* inputObj = *inputObjIt;

        //std::cout << "Found output object (nodeID " << outputObj->nodeID << ") and input object (nodeID " << inputObj->nodeID << ")." << std::endl;

        // Validate that the port indices are within bounds.
        if (edge->getoPort() < outputObj->outPorts.size() &&
            edge->getiPort() < inputObj->inPorts.size())
        {
            auto* origin = outputObj->outPorts[edge->getoPort()].get();
            auto* dest = inputObj->inPorts[edge->getiPort()].get();

            auto connection = std::make_unique<Connection>(origin, dest, edge->getHash());

            connectionsLayer.addComponent(connection.get());
            connection->updateConnectionGeometry();

            connections.push_back(std::move(connection));
        }
        else
        {
            //std::cerr << "Port index out of range for edge: " << edge->toString() << std::endl;
        }
    }
}
void Canvas::addMultipleConnections(std::vector<std::tuple<Port*, Port*>> connections)
{
    std::vector<std::tuple<int, int, int, int>> newConnections;

    for (auto& [origin, dest] : connections)
    {
        if (!origin->isOutput())
            std::swap(origin, dest);

        auto* outputObj = reinterpret_cast<Object*>(origin->getParent());
        auto* inputObj = reinterpret_cast<Object*>(dest->getParent());

        if (!outputObj || !inputObj) {
            std::cerr << "Error: Null object found while adding connection." << std::endl;
            continue;
        }

        // Ensure nodeID is used, not nodeIDString
        const int outputNodeID = outputObj->nodeID;
        const int inputNodeID = inputObj->nodeID;

        std::cout << "Connecting from node: " << outputObj->getName() << " ID: " << outputNodeID << " (port " << origin->getPortNum() << ") "
                  << "to node " << inputObj->getName() << " ID: " << inputNodeID << " (port " << dest->getPortNum() << ")" << std::endl;

        newConnections.emplace_back(outputNodeID, origin->getPortNum(), inputNodeID, dest->getPortNum());
    }

    auto newConnState = graphSystem->getActiveGraph()->connectMultiple(newConnections);

    reloadConnections(newConnState);
}

void Canvas::setPatchName(const std::string& name)
{
    patchName = name;
    onPatchChanged();
}

void Canvas::pasteFromClipboard()
{
    char* clipboardText = SDL_GetClipboardText();
    if (!clipboardText || clipboardText[0] == '\0')
    {
        SDL_free(clipboardText);
        return;
    }

    try {
        auto clipboardGraph = json::parse(clipboardText);

        auto [ pastedObjects, allObjects, allConnections ] = graphSystem->getActiveGraph()->pasteGraph(clipboardGraph);

        pptk::Point mousePos = getMousePositionOnCanvas();

        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        for (auto* obj : pastedObjects)
        {
            minX = std::min(minX, obj->audioNode->canvasPos.x);
            minY = std::min(minY, obj->audioNode->canvasPos.y);
        }

        float alignOffsetX = mousePos.x - minX;
        float alignOffsetY = mousePos.y - minY;

        for (auto* obj : pastedObjects)
        {
            obj->updateCanvasMode(mode);
            objects.push_back(obj);
            objectsLayer.addComponent(obj);

            pptk::Point newPos(
                obj->audioNode->canvasPos.x + canvasOrigin + alignOffsetX,
                obj->audioNode->canvasPos.y + canvasOrigin + alignOffsetY
            );
            obj->setPosition(newPos);
            // We need to also set objects new canvas position here (canvasOrigin is zero)
            obj->audioNode->canvasPos = newPos - canvasOrigin;
        }

        // Update selection: clear current selection and select the newly pasted objects.
        clearSelection();
        for (auto* obj : pastedObjects)
        {
            addToSelection(obj);
        }

        reloadConnections(allConnections);
        callObjectChangedListeners();
        repaint();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error while pasting from clipboard: " << e.what() << std::endl;
    }

    SDL_free(clipboardText);
}

void Canvas::copySelectionToClipboard() const
{
    std::vector<uint32_t> selectedNodes;
    for (auto* node : selected)
    {
        if (auto* obj = dynamic_cast<Object*>(node))
            selectedNodes.push_back(obj->nodeID);
    }
    auto selectedGraph = graphSystem->getActiveGraph()->copySelected(selectedNodes);

    SDL_SetClipboardText(selectedNodes.size() ? to_string(selectedGraph).c_str() : "");
}

void Canvas::duplicateSelection()
{
    std::vector<uint32_t> selectedNodes;
    for (auto* node : selected)
    {
        if (auto* obj = dynamic_cast<Object*>(node))
            selectedNodes.push_back(obj->nodeID);
    }

    if (selectedNodes.empty())
        return;

    auto clipboardGraph = graphSystem->getActiveGraph()->copySelected(selectedNodes);
    auto [ duplicatedObjects, allObjects, allConnections ] = graphSystem->getActiveGraph()->pasteGraph(clipboardGraph);

    const float offset = 20.0f;

    for (auto* obj : duplicatedObjects)
    {
        obj->updateCanvasMode(mode);
        objects.push_back(obj);
        objectsLayer.addComponent(obj);

        pptk::Point newPos(
            obj->audioNode->canvasPos.x + canvasOrigin + offset,
            obj->audioNode->canvasPos.y + canvasOrigin + offset
        );
        obj->setPosition(newPos);
        obj->audioNode->canvasPos = newPos - canvasOrigin;
    }

    clearSelection();
    for (auto* obj : duplicatedObjects)
    {
        addToSelection(obj);
    }

    reloadConnections(allConnections);
    callObjectChangedListeners();
    repaint();
}

void Canvas::addObjectChangedListener(std::function<void()> callback)
{
    objectChangedListeners.push_back(std::move(callback));
}

void Canvas::removeObjectChangedListener(std::function<void()> callback)
{
    auto it = std::find_if(objectChangedListeners.begin(), objectChangedListeners.end(),
        [&callback](const std::function<void()>& listener) {
            return listener.target_type() == callback.target_type();
        });

    if (it != objectChangedListeners.end()) {
        objectChangedListeners.erase(it);
    }
}

pptk::Point Canvas::getMousePositionOnCanvas()
{
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    float scale = getAccumulatedScale();

    pptk::Point canvasPos;
    canvasPos.x = (mouseX - canvasOffset.x) / scale;
    canvasPos.y = (mouseY - canvasOffset.y) / scale;

    return canvasPos;
}

void Canvas::callObjectChangedListeners()
{
    for (auto& objChangeListener : objectChangedListeners)
    {
        objChangeListener();
    }
}