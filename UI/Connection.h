/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include <iostream>
#include "CanvasItem.h"
#include "Port.h"

class Connection : public CanvasItem {
public:
    Connection(Port* origin, Port* destPos = nullptr, uint64_t edgeHash = 0);

    ~Connection();

    void updateConnectionGeometry();

    void mouseEnter(pptk::CompEvent& e) override;
    void mouseLeave(pptk::CompEvent& e) override;
    void mouseButtonDown(pptk::CompEvent& e) override;
    void keyPressed(pptk::CompEvent& e) override;
    bool hitTest(float px, float py) override;

    void setConnectionDest(const pptk::Point& p);

    void render(NVGcontext* nvg) override;

    Port* getOriginPort() { return originPort.get(); };
    Port* getDestPort() { return destPort.get(); };

    uint64_t getEdgeHash() { return edgeHash; };

private:
    uint64_t edgeHash;

    static inline bool isPointNearBezier(const pptk::Point& p,
                                         const pptk::Point& start,
                                         const pptk::Point& c1,
                                         const pptk::Point& c2,
                                         const pptk::Point& end,
                                         float threshold = 5.0f);

    static inline float pointToSegmentDistance(const pptk::Point& p,
                                               const pptk::Point& a,
                                               const pptk::Point& b);

    pptk::SafePointer<Port> originPort;
    pptk::SafePointer<Port> destPort;

    pptk::Point destPos;

    pptk::Point startPoint;
    pptk::Point controlPoint1;
    pptk::Point controlPoint2;
    pptk::Point endPoint;

    bool connectionBeingCreated = false;

    NVGcolor conCol = nvgRGB(90, 90, 90);
    NVGcolor highlightCol = nvgRGB(28, 73, 119);

    bool isHovered = false;

    enum class CableType
    {
        Signal,
        Event
    };

    CableType cableType = CableType::Event;
};