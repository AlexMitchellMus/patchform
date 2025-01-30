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
    Connection(Port* origin, Port* destPos = nullptr);

    ~Connection();

    void updateConnectionGeometry();

    void setConnectionDest(const pptk::Point& p);

    void render(NVGcontext* nvg) override;

    Port* getOriginPort() { return originPort; };
    Port* getDestPort() { return destPort; };

private:
    Port* originPort = nullptr;
    Port* destPort = nullptr;

    pptk::Point destPos;

    bool connectionBeingCreated = false;
};