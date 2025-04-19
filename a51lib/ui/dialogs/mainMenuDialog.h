#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"
#include "../UIBitmap.h"
#include "../UIButton.h"

namespace ui
{

    class MainMenuDialog : public Dialog
    {
    public:
        MainMenuDialog();
        ~MainMenuDialog();

        enum controlIDs
        {
            IDC_MAIN_MENU_CAMPAIGN,
            IDC_MAIN_MENU_MULTI,
            IDC_MAIN_MENU_ONLINE,
            IDC_MAIN_MENU_SETTINGS,
            IDC_MAIN_MENU_PROFILES,
            IDC_MAIN_MENU_CREDITS,
            IDC_MAIN_MENU_NAV_TEXT,
            IDC_MAIN_MENU_SIGN_IN,
            IDC_MAIN_MENU_SILENT_LOGIN_TEXT,
        };
        
        static void registerDialog(Manager*);

        bool create(User*           user,
                    Manager*        manager,
                    DialogTemplate* dialogTemplate,
                    const IntRect&  position,
                    Window*         parent,
                    int             flags);

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onUpdate(float deltaTime) override;

        void onPadSelect(Window*) override;

    protected:
            int currentHighlight;

        Button* buttonCampaign;
        Button* buttonMultiPlayer;
        Button* buttonOnline;
        Button* buttonSettings;
        Button* buttonProfiles;
        Button* buttonCredits;
        Text*    navText;
    };
}
