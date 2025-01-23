//
// Created by alexw on 23/01/2025.
//

#pragma once

#include <iostream>
#include "../UI_ToolKit/Component.h"
#include "Port.h"

class Connection : public pptk::Component {
public:
    Connection(Component* parent);

    void setConnectionDest(const pptk::Point& p)
    {
        dest = p;
    }

    void render(NVGcontext* nvg) override;
private:
    pptk::Point dest;
    Port* originPort = nullptr;
};