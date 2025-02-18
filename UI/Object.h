/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "CanvasItem.h"

#include "Editor.h"
#include "Port.h"

#include "../Glad/gl.h"

class AudioNode;
class Canvas;
class Object : public CanvasItem {
public:
    explicit Object(AudioNode* node);

    // Make a non-functioning UI only object (only for testing)
    explicit Object(const std::string& name);

    ~Object() override;

    void mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button) override;
    void mouseButtonDown(pptk::CompEvent& e) override;
    void mouseEnter(pptk::CompEvent& e) override;
    void mouseLeave(pptk::CompEvent& e) override;
    void keyPressed(pptk::CompEvent& e) override;

    void resized() override;

    void setGuiIsTransparent(bool isTransparent);
    void render(NVGcontext* nvg) override;
    void drawBackground(NVGcontext* nvg);
    virtual void drawGUI(NVGcontext* nvg);

    [[nodiscard]] const std::string& getName() const { return name; }

    [[nodiscard]] uint8_t getNumInputs() const { return inPorts.size(); };

    [[nodiscard]] uint8_t getNumOutputs() const { return outPorts.size(); };

    json getObjectDefinition()
    {
        return definition;
    };

    bool getIsHovered() const { return isHovered; };

    virtual void updateGraphValues() { };

    void setObjectDefinition(const json& j)
    {
        definition = j;
    }

    int nodeID = -1;

    AudioNode* audioNode = nullptr;

private:
    bool useDefaultUI = true;
    std::string shortName;
    std::string name;
    json definition;
    std::vector<std::unique_ptr<Port>> inPorts;
    std::vector<std::unique_ptr<Port>> outPorts;

    class InsetParameter;
    std::vector<std::unique_ptr<InsetParameter>> insetParameters;

    bool isHovered = false;

    friend class Canvas;

    bool multiSelected = false;

    float nameWidth = -1.0f;

    bool isGuiTransparent = false;
};

