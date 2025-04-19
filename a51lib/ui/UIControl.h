#pragma once

#include "UIWindow.h"
#include "UIManager.h"

namespace ui
{
    class Control : public Window
    {
    public:
        Control();
        void render(Renderer& renderer, int ox = 0, int oy = 0);

        const IntRect& getNavPos() const
        {
            return navPos;
        };

        void setNavPos(const IntRect& r)
        {
            navPos = r;
        }

    private:
        IntRect navPos;
    };
};
