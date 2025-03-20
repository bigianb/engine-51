#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"
#include "../UIBitmap.h"
#include "../UIButton.h"
#include "../UIListBox.h"

#include "../../state/StateMachine.h"

namespace ui
{

    class Popup;

    class ProfileSelectDialog : public Dialog
    {
    public:
        ProfileSelectDialog();
        ~ProfileSelectDialog();

        enum ProfileSelectType
        {
            PROFILE_SELECT_MANAGE,
            PROFILE_SELECT_NORMAL,
            PROFILE_SELECT_OVERWRITE,
        };

        enum controlIDs
        {
            IDC_PROFILE_SELECT_LISTBOX,
            IDC_PROFILE_SELECT_INFOBOX,

            IDC_PROFILE_CARD_SLOT,
            IDC_PROFILE_CREATE_DATE,
            IDC_PROFILE_MODIFIED_DATE,

            IDC_PROFILE_INFO_CREATE_DATE,
            IDC_PROFILE_INFO_MODIFIED_DATE,

            IDC_PROFILE_SELECT_NAV_TEXT,
        };

        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags,
                    StateMachine*   sm);

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onPadSelect(Window*) override;
        void onUpdate(float deltaTime) override;

        void refreshProfileList();

    protected:
        int currentHighlight;

        Text*    navText;
        ListBox* profileList;
        //BlankBox*                        m_pProfileDetails;

        Text* m_pProfileName;
        Text* m_pCreationDate;
        Text* m_pModifiedDate;

        Text* m_pInfoProfileName;
        Text* m_pInfoCardSlot;
        Text* m_pInfoCreationDate;
        Text* m_pInfoModifiedDate;

        Popup* m_PopUp;
        int    m_PopUpResult;
        int    m_PopUpType;

        Popup* m_BackupPopup;
        int    m_BackupPopupResult;

        std::wstring m_ProfileName;
        bool         m_ProfileEntered;
        bool         m_ProfileOk;

        int m_CreateIndex;
        int m_iCard;

        ProfileSelectType m_Type;

        bool m_bEditProfile;
        bool m_bRenderBlackout;

        int m_BlocksRequired;

        StateMachine* stateMachine;
    };
}
