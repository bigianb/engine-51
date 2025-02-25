#include "pressStartDialog.h"

namespace ui
{
    PressStartDialog::PressStartDialog()
    {
    }

    PressStartDialog::~PressStartDialog()
    {
        destroy();
    }

    enum controlIDs
    {
        IDC_START_GAME_TEXT,
    };

    static ControlTemplate controls[] =
        {
            {IDC_START_GAME_TEXT, "IDS_LOADING_MSG", "text", 246, 300, 120, 40, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_NULL",
            1, 1,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags)
    {
        PressStartDialog* dialog = new PressStartDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    bool PressStartDialog::create(User*           user,
                                 Manager*        manager,
                                 DialogTemplate* dialogTemplate,
                                 const IntRect&  position,
                                 Window*         parent,
                                 int             flags)
    {
        bool success = false;

        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        // initialize esrb text
        text = (ui::Text*)findChildById(IDC_START_GAME_TEXT);
        text->clearFlag(Window::WF_VISIBLE);
        text->setLabelColour(Colour(126, 220, 60, 255));

        setFlag(Window::WF_DISABLED);
        startLoading = false;
        state = DialogState::Active;
        return success;
    }

    void PressStartDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("press start", &thisDialogTemplate, &dlg_factory);
    }

    void PressStartDialog::onUpdate(float deltaTime)
    {
        startLoading = true;
        text->setFlag(Window::WF_VISIBLE);
        state = DialogState::Select;
    }

    void PressStartDialog::render(Renderer& renderer, int ox, int oy)
    {
        // render background filter
        if (!startLoading) {

            int XRes = 640;
            int YRes = 480;

            IntRect rb(0, 0, XRes, YRes);
            /* TODO:
            renderer->renderGouraudRect(rb, Colour(0, 0, 0, 180),
                                        Colour(0, 0, 0, 180),
                                        Colour(0, 0, 0, 180),
                                        Colour(0, 0, 0, 180), false);
                                        */
        }

        Dialog::render(renderer, ox, oy);
    }
}
