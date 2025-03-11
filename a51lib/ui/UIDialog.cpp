#include "UIDialog.h"
#include "UIControl.h"
#include "UIFont.h"
#include <cmath>

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
        setLabel(manager->lookupString("ui", dialogTemplate->stringID));
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
            if (pControl == nullptr) {
                continue;
            }

            pControl->setControlId(pControlTem->controlID);

            // Configure the control
            pControl->setLabel(manager->lookupString("ui", pControlTem->stringID));

            /* TODO
            if( strcmp( pControlTem->clazz, "edit" ) == 0 )
            {
                ui_edit*    pEdit = (ui_edit*)pControl;
                pEdit->SetVirtualKeyboardTitle( manager->lookupString( "ui", pControlTem->stringID ) );
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

ui::Control* ui::Dialog::gotoControl(int controlId)
{
    Control* control = nullptr;
    if (controlId < children.size()) {
        Window* child = children[controlId];

        if (child->enabled() && child->isDynamic()) {
            
            int x = child->getWidth() / 2;
            int y = child->getHeight() / 2;
            child->localToScreen(x, y);
            getUIManger()->setCursorPos(user, x, y);
            
            control = (Control*)child;
            const IntRect& r = control->getNavPos();
            navX = r.left + r.getWidth() / 2;
            navY = r.top + r.getHeight() / 2;

            currentControl = controlId;
        }
    }
    return control;
}

bool ui::Dialog::gotoControl(ui::Control* control)
{
    bool success = false;
    int  iControl = -1;

    Control* child = NULL;

    // Locate the control
    for (int i = 0; i < children.size() && iControl < 0; i++) {
        if (children[i] == control) {
            child = (Control*)children[i];
            iControl = i;
        }
    }

    if (!child->isStatic() && !child->disabled()) {
        
        int x = child->getWidth() / 2;
        int y = child->getHeight() / 2;

        // Position cursor over Child
        child->localToScreen(x, y);
        getUIManger()->setCursorPos(user, x, y);
        
        // Set Navigation cursor to center of the control
        const IntRect& r = child->getNavPos();
        navX = r.left + r.getWidth() / 2;
        navY = r.top + r.getHeight() / 2;

        // store index of current control
        currentControl = iControl;

        success = true;
    }

    return success;
}

void ui::Dialog::initScreenScaling(const IntRect& position)
{
    // store requested frame size
    m_RequestedPos = position;

    // set starting position
    getUIManger()->getScreenSize(m_CurrPos);
    m_StartPos = m_CurrPos;

    // set up scaling
    m_scaleCount = 0.3f; // time to scale in seconds
    m_scaleAngle = 180.0f / m_scaleCount;
    m_scaleX = (position.left - m_CurrPos.left) / 2.0f;
    m_totalX = 0.0f;

    // set starting position
    setPosition(m_CurrPos);
    getUIManger()->setScreenScaling(true);

    // play scaling sound
    /*
    if( g_UiMgr->IsScreenOn() )
    {
        if( m_scaleX > 0 )
        {
            g_AudioMgr.Play( "ResizeLarge" );
        }
        else
        {
            g_AudioMgr.Play( "ResizeSmall" );
        }
    }
    else
    {
        if( m_scaleX > 0 )
        {
            g_AudioMgr.Play( "Bars_Out" );
        }
        else
        {
            g_AudioMgr.Play( "Bars_In" );
        }
    }
        */
}

bool ui::Dialog::updateScreenScaling(float DeltaTime, bool DoWipe)
{
    // scale window if necessary
    if (m_scaleCount) {
        // apply delta time
        m_scaleCount -= DeltaTime;

        if (m_scaleCount <= 0) {
            // last one - make sure window is correct size
            m_scaleCount = 0;
            m_CurrPos = m_RequestedPos;

            // resize the window
            setPosition(m_CurrPos);
            getUIManger()->setScreenSize(m_CurrPos);
            getUIManger()->setScreenScaling(false);

            // start a screen wipe
            if (DoWipe) {
                //g_UiMgr->InitScreenWipe();
            }
        } else {
            m_totalX = m_scaleX + (m_scaleX * cos(DEG_TO_RAD(m_scaleAngle * m_scaleCount)));
            m_CurrPos.left = m_StartPos.left + m_totalX;
            m_CurrPos.right = m_StartPos.right - m_totalX;

            // resize the window
            setPosition(m_CurrPos);
            getUIManger()->setScreenSize(m_CurrPos);

            // still more to do!
            return true;
        }
    }

    // we're done!
    return false;
}

void ui::Dialog::render(Renderer& renderer, int ox, int oy)
{
    if (!visible()) {
        return;
    }

    getUIManger()->renderScreenHighlight(renderer);

    for (auto child : children) {
        child->render(renderer, position.left + ox, position.top + oy);
    }

    // manager->RenderScreenWipe();
    // manager->RenderRefreshBar();

    if (flags & WF_BORDER) {
        IntRect r(position.left + ox, position.top + oy, position.right + ox, position.bottom + oy);

        // Render the Frame
        if (disabled()) {
            // disabled version
            getUIManger()->renderElement(renderer, frameElementIdx, r, 1);
        } else {
            // normal frame
            getUIManger()->renderElement(renderer, frameElementIdx, r, 0);
        }

        // Render Title
        if (!getUIManger()->isScreenScaling()) {
            /* TODO
            rb.Deflate( 0, 5 );
            s32 FontID = g_UiMgr->FindFont("large");
            getUIManger()->renderText( FontID, rb, ui::Font::h_center, m_TextColorShadow, m_Label );
            rb.Translate( -1, -1 );
            getUIManger()->renderText( FontID, rb, ui_font::h_center, m_TextColorNormal, m_Label );
            */
        }

        getUIManger()->renderScreenGlow(renderer);
    }
}

void ui::Dialog::onPadNavigate(ui::Window::NavigateDir code, int presses, int repeats, bool wrapX, bool wrapY)
{
    if (!inputEnabled) {
        return;
    }
    int x = navX;
    int y = navY;
    int dx = 0;
    int dy = 0;

    // Which way are we moving
    switch (code) {
    case ui::Window::NAV_UP:
        dy = -1;
        break;
    case ui::Window::NAV_DOWN:
        dy = 1;
        break;
    case ui::Window::NAV_LEFT:
        dx = -1;
        break;
    case ui::Window::NAV_RIGHT:
        dx = 1;
        break;
    }

    bool        wrapped = false;
    ui::Window* pCurrentWin = user->lastWindowUnderCursor;
    while (((x < navW) && (x >= 0)) && ((y < navH) && (y >= 0))) {
        auto* pWin = navgraph[x + y * navW];
        if (pWin && (pWin != pCurrentWin)) {
            bool Found = false;

            if (pWin->disabled()) {
                if (dy != 0) {
                    // look to the left and right
                    int  xleft = x;
                    int  xright = x;
                    bool doneLeft = 0;
                    bool doneRight = 0;

                    while (1) {
                        // look left
                        if (xleft > 0) {
                            xleft--;
                            pWin = navgraph[xleft + y * navW];
                            if (pWin && (pWin != pCurrentWin) && pWin->enabled()) {
                                x = xleft;
                                Found = true;
                                break;
                            }
                        } else {
                            doneLeft = true;
                        }

                        // look right
                        if (xright < (navW - 1)) {
                            xright++;
                            pWin = navgraph[xright + y * navW];
                            if (pWin && (pWin != pCurrentWin) && pWin->enabled()) {
                                x = xright;
                                Found = true;
                                break;
                            }
                        } else {
                            doneRight = true;
                        }

                        if (doneLeft && doneRight) {
                            break;
                        }
                    }
                }
            } else {
                Found = true;
            }

            if (Found) {
                IntRect r = pWin->getPosition();
                int   cx = r.getWidth() / 2;
                int   cy = r.getHeight() / 2;
                pWin->localToScreen(cx, cy);
                getUIManger()->setCursorPos(user, cx, cy);
                navX = x;
                navY = y;
                break;
            }
        }

        // Advance to next position
        x += dx;
        y += dy;

        // Check for wrapping
        if ((wrapX) && (!wrapped)) {
            if (x < 0) {
                x = navW - 1;
                wrapped = true;
            }

            if (x == navW) {
                x = 0;
                wrapped = true;
            }
        }

        if ((wrapY) && (!wrapped)) {
            if (y < 0) {
                y = navH - 1;
                wrapped = true;
            }

            if (y == navH) {
                y = 0;
                wrapped = true;
            }
        }
    }
}
