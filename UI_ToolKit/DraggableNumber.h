#pragma once

#include "../UI_Toolkit/TextEditor.h"

class DraggableNumber : public pptk::Component {
public:
DraggableNumber()
{
    textEditor = std::make_unique<pptk::TextEditor>();
};

void DraggableNumber::mouseDrag(const pptk::Point& position, const pptk::Point& delta, pptk::Button button) override
{

}

private:
std::unique_ptr<pptk::TextEditor> textEditor;
};