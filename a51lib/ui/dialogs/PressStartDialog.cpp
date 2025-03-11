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
        IDC_A51_LOGO,
        IDC_START_BOX,
        IDC_PRESS_START,
    };

    static ControlTemplate controls[] =
        {
            {IDC_A51_LOGO, "IDS_NULL", "bitmap", 280, 10, 200, 50, 0, 0, 0, 0, Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_START_BOX, "IDS_NULL", "bitmap", 90, 312, 300, 30, 0, 0, 0, 0, Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_PRESS_START, "IDS_PRESS_START_TEXT", "text", 0, 307, 480, 30, 0, 0, 1, 1, Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_PRESS_START_SCREEN",
            1, 9,
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

        text = (ui::Text*)findChildById(IDC_PRESS_START);
        logoBitmap = (ui::BitmapControl*)findChildById(IDC_A51_LOGO);

        logoBitmap->clearFlag(Window::WF_VISIBLE);
        Bitmap* bitmap = manager->loadBitmap("logo", "UI_A51_Logo.XBMP");
        logoBitmap->setBitmap(bitmap);

        text->setFlag(Window::WF_VISIBLE);
        text->setLabelColour(Colour(230, 230, 230, 255));
        gotoControl(text);

        logoBitmap->setFlag(Window::WF_VISIBLE);
        state = DialogState::Active;
        return success;
    }

    void PressStartDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("press start", &thisDialogTemplate, &dlg_factory);
    }

    void PressStartDialog::onUpdate(float deltaTime)
    {
    }

    void PressStartDialog::onPadSelect(Window* )
    {
        if (state == DialogState::Active) {
            // set state
            state = DialogState::Select;
            currentControl = IDC_PRESS_START;

            //g_StateMgr.CloseMovie();
            //g_StateMgr.PlayMovie( "MenuBackground", true, true );
        }
    }

    void PressStartDialog::render(Renderer& renderer, int ox, int oy)
    {

        Dialog::render(renderer, ox, oy);
    }
}
