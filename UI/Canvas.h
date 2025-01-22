#pragma once

#include "../UI_Toolkit/Component.h"

class Object;
class Canvas : public pptk::Component {
public:
    Canvas();

    void render(NVGcontext* nvg) override;

private:
    std::vector<Object*> objects;
};