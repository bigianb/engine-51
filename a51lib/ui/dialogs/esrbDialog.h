#pragma once

#include "../UIDialog.h"
#include "../UIManager.h"

namespace ui
{
    //extern ui_win*  dlg_esrb_factory   ( s32 UserID, ui_manager* pManager, ui_manager::dialog_tem* pDialogTem, const irect& Position, ui_win* pParent, s32 Flags, void* pUserData );

    class EsrbDialog : public Dialog
    {
    public:
        EsrbDialog(void);
        ~EsrbDialog(void);

        static void registerDialog(Manager*);

        bool         create(User*           user,
                            Manager*        manager,
                            DialogTemplate* dialogTemplate,
                            const IntRect&  position,
                            Window*         parent,
                            int             flags);


        void onPadSelect(Window* pWin);
        void onUpdate(Window* pWin, float DeltaTime);
        void onPadHelp(Window* pWin);

    protected:
        //ui_text*			m_pESRBText;
        float waitTime;
    };

}
