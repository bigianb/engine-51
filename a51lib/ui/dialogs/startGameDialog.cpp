#include "startGameDialog.h"
#include "../../system/Renderer.h"

namespace ui
{
    StartGameDialog::StartGameDialog()
    {
    }

    StartGameDialog::~StartGameDialog()
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

    static DialogTemplate startGameDialogTemplate =
        {
            "IDS_NULL",
            1, 1,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_startgame_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags, void* userData)
    {
        StartGameDialog* dialog = new StartGameDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    bool StartGameDialog::create(User*           user,
                                 Manager*        manager,
                                 DialogTemplate* dialogTemplate,
                                 const IntRect&  position,
                                 Window*         parent,
                                 int             flags)
    {
        bool success = false;

        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);
        text = (ui::Text*)findChildById(IDC_START_GAME_TEXT);
        text->clearFlag(Window::WF_VISIBLE);
        text->setLabelColour(Colour(126, 220, 60, 255));

        getUIManger()->setScreenOn(false);
        setFlag(Window::WF_DISABLED);

        initScreenScaling(position);

        getUIManger()->disableScreenHighlight();

        setFlag(Window::WF_DISABLED);
        startLoading = false;
        state = DialogState::Active;
        return success;
    }

    void StartGameDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("start game", &startGameDialogTemplate, &dlg_startgame_factory);
    }

    void StartGameDialog::onUpdate(float deltaTime)
    {
        if (getUIManger()->isScreenScaling()) {
            if (!updateScreenScaling(deltaTime, false)) {
                state = DialogState::Select;
            }
        }
    }

    void StartGameDialog::render(Renderer& renderer, int ox, int oy)
    {
        // render background filter
        if (!startLoading) {

            int XRes = 640;
            int YRes = 480;

            IntRect rb(0, 0, XRes, YRes);
            renderer.drawColourRect(rb, Colour(0, 0, 0, 180), false);
        }

        Dialog::render(renderer, ox, oy);
    }
}
