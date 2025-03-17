#include "UIListBox.h"
#include "UIFont.h"

#include "../Colour.h"
#include "../system/Renderer.h"

namespace ui
{
#define SPACE_TOP 4
#define SPACE_BOTTOM 4
#define LINE_HEIGHT 16
#define HEADER_HEIGHT 22
#define TEXT_OFFSET -2

    bool ListBox::create(User*          user,
                         Manager*       manager,
                         const IntRect& position,
                         Window*        parent,
                         int            flags)
    {
        bool Success = Control::create(user, manager, position, parent, flags);

        // Set default text flags
        labelFlags = Font::h_center | Font::v_center | Font::clip_ellipsis | Font::clip_l_justify;
        m_BackgroundColor = Colour(0, 0, 0, 0);
        m_HeaderBarColor = Colour(0, 0, 0, 0);
        m_HeaderColor = COLOR_WHITE;

        // Initialize data
        m_iElementFrame = manager->findElement("sb_frame");
        m_iElement_sb_arrowdown = manager->findElement("sb_arrowdown");
        m_iElement_sb_arrowup = manager->findElement("sb_arrowup");
        m_iElement_sb_container = manager->findElement("sb_container");
        m_iElement_sb_thumb = manager->findElement("sb_thumb");

        m_LineHeight = LINE_HEIGHT;
        m_ExitOnSelect = true;
        m_ExitOnBack = false;
        m_iSelection = -1;
        m_iSelectionBackup = -1;
        m_iFirstVisibleItem = 0;
        m_ShowBorders = true;
        m_ShowFrame = true;
        m_ShowHeaderBar = false;
        m_AllowParentNavigate = false;
        m_DisableCursor = false;
        m_nVisibleItems = (position.getHeight() - SPACE_TOP - SPACE_BOTTOM) / m_LineHeight;
        //m_Font = g_UiMgr->FindFont("small");

        m_MouseDown = false;
        m_ScrollDown = false;
        m_ScrollTime = 0.0f;

        return Success;
    }

    void ListBox::render(Renderer& renderer, int ox, int oy)
    {
        if (!visible()) {
            return;
        }

        // Calculate rectangle
        IntRect br((position.left + ox), (position.top + oy), (position.right + ox), (position.bottom + oy));
        IntRect r = br;
        IntRect r2 = r;
        r.right -= 14;
        r2.left = r.right;

        Colour TextColor1 = COLOR_WHITE;
        Colour TextColor2 = COLOR_BLACK;
        int    state = Manager::CS_NORMAL;
        if (disabled()) {
            state = Manager::CS_DISABLED;
            TextColor1 = COLOR_GREY;
            TextColor2 = Colour(0, 0, 0, 0);
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == WF_HIGHLIGHT) {
            state = Manager::CS_HIGHLIGHT;
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == WF_SELECTED) {
            state = Manager::CS_SELECTED;
        } else if ((flags & (WF_HIGHLIGHT | WF_SELECTED)) == (WF_HIGHLIGHT | WF_SELECTED)) {
            state = Manager::CS_HIGHLIGHT_SELECTED;
        } else {
            state = Manager::CS_NORMAL;
        }

        auto* mgr = getUIManger();
        if (flags & WF_HIGHLIGHT) {
            //mgr->addHighlight(user, r);
        }

        IntRect rb = r;
        if (m_ShowFrame) {
            rb.deflate(1, 1);
        }

        if (m_ShowHeaderBar) {
            IntRect hb = rb;
            hb.setHeight(HEADER_HEIGHT);

            renderer.renderRect(hb, m_HeaderBarColor, false);

            hb.left += 2;
            mgr->renderText(renderer, "small", hb, Font::h_center | Font::v_center, COLOR_BLACK, label);
            hb.translate(-1, -1);
            mgr->renderText(renderer, "small", hb, Font::h_center | Font::v_center, m_HeaderColor, label);

            rb.top += 22;
            r2.top += 22;
        }
    }

    void ListBox::EnableHeaderBar(void)
    {
        m_ShowHeaderBar = true;
        m_nVisibleItems = (position.getHeight() - SPACE_TOP - SPACE_BOTTOM - HEADER_HEIGHT) / m_LineHeight;
    }

    void ListBox::DisableHeaderBar(void)
    {
        m_ShowHeaderBar = false;
        m_nVisibleItems = (position.getHeight() - SPACE_TOP - SPACE_BOTTOM) / m_LineHeight;
    }

    int ListBox::GetItemCount() const
    {
        return m_Items.size();
    }

    const std::wstring& ListBox::GetItemLabel(int iItem) const
    {
        return m_Items[iItem].Label;
    }

    void ListBox::SetItemLabel(int iItem, const std::wstring& Label)
    {
        m_Items[iItem].Label = Label;
    }

    int ListBox::GetItemData(int iItem, int Index) const
    {
        return m_Items[iItem].Data[Index];
    }

    const std::wstring& ListBox::GetSelectedItemLabel() const
    {
        return m_Items[m_iSelection].Label;
    }

    int ListBox::GetSelectedItemData(int Index) const
    {
        return m_Items[m_iSelection].Data[Index];
    }

    void ListBox::SetItemColor(int iItem, const Colour& Color)
    {
        m_Items[iItem].Color = Color;
    }

    Colour ListBox::GetItemColor(int iItem) const
    {
        return m_Items[iItem].Color;
    }

    void ListBox::SetBackgroundColor(Colour Color)
    {
        m_BackgroundColor = Color;
    }

    Colour ListBox::GetBackgroundColor() const
    {
        return m_BackgroundColor;
    }

    void ListBox::SetExitOnSelect(bool State)
    {
        m_ExitOnSelect = State;
    }

}
