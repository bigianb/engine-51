#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"
#include "../UIText.h"
#include "../UIBitmap.h"
#include "../UIButton.h"
#include "../UIListBox.h"

namespace ui
{

    class ProfileSelectDialog : public Dialog
    {
    public:
        ProfileSelectDialog();
        ~ProfileSelectDialog();

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
                    int             flags);

        void render(Renderer& renderer, int ox = 0, int oy = 0) override;

        void onUpdate(float deltaTime) override;

        void onPadSelect(Window*) override;

    protected:
        int currentHighlight;

        Text*    navText;
        ListBox* profileList;
    };
}
