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

#include "Object.h"
#include "Connection.h"
#include "Lasso.h"
#include "CanvasItem.h"
#include "../Graph/AudioGraph.h"
#include "../Graph/Edge.h"

Canvas::Canvas(GraphManager* gm) : graphManager(gm)
{
#ifdef GENERATE_TEST_OBJECTS
    for (int i = 0; i < 1000; ++i)
    {
        auto obj = std::make_unique<Object>("test_obj " + std::to_string(i));
        addComponent(obj.get());
        objects.push_back(obj.get());
        testObjects.push_back(std::move(obj));
    }

    for (const auto& obj : objects)
    {
        obj->setPosition((std::rand() % 800) + canvasOrigin, (std::rand() % 800) + canvasOrigin);
    }
#endif

    objectsLayer.setSize(infinteCanvasSize, infinteCanvasSize);
    connectionsLayer.setSize(infinteCanvasSize, infinteCanvasSize);

    addComponent(&objectsLayer);
    addComponent(&connectionsLayer);

    objectsLayer.setInterceptsMouseClicks(false, true);
    connectionsLayer.setInterceptsMouseClicks(false, true);

    setWantsFocus(true);
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
    std::cout << "resizing canvas" << std::endl;
}

std::vector<Object*> Canvas::getSelectedObjects() const
{
    std::vector<Object*> selObjecst;
    selObjecst.reserve(selected.size());

    for (auto& canvasItem : selected)
    {
        if (auto obj = dynamic_cast<Object*>(canvasItem))
        selObjecst.push_back(obj);
    }

    return selObjecst;
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
    if (e.sdlEvent.button.button == SDL_BUTTON_LEFT)
    {
        clearSelection();

        lasso = std::make_unique<Lasso>(pptk::Point(e.sdlEvent.button.x, e.sdlEvent.button.y));   //lasso->start({e.button.x, e.button.y});
        addComponent(lasso.get());
    }
}

void Canvas::mouseButtonUp(pptk::CompEvent& e)
{
    SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT));
    lasso.reset();
}

void Canvas::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button)
{
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
    std::cout << "focusGained" << std::endl;
}

void Canvas::focusLost()
{
    std::cout << "focusLost" << std::endl;
}

void Canvas::mouseWheel(pptk::CompEvent& e)
{
    float mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Translate mouse position to canvas coordinates
    float canvasMouseX = (mouseX - x) / scale;
    float canvasMouseY = (mouseY - y) / scale;

    // Use logarithmic scaling so scaling feels consistent to user
    // ie: It doesn't feel like scaling > 1.0f is slow, while < 1.0f is super fast
    float logScale = std::log(scale);
    float logDelta = e.sdlEvent.wheel.y * 0.3f;
    float newScale = std::exp(logScale + logDelta);

    // Clamp the new scale between 0.1f and 3.0f
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);

    // If scrolling crosses 1.0 from either side, snap exactly to 1.0f
    // We use this to always allow users to scroll to 100% regardless of the last increment
    if ((scale < 1.0f && newScale > 1.0f) || (scale > 1.0f && newScale < 1.0f))
    {
        newScale = 1.0f;
    }

    // Apply the new scale
    if (scale != newScale)
    {
        // Adjust canvas offset to scale around the mouse point
        x -= canvasMouseX * (newScale - scale);
        y -= canvasMouseY * (newScale - scale);

        // TODO: Canvas should be in a viewport! We will not need to keep both x and offset then!
        canvasOffset.x = x + canvasOrigin * newScale;
        canvasOffset.y = y + canvasOrigin * newScale;

        scale = newScale;
        onScaleChange(scale);
        repaint();
        frameBufferRepaint = true;
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
    auto modKey = e.sdlEvent.key.mod;
    bool ctrlPressed = (modKey & SDL_KMOD_CTRL) != 0;
    bool onlyCtrl = (modKey & ~SDL_KMOD_CTRL) == 0;
    if (ctrlPressed && onlyCtrl)
    {
        if (e.sdlEvent.key.scancode == SDL_SCANCODE_C)
        {
            copySelectionToClipboard();
        }
    } else if(e.sdlEvent.key.key == SDLK_DELETE || e.sdlEvent.key.key == SDLK_BACKSPACE)
    {
        deleteSelectedObjects();
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

Port* Canvas::findPort(int x, int y)
{
    return dynamic_cast<Port*>(objectsLayer.findComponentAt(x, y));
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
            std::cout << "deleting object which is called: " << objPtr->getName() << std::endl;
            idsToDelete.push_back(objPtr->nodeID);
            objPtr->audioNode->destroyUI();
        } else if (auto* connPtr = dynamic_cast<Connection*>(obj))
        {
            edgeHashToDelete.push_back(connPtr->getEdgeHash());
        }
    }

    auto graphManager = reinterpret_cast<Editor*>(getRootComponent())->graphManager;

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

    for (auto& cnvItem : selected)
    {
        auto newPosition = cnvItem->getPosition() + pos;
        cnvItem->setPosition(newPosition);
        if (auto* objObject = dynamic_cast<Object*>(cnvItem))
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


void Canvas::render(NVGcontext* nvg)
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
        nvgFillColor(nvg, nvgRGBA(20, 20, 20, 255));
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
        nvgStrokeColor(nvg, nvgRGBA(26, 26, 26, 255)); // Lighter gray for major grid
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
void Canvas::renderAll(NVGcontext* nvg)
{
    nvgSave(nvg);

    nvgTranslate(nvg, x, y);
    nvgScale(nvg, scale, scale);

    // Render the background
    render(nvg);

    // TODO: as we are ALSO dealing with this at the component level, don't ALSO do it here!
    if (mode == Canvas::DisplayMode::Edit)
    {
        renderAllObjects(nvg);
        renderAllConnections(nvg);
    } else if (mode == Canvas::DisplayMode::Lock)
    {
        renderAllConnections(nvg);
        renderAllObjects(nvg);
    }

    if (newConnections.size() > 0)
    {
        for (auto& conn : newConnections)
        {
            nvgSave(nvg);
            nvgTranslate(nvg, conn->getX(), conn->getY());

            conn->render(nvg);

            nvgRestore(nvg);
        }
    }

    if (lasso)
    {
        nvgSave(nvg);
        nvgTranslate(nvg, lasso->getX(), lasso->getY());

        lasso->render(nvg);

        nvgRestore(nvg);
    }

    // Restore previous transformation
    nvgRestore(nvg);
}

void Canvas::renderAllObjects(NVGcontext* nvg)
{
    for (auto const& obj : objects)
    {
        // Objects are widgets that have children
        // So we need to render child components
        obj->renderAll(nvg);
    }
}

void Canvas::renderAllConnections(NVGcontext* nvg)
{
    for (auto const& con : connections)
    {
        // Connections have no child components
        nvgSave(nvg);
        nvgTranslate(nvg, con->getX(), con->getY());

        con->render(nvg);

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

    callObjectChangedListeners();

    gainFocus();
}

void Canvas::addObject(Object* toAdd, pptk::Point position)
{
    std::cout << "adding object into graph: " << toAdd->getObjectDefinition() << std::endl;

    auto audioObject = graphManager->addObject(toAdd->getObjectDefinition());

    if (audioObject == nullptr)
        return;

    auto object = audioObject->getOrCreateUI();

    objectsLayer.addComponent(object);
    object->setPosition(position);

    setSelected(object);

    objects.push_back(object);


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
        objects.push_back(obj);
        objectsLayer.addComponent(obj);
        obj->setPosition(pptk::Point(obj->audioNode->canvasPos.x + canvasOrigin, obj->audioNode->canvasPos.y + canvasOrigin));
    }

    callObjectChangedListeners();
    repaint();
}

void Canvas::reloadConnections(std::vector<Edge*> edges)
{
    std::cout << "reloading ALL connections" << std::endl;
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

        auto outputObj = reinterpret_cast<Object*>(origin->getParent());
        auto inputObj = reinterpret_cast<Object*>(dest->getParent());

        newConnections.emplace_back(outputObj->nodeID, origin->getPortNum(), inputObj->nodeID, dest->getPortNum());
    }

    auto newConnState = graphManager->connectMultiple(newConnections);

    reloadConnections(newConnState);
}

void Canvas::addConnection(Port* origin, Port* dest)
{
    if (!origin->isOutput())
        std::swap(origin, dest);

    auto outputObj = reinterpret_cast<Object*>(origin->getParent());
    auto inputObj = reinterpret_cast<Object*>(dest->getParent());

    std::cout << "Connecting from node: " << outputObj->getName() << " ID: " << outputObj->nodeID << " (port " << origin->getPortNum() << ") "
          << "to node " << inputObj->getName() << " ID: " << inputObj->nodeID << " (port " << dest->getPortNum() << ")" << std::endl;

    auto newConnState = graphManager->connect(outputObj->nodeID, origin->getPortNum(), inputObj->nodeID, dest->getPortNum());

    reloadConnections(newConnState);
}

void Canvas::setPatchName(const std::string& name)
{
    patchName = name;
    onPatchChanged();
}

void Canvas::copySelectionToClipboard()
{
    std::vector<uint32_t> selectedNodes;
    for (auto* node : selected)
    {
        if (auto* obj = dynamic_cast<Object*>(node))
            selectedNodes.push_back(obj->nodeID);
    }
    auto selectedGraph = graphManager->copySelectedToClipboard(selectedNodes);

    SDL_SetClipboardText(selectedNodes.size() ? to_string(selectedGraph).c_str() : "");
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

void Canvas::callObjectChangedListeners()
{
    for (auto& objChangeListener : objectChangedListeners)
    {
        objChangeListener();
    }
}