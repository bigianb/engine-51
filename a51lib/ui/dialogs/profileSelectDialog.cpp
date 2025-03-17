#include "profileSelectDialog.h"
#include "../UIButton.h"
#include "../UIFont.h"
#include "../../system/Renderer.h"
#include "../../state/PlayerProfile.h"

namespace ui
{
    ProfileSelectDialog::ProfileSelectDialog()
    {
    }

    ProfileSelectDialog::~ProfileSelectDialog()
    {
        // TODO: why doesn't this call destroy?
    }

    static ControlTemplate controls[] =
        {
            {ProfileSelectDialog::IDC_PROFILE_SELECT_LISTBOX, "IDS_PROFILE_PROFILES", "listbox", 45, 40, 240, 206, 0, 0, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            //   {ProfileSelectDialog::IDC_PROFILE_SELECT_INFOBOX, "IDS_PROFILE_INFO", "blankbox", 45, 256, 240, 76, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},

            {ProfileSelectDialog::IDC_PROFILE_CARD_SLOT, "IDS_NULL", "text", 53, 278, 120, 16, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {ProfileSelectDialog::IDC_PROFILE_CREATE_DATE, "IDS_PROFILE_CREATE_DATE", "text", 53, 294, 120, 16, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {ProfileSelectDialog::IDC_PROFILE_MODIFIED_DATE, "IDS_PROFILE_MODIFIED_DATE", "text", 53, 310, 120, 16, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},

            {ProfileSelectDialog::IDC_PROFILE_INFO_CREATE_DATE, "IDS_NULL", "text", 157, 294, 80, 16, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {ProfileSelectDialog::IDC_PROFILE_INFO_MODIFIED_DATE, "IDS_NULL", "text", 157, 310, 80, 16, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},

            {ProfileSelectDialog::IDC_PROFILE_SELECT_NAV_TEXT, "IDS_NULL", "text", 0, 0, 0, 0, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_PROFILE_MAIN_MENU",
            1, 9,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags)
    {
        ProfileSelectDialog* dialog = new ProfileSelectDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags);

        return dialog;
    }

    void ProfileSelectDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("profile select", &thisDialogTemplate, &dlg_factory);
    }

    bool ProfileSelectDialog::create(User*           user,
                                     Manager*        manager,
                                     DialogTemplate* dialogTemplate,
                                     const IntRect&  position,
                                     Window*         parent,
                                     int             flags)
    {
        bool success = false;
        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        profileList = (ui::ListBox*)findChildById(IDC_PROFILE_SELECT_LISTBOX);
        navText = (ui::Text*)findChildById(IDC_PROFILE_SELECT_NAV_TEXT);

        //gotoControl(profileList);
        currentHighlight = 0;

        navText->clearFlag(Window::WF_VISIBLE);

        std::wstring wNavText = manager->lookupString("ui", "IDS_NAV_SELECT");
        wNavText += manager->lookupString("ui", "IDS_NAV_BACK");
        wNavText += manager->lookupString("ui", "IDS_NAV_DELETE");
        navText->setLabel(wNavText);
        navText->setLabelFlags(Font::h_center | Font::v_top | Font::is_help_text);
        navText->setUseSmallText();

        // set up level list
        profileList->setFlag(Window::WF_SELECTED);
        profileList->SetBackgroundColor(Colour(39, 117, 28, 128));
        profileList->DisableFrame();
        profileList->SetExitOnSelect(false);
        profileList->SetExitOnBack(true);
        profileList->EnableHeaderBar();
        profileList->SetHeaderBarColor(Colour(19, 59, 14, 196));
        profileList->SetHeaderColor(Colour(255, 252, 204, 255));

        initScreenScaling(position);

        state = DialogState::Active;
        return success;
    }

    void ProfileSelectDialog::render(Renderer& renderer, int ox, int oy)
    {
        static int offset = 0;
        int        gap = 9;
        int        width = 4;
        auto*      manager = getUIManger();

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

        Dialog::render(renderer, ox, oy);
        getUIManger()->renderGlowBar(renderer);
    }

    void ProfileSelectDialog::onUpdate(float deltaTime)
    {
        int highlight = -1;
        if (getUIManger()->isScreenScaling()) {
            if (!updateScreenScaling(deltaTime, false)) {
                navText->setFlag(Window::WF_VISIBLE);

                //gotoControl(profileList);
                //getUIManger()->setScreenHighlight(profileList->getPosition());

                auto* manager = getUIManger();
                if (!manager->isScreenOn()) {
                    // enable the frame
                    clearFlag(WF_DISABLED);
                    manager->setScreenOn(true);
                }
            }
        }

        getUIManger()->updateGlowBar(deltaTime);
    }

    void ProfileSelectDialog::onPadSelect(Window* win)
    {
        if (state == DialogState::Active) {
            if (profileList->GetSelectedItemData(1) != PROFILE_OK) {
                //g_AudioMgr.Play( "InvalidEntry" );
                return;
            }
        }
    }
}
