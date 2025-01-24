#pragma once

#include "../UI_Toolkit/Component.h"

class Lasso;
class Object;
class Connection;

class Canvas : public pptk::Component {
public:
    Canvas(Component* parent);

    const std::vector<Object*> getObjects() const
    {
        std::vector<Object*> objs;
        objs.reserve(objects.size());

        for (auto& obj : objects)
        {
            objs.push_back(obj.get());
        }

        return objs;
    };

    void mouseButtonDown(SDL_Event& e) override;

    void mouseButtonUp(SDL_Event& e) override;

    void mouseDrag(const pptk::Point& position, const pptk::Point& delta) override;

    void setSelected(Object* obj);

    bool areMultiObjectsSelected();

    void setMultiObjectPosition(pptk::Point pos);

    void addToSelection(Object* obj);

    void removeFromSelection(Object* obj);

    void render(NVGcontext* nvg) override;

    void renderAll(NVGcontext* nvg) override;

    std::unique_ptr<Connection> newConnection;

private:
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Connection>> connections;

    std::vector<Object*> selected;

    std::unique_ptr<Lasso> lasso;

    void clearSelection();
};