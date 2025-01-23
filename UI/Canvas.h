#pragma once

#include "../UI_Toolkit/Component.h"

class Object;
class Connection;
class Canvas : public pptk::Component {
public:
    Canvas(Component* parent);

    void render(NVGcontext* nvg) override;

    void renderAll(NVGcontext* nvg) override;

    std::unique_ptr<Connection> newConnection;

private:
    std::vector<std::unique_ptr<Object>> objects;
    std::vector<std::unique_ptr<Connection>> connections;
};