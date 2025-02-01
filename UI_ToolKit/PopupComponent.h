//
// Created by alexw on 1/02/2025.
//

#pragma once

#include "Component.h"
#include "ToggleButton.h"

namespace pptk
{
    class PopupComponent : public Component
    {
    public:
        PopupComponent()
        {
        }

        void registerMouseListener(ToggleButton* toggleButton)
        {
            button = toggleButton;
            registerGlobalMouseListener([this](Component* comp)
            {
                if (!(this->isOrHasChild(comp) || comp == button))
                {
                    button->setActive(false);
                    close();
                }
            });
        }

        virtual ~PopupComponent()
        {
            if (button)
                button->setActive(false);
            unregisterGlobalMouseListener();
        }

        virtual void close()
        {
            unregisterGlobalMouseListener();
            setPopupComponent(nullptr);
        }

    private:
        ToggleButton* button = nullptr;
    };
}
