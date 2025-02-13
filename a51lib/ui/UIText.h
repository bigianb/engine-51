#pragma once

#include "UIControl.h"
#include "UIManager.h"

namespace ui
{
    class Text : public Control
    {
    public:
        static Window* factory(User* user, Manager* manager, const IntRect& position, Window* parent, int flags)
        {
            Text* control = new Text();
            control->create(user, manager, position, parent, flags);
            return control;
        }

        Text() : useSmallText(false) {}

        void render(Renderer& renderer, int ox = 0, int oy = 0);

    private:
        bool useSmallText;
    };
};
