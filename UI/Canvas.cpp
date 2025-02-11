/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

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

    // Adjust scale with constraints
    float newScale = scale + e.sdlEvent.wheel.y * 0.125f;
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);

    // Adjust canvas offset to scale around the mouse point
    x -= canvasMouseX * (newScale - scale);
    y -= canvasMouseY * (newScale - scale);

    // Apply the new scale
    scale = newScale;

    onScaleChange(scale);

    repaint();
}

void Canvas::setScale(float offset)
{
    float newScale = scale + (offset * 0.001f);
    newScale = std::min(std::max(newScale, 0.1f), 3.0f);
    scale = newScale;

    onScaleChange(scale);
    repaint();
}

void Canvas::resetScale()
{
    scale = 1.0f;
    onScaleChange(scale);
    repaint();
}

void Canvas::keyPressed(pptk::CompEvent& e)
{
    if (e.sdlEvent.key.key == SDLK_DELETE || e.sdlEvent.key.key == SDLK_BACKSPACE)
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

    for (auto* obj : selected)
    {
        if (auto* objPtr = dynamic_cast<Object*>(obj))
        {
            std::cout << "deleting object which is called: " << objPtr->getName() << std::endl;
            idsToDelete.push_back(objPtr->nodeID);
            removeConnectionsFor(objPtr);
            objPtr->audioNode->destroyUI();
        }
    }

    auto graphManager = reinterpret_cast<Editor*>(getRootComponent())->graphManager;

    auto newConnState = graphManager->removeObjects(idsToDelete);

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

void Canvas::removeConnectionsFor(Object* target)
{
    // TODO: Implement a SmartPointer system so we can give each object a list of connections, which will become null when removed
    auto it = connections.begin();
    while (it != connections.end())
    {
        if ((*it)->getOriginPort()->getParent() == target ||
            (*it)->getDestPort()->getParent() == target)
        {
            it = connections.erase(it); // Erases and moves iterator to next element
        }
        else
        {
            ++it;
        }
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
    nvgFillColor(nvg, nvgRGB(23, 23, 23));
    nvgFillRect(nvg, 0, 0, width, height);

    if (mode == Canvas::DisplayMode::Edit)
    {
        // Draw bg lines
        nvgBeginPath(nvg);
        nvgLineStyle(nvg, NVG_SOLID);
        nvgStrokeColor(nvg, nvgRGB(33, 33, 33)); // Set stroke color

        // Draw vertical dashed lines
        for (float x = 0; x <= infinteCanvasSize; x += 100)
        {
            nvgMoveTo(nvg, x, 0);
            nvgLineTo(nvg, x, infinteCanvasSize);
        }

        // Draw horizontal dashed lines
        for (float y = 0; y <= infinteCanvasSize; y += 100)
        {
            nvgMoveTo(nvg, 0, y);
            nvgLineTo(nvg, infinteCanvasSize, y);
        }

        nvgStroke(nvg);
    }

    // Draw dashed origin lines
    nvgBeginPath(nvg);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, infinteCanvasSize, canvasOrigin);

    nvgMoveTo(nvg, canvasOrigin, canvasOrigin);
    nvgLineTo(nvg, canvasOrigin, infinteCanvasSize);

    nvgStrokeColor(nvg, nvgRGB(43, 43, 43)); // Set stroke color
    nvgStrokeWidth(nvg, 2.0f);   // Set line width
    nvgDashLength(nvg, 10.0f);
    nvgLineStyle(nvg, NVG_LINE_DASHED);
    nvgStroke(nvg);
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

    if (newConnection)
    {
        nvgSave(nvg);
        nvgTranslate(nvg, newConnection->getX(), newConnection->getY());

        newConnection->render(nvg);

        nvgRestore(nvg);
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

            auto connection = std::make_unique<Connection>(origin, dest);

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


void Canvas::addConnection(Port* origin, Port* dest)
{
    if (!origin->isOutput())
        std::swap(origin, dest);

    auto outputObj = reinterpret_cast<Object*>(origin->getParent());
    auto inputObj = reinterpret_cast<Object*>(dest->getParent());

    std::cout << "Connecting from node: " << outputObj->getName() << " ID: " << outputObj->nodeID << " (port " << origin->getPortNum() << ") "
          << "to node " << inputObj->getName() << " ID: " << inputObj->nodeID << " (port " << dest->getPortNum() << ")" << std::endl;

    graphManager->connect(outputObj->nodeID, origin->getPortNum(), inputObj->nodeID, dest->getPortNum());

    auto connection = std::make_unique<Connection>(origin, dest);
    connectionsLayer.addComponent(connection.get());
    connection->updateConnectionGeometry();
    connections.push_back(std::move(connection));
}

void Canvas::setPatchName(const std::string& name)
{
    patchName = name;
    onPatchChanged();
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