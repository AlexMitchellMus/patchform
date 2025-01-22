//
// Created by alexw on 23/01/2025.
//

#pragma once

class Connection : public pptk::Component {
public:
    Connection() : dest(x, y) {}

    void setConnectionDest(const pptk::Point& p) {
        std::cout << "Setting dest: " << p.toString() << std::endl;
        dest = p;
    }

    void render(NVGcontext* nvg) override {
        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x, y);
        auto gPos = getAbsolutePosition();
        nvgLineTo(nvg, dest.x - gPos.x, dest.y - gPos.y);

        nvgStrokeColor(nvg, nvgRGB(100, 100, 100)); // Set stroke color
        nvgStrokeWidth(nvg, 1.5f);   // Set line width
        nvgStroke(nvg);
    }

private:
    pptk::Point dest;
};