#pragma once

#include "UIWindow.h"
#include "UIManager.h"

namespace ui
{
    class DialogTemplate;
    class Control;

    class Dialog : public Window
    {
    public:
        virtual void render(Renderer& renderer, int ox = 0, int oy = 0);

        Control* gotoControl(int controlId = 0)
        {
            // TODO
            return nullptr;
        }

        bool gotoControl( ui::Control* control );

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        protected:
            int frameElementIdx;
            int navW, navH, navX, navY;
            DialogTemplate* dialogTemplate;
            Colour backgroundColor;
            bool inputEnabled;
            int currentControl;
            int oldCursorX, oldCursorY;
            std::vector<Control*> navgraph;
            enum DialogState
            {
                Init,
                Active,
                Select,
                Back,
                Activate,
                Delete,
                Cancel,
                Retry,
                Timeout,
                Exit,
                Edit,
                Create,
                Popup
            } state;
    };
    
};
