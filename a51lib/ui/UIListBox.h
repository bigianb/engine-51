#pragma once

#include <string>

#include "UIControl.h"
#include "UIManager.h"

namespace ui
{
    class ListBox : public Control
    {
    public:
        static Window* factory(User* user, Manager* manager, const IntRect& position, Window* parent, int flags)
        {
            ListBox* control = new ListBox();
            control->create(user, manager, position, parent, flags);
            return control;
        }

        struct Item
        {
            bool         Enabled;
            std::wstring Label;
            int          Data[2];
            Colour       Color;
            uint32_t     Flags;
        };

        ListBox()
        {
        }

        bool create(User*          user,
                    Manager*       manager,
                    const IntRect& position,
                    Window*        parent,
                    int            flags);

        void render(Renderer& renderer, int ox = 0, int oy = 0);

    protected:
        bool m_ExitOnSelect;
        bool m_ExitOnBack;
        bool m_ShowBorders;
        bool m_ShowFrame;
        bool m_ShowHeaderBar;
        bool m_AllowParentNavigate;
        bool m_DisableCursor;
        int  m_iElementFrame;
        int  m_iElement_sb_arrowdown;
        int  m_iElement_sb_arrowup;
        int  m_iElement_sb_container;
        int  m_iElement_sb_thumb;

        int     m_TrackHighLight;
        int     m_CursorX;
        int     m_CursorY;
        IntRect m_UpArrow;
        IntRect m_DownArrow;
        IntRect m_ScrollBar;
        bool    m_MouseDown;
        bool    m_ScrollDown;
        float   m_ScrollTime;

        std::vector<Item> m_Items;
        int               m_iSelection;
        int               m_iSelectionBackup;
        int               m_iFirstVisibleItem;
        int               m_nVisibleItems;
        int               m_LineHeight;

        Colour m_BackgroundColor;
        Colour m_HeaderBarColor;
        Colour m_HeaderColor;
    };
};
