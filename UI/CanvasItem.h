/*
// Copyright (c) 2025 Alex Mitchell
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

#include "../UI_Toolkit/Component.h"

class CanvasItem : public pptk::Component {
public:

    friend class Object;
    friend class Connection;

    void setSelected(bool shouldBeSelected)
    {
        if (isSelected != shouldBeSelected)
            isSelected = shouldBeSelected;
    }

    [[nodiscard]] bool getIsSelected() const { return isSelected; }

private:

    bool isSelected = false;
};