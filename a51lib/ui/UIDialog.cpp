#include "UIDialog.h"
#include "UIControl.h"

bool ui::Dialog::create(User*           user,
                        Manager*        manager,
                        DialogTemplate* dialogTemplate,
                        const IntRect&  position,
                        Window*         parent,
                        int             flags)
{
    // Do window creation
    bool success = Window::create(user, manager, position, parent, flags);

    frameElementIdx = manager->findElement("frame");

    this->dialogTemplate = dialogTemplate;
    navW = dialogTemplate ? dialogTemplate->navW : 0;
    navH = dialogTemplate ? dialogTemplate->navH : 0;
    navX = 0;
    navY = 0;

    backgroundColor = Colour(40, 40, 0, 192);
    inputEnabled = true;

    state = DialogState::Init;
    currentControl = -1;

    oldCursorX = user->cursorX;
    oldCursorY = user->cursorY;

    navgraph.reserve(navW * navH);
    for (int i = 0; i < (navW * navH); ++i) {
        navgraph.push_back(nullptr);
    }

    if (dialogTemplate) {
        //setLabel(g_StringTableMgr( "ui", dialogTemplate->stringID ));
    }

    this->dialogTemplate = dialogTemplate;

    // Create controls and add to navigation graph if template exists
    if (dialogTemplate) {
        for (int i = 0; i < dialogTemplate->nControls; ++i) {
            ui::ControlTemplate* pControlTem = &dialogTemplate->controls[i];

            // For now create each control
            IntRect Pos(pControlTem->x, pControlTem->y, pControlTem->x + pControlTem->w, pControlTem->y + pControlTem->h);

            Pos.left = (int)(pControlTem->x * manager->getScaleX());
            Pos.top = (int)(pControlTem->y * manager->getScaleY());
            Pos.right = Pos.left + (int)(pControlTem->w * manager->getScaleX());
            Pos.bottom = Pos.top + (int)(pControlTem->h * manager->getScaleY());

            Control* pControl = (Control*)manager->createWin(user, pControlTem->clazz, Pos, this, pControlTem->flags);

            pControl->setControlId(pControlTem->controlID);

            // Configure the control
            //pControl->setLabel( g_StringTableMgr( "ui", pControlTem->stringID ) );

            /*
            if( strcmp( pControlTem->clazz, "edit" ) == 0 )
            {
                ui_edit*    pEdit = (ui_edit*)pControl;
                pEdit->SetVirtualKeyboardTitle( g_StringTableMgr( "ui", pControlTem->stringID ) );
            }
            */

            // Save navgrid position
            pControl->setNavPos(IntRect(pControlTem->nx, pControlTem->ny, pControlTem->nx + pControlTem->nw, pControlTem->ny + pControlTem->nh));

            // Add control into navgrid
            for (int y = pControlTem->ny; y < (pControlTem->ny + pControlTem->nh); y++) {
                for (int x = pControlTem->nx; x < (pControlTem->nx + pControlTem->nw); x++) {
                    navgraph[x + y * navW] = pControl;
                }
            }
        }
    }

    return success;
}
