//
// Created by alexw on 28/01/2025.
//

#pragma once

#include "Component.h"

namespace pptk {

class ViewPort : public Component {

Component* viewedComponent;

public:
ViewPort(Component *c) : viewedComponent(c) {}

};

}

