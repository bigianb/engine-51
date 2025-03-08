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

        Control* gotoControl(int controlId);

        bool gotoControl(ui::Control* control);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

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
        };

        void        setState(DialogState s) { state = s; }
        DialogState getState() { return state; }

        void initScreenScaling(const IntRect& position);
        bool updateScreenScaling(float DeltaTime, bool DoWipe);

        int oldCursorX, oldCursorY;

    protected:
        int             frameElementIdx;
        int             navW, navH, navX, navY;
        DialogTemplate* dialogTemplate;
        Colour          backgroundColor;
        bool            inputEnabled;
        int             currentControl;

        std::vector<Control*> navgraph;
        DialogState           state;

        // scaling
        IntRect m_CurrPos;
        IntRect m_RequestedPos;
        IntRect m_StartPos;
        float   m_scaleX;
        float   m_scaleY;
        float   m_totalX;
        float   m_scaleCount;
        float   m_scaleAngle;
    };

};
