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

        void SetLineHeight(int Height);

        void SetExitOnSelect(bool State);
        void SetExitOnBack(bool State) { m_ExitOnBack = State; }

        int      AddItem(const std::wstring& Item, int Data = 0, int Data2 = 0, bool State = true, uint32_t Flags = 0);
        int      AddItem(const wchar_t* Item, int Data = 0, int Data2 = 0, bool State = true, uint32_t Flags = 0);
        void     DeleteAllItems();
        void     DeleteItem(int iItem);
        void     DeleteSelectedItem();
        void     EnableItem(int iItem, bool State);
        uint32_t GetItemFlags(int iItem);

        void EnableBorders() { m_ShowBorders = true; }
        void DisableBorders() { m_ShowBorders = false; }

        void EnableFrame() { m_ShowFrame = true; }
        void DisableFrame() { m_ShowFrame = false; }

        void EnableHeaderBar();
        void DisableHeaderBar();

        void SetHeaderBarColor(Colour Color) { m_HeaderBarColor = Color; }
        void SetHeaderColor(Colour Color) { m_HeaderColor = Color; }

        void EnableParentNavigation() { m_AllowParentNavigate = true; }
        void DisableParentNavigation() { m_AllowParentNavigate = false; }

        void EnableCursor() { m_DisableCursor = false; }
        void DisableCursor() { m_DisableCursor = true; }

        Item&               GetItem(int Index) { return m_Items[Index]; }
        int                 GetItemCount() const;
        const std::wstring& GetItemLabel(int iItem) const;
        void                SetItemLabel(int iItem, const std::wstring& Label);
        int                 GetItemData(int iItem, int Index = 0) const;
        const std::wstring& GetSelectedItemLabel() const;
        int                 GetSelectedItemData(int Index = 0) const;
        void                SetItemColor(int iItem, const Colour& Color);
        Colour              GetItemColor(int iItem) const;

        int FindItemByLabel(const std::wstring& Label);
        int FindItemByData(int Data, int Index = 0);

        int  GetSelection() const;
        void SetSelection(int iSelection);
        void ClearSelection();

        void EnsureVisible(int iItem);
        int  GetNumEnabledItems();

        int  GetCursorOffset();
        void SetSelectionWithOffset(int iSelection, int Offset);

        void   SetBackgroundColor(Colour Color);
        Colour GetBackgroundColor() const;

        void AlphaSortList();

    private:
        void renderItem(Renderer& renderer, IntRect r, const Item& item, const Colour& c1, const Colour& c2);

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
