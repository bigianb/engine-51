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

    class CampaignMenuDialog : public Dialog
    {
    public:
        CampaignMenuDialog();
        ~CampaignMenuDialog();

        enum controlIDs
        {
            IDC_CAMPAIGN_MENU_NEW_CAMPAIGN,
            IDC_CAMPAIGN_MENU_RESUME_CAMPAIGN,
            IDC_CAMPAIGN_MENU_EDIT_PROFILE,
            IDC_CAMPAIGN_MENU_LORE,
            IDC_CAMPAIGN_MENU_SECRETS,
            IDC_CAMPAIGN_MENU_EXTRAS,
            IDC_CAMPAIGN_MENU_LEVEL_SELECT, // debug
            IDC_CAMPAIGN_MENU_NAV_TEXT,
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

    protected:
        Button* m_pButtonNewCampaign;
        Button* m_pButtonResumeCampaign;
        Button* m_pButtonEditProfile;
        Button* m_pButtonLore;
        Button* m_pButtonSecrets;
        Button* m_pButtonExtras;

        Button* m_pButtonLevelSelect; // Debug
        Text*   m_pNavText;

        Popup* m_PopUp;
        int    m_PopUpResult;

        int m_CurrHL;

        bool m_bCheckKeySequence;

        StateMachine* stateMachine;
    };
}
