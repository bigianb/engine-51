#include "esrbDialog.h"

namespace ui
{
    EsrbDialog::EsrbDialog()
    {
    }

    EsrbDialog::~EsrbDialog()
    {
        destroy();
    }

    enum controls
    {
        IDC_ESRB_MESSAGE,
    };

    ControlTemplate Esrb_Controls[] =
        {
            {IDC_ESRB_MESSAGE, "IDS_ESRB_NOTICE", "text", 0, 200, 480, 30, 0, 0, 1, 1, Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    DialogTemplate Esrb_Dialog =
        {
            "IDS_NULL",
            1, 9,
            sizeof(Esrb_Controls) / sizeof(ControlTemplate),
            &Esrb_Controls[0],
            0};

    Dialog* dlg_esrb_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags)
    {
        EsrbDialog* dialog = new EsrbDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    bool EsrbDialog::create(User*           user,
                            Manager*        manager,
                            DialogTemplate* dialogTemplate,
                            const IntRect&  position,
                            Window*         parent,
                            int             flags)
    {
        bool success = false;

        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        // initialize esrb text
        text = (ui::Text*)findChildById(IDC_ESRB_MESSAGE);
        text->setFlag(Window::WF_VISIBLE);
        text->setLabelColour(Colour(230, 230, 230, 255));
        gotoControl(text);

        waitTime = 20.0f;
        state = DialogState::Active;
        return success;
    }

    void EsrbDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("ESRB", &Esrb_Dialog, &dlg_esrb_factory);
    }

    void EsrbDialog::onPadSelect(Window* )
    {
        if (state == DialogState::Active) {
            waitTime = 0.1f;
        }
    }

    void EsrbDialog::onUpdate(float deltaTime)
    {
        if (waitTime > 0.0f) {
            waitTime -= deltaTime;
            if (waitTime <= 0.0f) {
                waitTime = 0.0f;
                state = DialogState::Select;
                text->setLabel(getUIManger()->lookupString("ui", "IDS_NULL"));
            }
        }
    }

}
