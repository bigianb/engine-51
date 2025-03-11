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

        buttonCampaign = (ui::Button*)findChildById(IDC_MAIN_MENU_CAMPAIGN);
        buttonMultiPlayer = (ui::Button*)findChildById(IDC_MAIN_MENU_MULTI);
        buttonOnline = (ui::Button*)findChildById(IDC_MAIN_MENU_ONLINE);
        buttonSettings = (ui::Button*)findChildById(IDC_MAIN_MENU_SETTINGS);
        buttonProfiles = (ui::Button*)findChildById(IDC_MAIN_MENU_PROFILES);
        buttonCredits = (ui::Button*)findChildById(IDC_MAIN_MENU_CREDITS);
        navText = (ui::Text*)findChildById(IDC_MAIN_MENU_NAV_TEXT);

        gotoControl(buttonCampaign);
        currentHighlight = 0;

        buttonCampaign->clearFlag(Window::WF_VISIBLE);
        buttonMultiPlayer->clearFlag(Window::WF_VISIBLE);
        buttonOnline->clearFlag(Window::WF_VISIBLE);
        buttonSettings->clearFlag(Window::WF_VISIBLE);
        buttonProfiles->clearFlag(Window::WF_VISIBLE);
        buttonCredits->clearFlag(Window::WF_VISIBLE);
        navText->clearFlag(Window::WF_VISIBLE);

        std::wstring wNavText = manager->lookupString("ui", "IDS_NAV_SELECT");
        navText->setLabel(wNavText);
        navText->setLabelFlags(Font::h_center | Font::v_top | Font::is_help_text);
        navText->setUseSmallText();

        // TODO g_PendingConfig.SetPlayerCount( 0 );

        if (!getUIManger()->isScreenOn()) {
            setFlag(WF_DISABLED);
        }

        initScreenScaling(position);

        //popUp = nullptr;

        state = DialogState::Active;
        return success;
    }

    void MainMenuDialog::render(Renderer& renderer, int ox, int oy)
    {
        static int offset = 0;
        int        gap = 9;
        int        width = 4;
        auto*      manager = getUIManger();

        if (manager->isScreenOn()) {
            IntRect rb;
            rb.left = m_CurrPos.left + 22;
            rb.top = m_CurrPos.top;
            rb.right = m_CurrPos.right - 23;
            rb.bottom = m_CurrPos.bottom;

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
        }
        Dialog::render(renderer, ox, oy);
        getUIManger()->renderGlowBar(renderer);
    }

    void MainMenuDialog::onUpdate(float deltaTime)
    {
        int highlight = -1;
        if (getUIManger()->isScreenScaling()) {
            if (!updateScreenScaling(deltaTime, false)) {
                buttonCampaign->setFlag(Window::WF_VISIBLE);
                buttonMultiPlayer->setFlag(Window::WF_VISIBLE);
                buttonOnline->setFlag(Window::WF_VISIBLE);
                buttonSettings->setFlag(Window::WF_VISIBLE);
                buttonProfiles->setFlag(Window::WF_VISIBLE);
                buttonCredits->setFlag(Window::WF_VISIBLE);
                navText->setFlag(Window::WF_VISIBLE);

                gotoControl(buttonCampaign);
                buttonCampaign->setFlag(WF_HIGHLIGHT);
                getUIManger()->setScreenHighlight(buttonCampaign->getPosition());
                currentControl = IDC_MAIN_MENU_CAMPAIGN;

                auto* manager = getUIManger();
                if (!manager->isScreenOn()) {
                    // enable the frame
                    clearFlag(WF_DISABLED);
                    manager->setScreenOn(true);
                }
            }
        }

        getUIManger()->updateGlowBar(deltaTime);
        if (buttonCampaign->highlighted()) {
            getUIManger()->setScreenHighlight(buttonCampaign->getPosition());
            highlight = 0;
        } else if (buttonMultiPlayer->highlighted()) {
            getUIManger()->setScreenHighlight(buttonMultiPlayer->getPosition());
            highlight = 1;
        } else if (buttonOnline->highlighted()) {
            getUIManger()->setScreenHighlight(buttonOnline->getPosition());
            highlight = 2;
        } else if (buttonSettings->highlighted()) {
            getUIManger()->setScreenHighlight(buttonSettings->getPosition());
            highlight = 3;
        } else if (buttonProfiles->highlighted()) {
            getUIManger()->setScreenHighlight(buttonProfiles->getPosition());
            highlight = 4;
        } else if (buttonCredits->highlighted()) {
            getUIManger()->setScreenHighlight(buttonCredits->getPosition());
            highlight = 5;
        }
        if (highlight != currentHighlight) {
            if (highlight != -1) {
                //g_AudioMgr.Play("Cusor_Norm");
            }
            currentHighlight = highlight;
        }
    }

    void MainMenuDialog::onPadSelect(Window* win)
    {
        if (state == DialogState::Active) {
            if (win == buttonCampaign) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_CAMPAIGN;
                state = DialogState::Select;
            } else if (win == buttonMultiPlayer) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_MULTI;
                state = DialogState::Select;
            } else if (win == buttonOnline) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_ONLINE;
                state = DialogState::Select;
            } else if (win == buttonSettings) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_SETTINGS;
                state = DialogState::Select;
            } else if (win == buttonProfiles) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_PROFILES;
                state = DialogState::Select;
            } else if (win == buttonCredits) {
                //g_AudioMgr.Play("Select_Norm");
                currentControl = IDC_MAIN_MENU_CREDITS;
                state = DialogState::Select;
            }
        }
    }
}
