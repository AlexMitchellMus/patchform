#pragma once

#include "Component.h"
#include "SafePointer.h"
#include "ToggleButton.h"

namespace pptk
{
    class PopupComponent : public Component
    {
    public:
        PopupComponent() = default;

        void registerMouseListener(Component* trigger)
        {
            button = trigger;

            registerGlobalMouseListener([this](Component* comp)
            {
                if (!(this->isOrHasChild(comp) || comp == button.get()))
                {
                    if (auto toggle = dynamic_cast<ToggleButton*>(button.get()))
                        toggle->setActive(false);

                    close();
                }
            });
        }

        virtual ~PopupComponent()
        {
            if (auto toggle = dynamic_cast<ToggleButton*>(button.get()))
                toggle->setActive(false);

            if (button)
                button->repaint();
        }

        virtual void close()
        {
            unregisterGlobalMouseListener();
            setPopupComponent(nullptr);

            //if (button)
            //    button->repaint();
        }

    private:
        SafePointer<Component> button;
    };
}
