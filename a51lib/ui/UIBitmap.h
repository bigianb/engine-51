#pragma once

#include "UIControl.h"
#include "UIManager.h"

namespace ui
{
    class BitmapControl : public Control
    {
    public:
        static Window* factory(User* user, Manager* manager, const IntRect& position, Window* parent, int flags)
        {
            BitmapControl* control = new BitmapControl();
            control->create(user, manager, position, parent, flags);
            return control;
        }

        BitmapControl();

        void setBitmap(Bitmap* b);

        bool create(User*          user,
                    Manager*       manager,
                    const IntRect& position,
                    Window*        parent,
                    int            flags) override;

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

    protected:
        Bitmap*  bitmap;
        bool isElement;
        int  renderState;
    };
};
