#include "mainMenuDialog.h"
#include "../UIButton.h"
#include "../UIFont.h"
#include "../../system/Renderer.h"

namespace ui
{
    MainMenuDialog::MainMenuDialog()
    {
    }

    MainMenuDialog::~MainMenuDialog()
    {
        // TODO: why doesn't this call destroy?
    }

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

    static ControlTemplate controls[] =
        {
            {IDC_MAIN_MENU_CAMPAIGN, "IDS_MAIN_MENU_CAMPAIGN", "button", 60, 60, 120, 40, 0, 0, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_MULTI, "IDS_MAIN_MENU_MULTI", "button", 60, 100, 120, 40, 0, 1, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_ONLINE, "IDS_MAIN_MENU_ONLINE", "button", 60, 140, 120, 40, 0, 2, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_SETTINGS, "IDS_MAIN_MENU_SETTINGS", "button", 60, 180, 120, 40, 0, 3, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_PROFILES, "IDS_MAIN_MENU_PROFILES", "button", 60, 220, 120, 40, 0, 4, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_CREDITS, "IDS_EXTRAS_ITEM_CREDITS", "button", 60, 260, 120, 40, 0, 5, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {IDC_MAIN_MENU_NAV_TEXT, "IDS_NULL", "text", 0, 0, 0, 0, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_MAIN_MENU",
            1, 9,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags)
    {
        MainMenuDialog* dialog = new MainMenuDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    void MainMenuDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("main menu", &thisDialogTemplate, &dlg_factory);
    }

    bool MainMenuDialog::create(User*           user,
                                Manager*        manager,
                                DialogTemplate* dialogTemplate,
                                const IntRect&  position,
                                Window*         parent,
                                int             flags)
    {
        bool success = false;
        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        ui::Button* buttonCampaign = (ui::Button*)findChildById(IDC_MAIN_MENU_CAMPAIGN);
        gotoControl(buttonCampaign);

        ui::Text*    navText = (ui::Text*)findChildById(IDC_MAIN_MENU_NAV_TEXT);
        std::wstring wNavText = manager->lookupString("ui", "IDS_NAV_SELECT");
        navText->setLabel(wNavText);
        navText->setLabelFlags(Font::h_center | Font::v_top | Font::is_help_text);
        navText->setUseSmallText();

        initScreenScaling(position);

        state = DialogState::Active;
        return success;
    }

    void MainMenuDialog::render(Renderer& renderer, int ox, int oy)
    {
        static int offset = 0;
        int gap = 9;
        int width = 4;

        IntRect rb;
        rb.left = m_CurrPos.left + 22;
        rb.top = m_CurrPos.top;
        rb.right = m_CurrPos.right - 23;
        rb.bottom = m_CurrPos.bottom;

        auto* manager = getUIManger();
        renderer.drawColourRect(rb, Colour(56, 115, 58, 64), false);

        // render the screen bars
        int y = rb.top + offset;

        while (y < rb.bottom) {
            IntRect bar;

            if ((y + width) > rb.bottom) {
                bar.set(rb.left, y, rb.right, rb.bottom);
            } else {
                bar.set(rb.left, y, rb.right, y + width);
            }

            // draw the bar
            renderer.drawColourRect(bar, Colour(56, 115, 58, 30), false);

            y += gap;
        }

        // increment the offset
        // TODO: should not be static. onUpdate should update this taking deltaTime into account.
        if (++offset > 9) {
            offset = 0;
        }
        renderer.drawEnd();
        Dialog::render(renderer, ox, oy);
        getUIManger()->renderGlowBar(renderer);
    }

    void MainMenuDialog::onUpdate(float deltaTime)
    {
        if (getUIManger()->isScreenScaling()) {
            if (!updateScreenScaling(deltaTime, false)) {
                Control* control = gotoControl(currentControl);
                if (control) {
                    control->setFlag(WF_HIGHLIGHT);
                    getUIManger()->setScreenHighlight(control->getPosition());
                }
            }
        }

        getUIManger()->updateGlowBar(deltaTime);
    }

    void MainMenuDialog::onPadSelect()
    {
        if (state == DialogState::Active) {
        }
    }
}
