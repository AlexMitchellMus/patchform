/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include "../UI_ToolKit/Component.h"
#include "Port.h"

class Connection : public pptk::Component {
public:
    Connection(Port* port);

    void setConnectionDest(const pptk::Point& p);

    void render(NVGcontext* nvg) override;
private:
    pptk::Point dest;
    Port* originPort = nullptr;
};