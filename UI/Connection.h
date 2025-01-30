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

    Port* getOriginPort() { return originPort.get(); };
    Port* getDestPort() { return destPort.get(); };

private:
    pptk::SafePointer<Port> originPort;
    pptk::SafePointer<Port> destPort;

    pptk::Point destPos;

    bool connectionBeingCreated = false;
};