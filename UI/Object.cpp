/*
// Copyright (c) 2024-2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "Canvas.h"
#include "Object.h"
#include "../Nodes/AudioNodeBase.h"

#include <glaze/reflection/get_name.hpp>

class Object::InsetParameter : public Component
{
    public:
    InsetParameter(Parameter* linkedParam)
    {
        paramDisplayText = linkedParam->getName() + " : " + linkedParam->getAsString();

        linkedParam->onParameterChanged = [this, linkedParam]()
        {
            auto newText = linkedParam->getName() + " : " + linkedParam->getAsString();
            if (paramDisplayText != newText)
            {
                paramDisplayText = newText;
                getParent()->resized();
            }
        };

    };

    bool hitTest(float x, float y) override
    {
        return false;
    };

    void render(NVGcontext* vg) override
    {
        auto col = nvgRGBA(44, 44, 44, 255);
        nvgDrawRoundedRect(vg, 0, 0, width, height, col, col, 5);

        nvgFontSize(vg, 13.0f);
        nvgFontFace(vg, "Regular");
        nvgFillColor(vg, nvgRGB(120, 120, 120));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

        nvgText(vg, width / 2, height / 2, paramDisplayText.c_str(), nullptr);
    }

    std::string paramDisplayText;
};

Object::Object(AudioNode* node)
    : nodeID(node->nodeID)
    , audioNode(node)
    , shortName(node->getShortName())
    , name(node->getName())
    , useDefaultUI(node->isDefaultUI())
{
    setBounds(0, 0, 120, 40);

    if (useDefaultUI)
    {
        for (const auto& param : node->getParameters())
        {
            insetParameters.push_back(std::make_unique<InsetParameter>(param.get()));
            addComponent(insetParameters.back().get());
        }
    }

    auto convertPortType = [](const AudioPort::PortType& type) {
        return (type == AudioPort::Signal) ? Port::PortType::Audio :
               (type == AudioPort::Data) ? Port::PortType::Event : Port::PortType::None;
    };

    for (int i = 0; i < node->inputPortBuffers.size(); ++i)
    {
        inPorts.push_back(std::make_unique<Port>(i, convertPortType(node->inputPortBuffers[i]->getPortType())));
        addComponent(inPorts.back().get());
    }

    for (int i = 0; i < node->outputPortBuffers.size(); ++i)
    {
        outPorts.push_back(std::make_unique<Port>(i, convertPortType(node->outputPortBuffers[i]->getPortType()), Port::Direction::Output));
        addComponent(outPorts.back().get());
    }

    Object::resized();
}

Object::Object(const std::string& name) : name(name)
{
    setBounds(0, 0, 120, 40);

    for (int i = 0; i < 2; ++i)
    {
        inPorts.push_back(std::make_unique<Port>(i, Port::PortType::Audio));
        addComponent(inPorts.back().get());
    }

    outPorts.push_back(std::make_unique<Port>(0, Port::PortType::Audio, Port::Direction::Output));
    addComponent(outPorts.back().get());
}

Object::~Object()
{
    //std::cout << "object deleting: " << std::endl;
}

void Object::resized()
{
    static constexpr int portDiam = 10;
    const int numInputs = static_cast<int>(inPorts.size());
    const int numOutputs = static_cast<int>(outPorts.size());
    const int maxPorts = std::max(numInputs, numOutputs);

    int insetParamOffset = 0;

    if (useDefaultUI)
    {
        nameWidth = getTextWidthForFont("Regular", 16.0f, shortName);

        for (int i = 0; i < insetParameters.size(); i++)
        {
            auto iP = insetParameters[i].get();
            constexpr int parmHeight = 16;
            iP->setBounds(nameWidth + 20 + insetParamOffset, height * 0.5f - parmHeight * 0.5f, getTextWidthForFont("Regular", 14.0f, iP->paramDisplayText) + 4, parmHeight);
            insetParamOffset += iP->getWidth() + 10;
        }

        auto currentBounds = getBounds();
        auto finalWidth = std::max(nameWidth + 20 + insetParamOffset, maxPorts * 20.0f);
        setBounds(currentBounds.x, currentBounds.y, finalWidth, getHeight());

        if (finalWidth != currentBounds.w)
        {
            if (const auto* cnv = findParentOfClass<Canvas>())
                cnv->updateConnectionsPosition();
        }
    }

    const float inputSpacing = (getWidth() - 2 - (numInputs * portDiam)) / std::max(1, numInputs - 1);
    for (int i = 0; i < inPorts.size(); ++i)
    {
        inPorts[i]->setBounds(i * (inputSpacing + portDiam) + 1, 1, portDiam, portDiam);
    }

    const float outputSpacing = (getWidth() - 2 - (numOutputs * portDiam)) / std::max(1, numOutputs - 1);
    for (int i = 0; i < outPorts.size(); ++i)
    {
        outPorts[i]->setBounds(i * (outputSpacing + portDiam) + 1, getHeight() - portDiam - 1, portDiam, portDiam);
    }

    repaint();
}

void Object::updateCanvasMode(Canvas::DisplayMode newMode)
{
    auto transparentPorts = newMode == Canvas::DisplayMode::Lock;

    for (auto& iPort : inPorts)
    {
        iPort->setCanvasMode(transparentPorts);
    }

    for (auto& oPort : outPorts)
    {
        oPort->setCanvasMode(transparentPorts);
    }

    isInLockedMode = newMode == Canvas::DisplayMode::Lock;
}

void Object::mouseEnter(pptk::CompEvent& e)
{
    isHovered = true;
    repaint();
}

void Object::mouseLeave(pptk::CompEvent& e)
{
    isHovered = false;
    repaint();
}

void Object::mouseButtonDown(pptk::CompEvent& e)
{
    if (auto cnv = findParentOfClass<Canvas>())
    {
        if (!isSelected)
        {
            cnv->setSelected(this);
            return;
        }
        multiSelected = cnv->areMultiObjectsSelected();

        if (!multiSelected)
        {
            cnv->setSelected(this);
        }
    }
}

void Object::keyPressed(pptk::CompEvent& e)
{
    if (e.sdlEvent.key.key == SDLK_DELETE || e.sdlEvent.key.key == SDLK_BACKSPACE)
    {
        if (auto cnv = findParentOfClass<Canvas>())
            cnv->deleteSelectedObjects();
    }
}

void Object::mouseDrag(const pptk::Point& currentPosition, const pptk::Point& delta, pptk::Button button)
{
    if (button == pptk::Button::LEFT)
    {
        if (auto cnv = findParentOfClass<Canvas>())
        {
            cnv->setMultiObjectPosition(delta);
        }
    }
}

void Object::setGuiIsTransparent(bool isTransparent)
{
    isGuiTransparent = isTransparent;
}


void Object::render(NVGcontext* nvg)
{
    drawBackground(nvg);

    drawGUI(nvg);
}

void Object::drawBackground(NVGcontext* nvg)
{
    nvgBeginPath(nvg);
    auto bgCol = isHovered ? nvgRGB(34, 34, 34) : nvgRGB(33, 33, 33);

    if (isGuiTransparent)
        bgCol.a *= isInLockedMode ? 0.0f : 0.3f;

    auto outLineCol = isSelected ? nvgRGB(28, 73, 119) : isGuiTransparent ? bgCol : nvgRGB(45, 45, 45);

    nvgDrawRoundedRect(nvg, 0, 0, width, height, bgCol, outLineCol, 6.0f);
};

void Object::drawGUI(NVGcontext* nvg)
{
    nvgFontSize(nvg, 16.0f);
    nvgFontFace(nvg, "Regular");
    nvgFillColor(nvg, nvgRGB(190, 190, 190));
    nvgTextAlign(nvg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);

    nvgText(nvg, 10, height / 2, shortName.c_str(), nullptr);

//#define  DEBUG_FONT_METRICS
#ifdef DEBUG_FONT_METRICS
    auto none = nvgRGBA(255, 0, 0, 0);
    auto red = nvgRGB(255, 0, 0);
    auto green = nvgRGB(0, 255, 0);

    auto textW = nvgTextBounds(nvg, 0, 0, shortName.c_str(), nullptr, nullptr);
    nvgDrawRoundedRect(nvg, 10, 0, textW, 32, none, red, 0);
    nvgDrawRoundedRect(nvg, 10, 0, textCacheWidth, 32, none, green, 0);
#endif
}
