#include "UIButton.h"
#include "UIFont.h"

namespace ui
{

    Colour Button::m_TextColorNormal;
    Colour Button::m_TextColorHighlight;
    Colour Button::m_TextColorDisabled;
    Colour Button::m_TextColorShadow;

    Button::~Button()
    {
        destroy();
    }

    void Button::render(Renderer& renderer, int ox, int oy)
    {
        if (!visible()) {
            return;
        }

        //int     State = ui_manager::CS_NORMAL;

        Colour TextColor1 = m_TextColorNormal;
        Colour TextColor2 = m_TextColorShadow;

        // Calculate rectangle
        IntRect r((position.left + ox), (position.top + oy), (position.right + ox), (position.bottom + oy));

        // Render appropriate state
        if (disabled()) {
            // state = ui_manager::CS_DISABLED;
            TextColor1 = m_TextColorDisabled;
            TextColor2 = Colour(0, 0, 0, 0);
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == WF_HIGHLIGHT) {
            //State = ui_manager::CS_HIGHLIGHT;
            TextColor1 = m_TextColorHighlight;
            TextColor2 = m_TextColorShadow;
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == WF_SELECTED) {
            //State = ui_manager::CS_SELECTED;
            TextColor1 = m_TextColorHighlight;
            TextColor2 = m_TextColorShadow;
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == (WF_HIGHLIGHT | WF_SELECTED)) {
            //State = ui_manager::CS_HIGHLIGHT_SELECTED;
            TextColor1 = m_TextColorHighlight;
            TextColor2 = m_TextColorShadow;
        } else {
            //State = ui_manager::CS_NORMAL;
            TextColor1 = m_TextColorNormal;
            TextColor2 = m_TextColorShadow;
        }
        /*
                // check to see if we should render the button artwork
                if (flags & WF_BORDER) {
                    m_pManager->RenderElement(m_iElement, r, State);

                    // Add Highlight to list
                    if (flags & WF_HIGHLIGHT) {
                        m_pManager->AddHighlight(m_UserID, r);
                    }
                } else if (m_useSmallText) {
                    if (flags & WF_HIGHLIGHT) {
                        TextColor1 = Colour(0, 0, 0, 255);
                        TextColor2 = Colour(0, 0, 0, 0);

                        s32 alpha = 128 + (g_UiMgr->GetHighlightAlpha(8) * 8); // 64<->192
                        m_pManager->RenderRect(r, Colour(79, 214, 60, alpha), false);
                    }
                } else {
                    if (m_BitmapID != -1) {
                        if (flags & WF_HIGHLIGHT) {
                            IntRect r2 = r;
                            r2.top -= 2;
                            r2.left -= 2;
                            r2.bottom += 2;
                            r2.right += 2;

                            m_pManager->RenderRect(r2, TextColor1, false);
                        }

                        // render the button bitmap
                        if (m_useNativeColor) {
                            m_pManager->RenderBitmap(m_BitmapID, r, COLOR_WHITE);
                        } else {
                            m_pManager->RenderBitmap(m_BitmapID, r, TextColor1);
                        }
                    }
                }
        */
        // Render Text
        int justFlags = 0;
        if (flags & WF_BUTTON_LEFT) {
            justFlags = (Font::h_left | Font::v_center);
        } else if (flags & WF_BUTTON_RIGHT) {
            justFlags = (Font::h_right | Font::v_center);
        } else {
            justFlags = (Font::h_center | Font::v_center);
        }

        // Render Text
        const char* fontName = m_useSmallText ? "small" : "large";

        // check for pulsing
        if (m_bPulseOn) {
            TextColor1.a = (uint8_t)m_PulseValue;
        }

        r.translate(1, -1);
        getUIManger()->renderText(renderer, fontName, r, justFlags, TextColor2, label);
        r.translate(-1, -1);
        getUIManger()->renderText(renderer, fontName, r, justFlags, TextColor1, label);

        for (Window* child : children) {
            child->render(renderer, getPosition().left + ox, getPosition().top + oy);
        }
    }

    bool Button::create(User*          user,
                        Manager*       manager,
                        const IntRect& position,
                        Window*        parent,
                        int            flags)
    {

        bool success = Control::create(user, manager, position, parent, flags);

        // Initialize Data
        m_iElement = manager->findElement("button");

        // Initialize flags
        m_useSmallText = false;
        m_useNativeColor = false;
        m_bPulseOn = false;
        m_bPulseUp = false;
        m_PulseRate = 1024.0f;
        m_PulseValue = 255.0f;

        // Initialize bitmap ID
        m_BitmapID = -1;

        // clear data
        m_Data = 0;

        return success;
    }

    void Button::onUpdate(float DeltaTime)
    {
        // update color pulsing
        if (m_bPulseOn) {
            if (m_bPulseUp) {
                m_PulseValue += DeltaTime * m_PulseRate;

                if (m_PulseValue >= 255.0f) {
                    m_PulseValue = 255.0f;
                    m_bPulseUp = false;
                }
            } else {
                m_PulseValue -= DeltaTime * m_PulseRate;

                if (m_PulseValue <= 128.0f) {
                    m_PulseValue = 128.0f;
                    m_bPulseUp = true;
                }
            }
        }
    }
}
