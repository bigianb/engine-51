#include "profileSelectDialog.h"
#include "../UIButton.h"
#include "../UIFont.h"
#include "../../system/Renderer.h"
#include "../../state/PlayerProfile.h"

#include <ctime>

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

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags, void* userData)
    {
        ProfileSelectDialog* dialog = new ProfileSelectDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags, (StateMachine*)userData);

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
                                     int flags, StateMachine* sm)
    {
        stateMachine = sm;
        bool success = false;
        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        profileList = (ui::ListBox*)findChildById(IDC_PROFILE_SELECT_LISTBOX);
        navText = (ui::Text*)findChildById(IDC_PROFILE_SELECT_NAV_TEXT);

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

        m_pCardSlot = (Text*)findChildById(IDC_PROFILE_CARD_SLOT);
        m_pCreationDate = (Text*)findChildById(IDC_PROFILE_CREATE_DATE);
        m_pModifiedDate = (Text*)findChildById(IDC_PROFILE_MODIFIED_DATE);
        m_pInfoCreationDate = (Text*)findChildById(IDC_PROFILE_INFO_CREATE_DATE);
        m_pInfoModifiedDate = (Text*)findChildById(IDC_PROFILE_INFO_MODIFIED_DATE);

        m_pCardSlot->setUseSmallText();
        m_pCreationDate->setUseSmallText();
        m_pModifiedDate->setUseSmallText();
        m_pInfoCreationDate->setUseSmallText();
        m_pInfoModifiedDate->setUseSmallText();

        m_pCardSlot->setLabelFlags(Font::h_left | Font::v_center);
        m_pCreationDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pModifiedDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pInfoCreationDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pInfoModifiedDate->setLabelFlags(Font::h_left | Font::v_center);

        m_pCardSlot->clearFlag(Window::WF_VISIBLE);
        m_pCreationDate->clearFlag(Window::WF_VISIBLE);
        m_pModifiedDate->clearFlag(Window::WF_VISIBLE);
        m_pInfoCreationDate->clearFlag(Window::WF_VISIBLE);
        m_pInfoModifiedDate->clearFlag(Window::WF_VISIBLE);

        m_pCardSlot->setLabelColour(Colour(255, 252, 204, 255));
        m_pCreationDate->setLabelColour(Colour(255, 252, 204, 255));
        m_pModifiedDate->setLabelColour(Colour(255, 252, 204, 255));
        m_pInfoCreationDate->setLabelColour(Colour(255, 252, 204, 255));
        m_pInfoModifiedDate->setLabelColour(Colour(255, 252, 204, 255));

        m_Type = PROFILE_SELECT_NORMAL;
        m_bEditProfile = false;

        // get profile data
        refreshProfileList();
        m_ProfileName = manager->lookupString("ui", "IDS_PROFILE_DEFAULT_PLAYER");
        m_ProfileEntered = false;
        m_ProfileOk = false;

        currentHighlight = 0;
        gotoControl(profileList);
        m_PopUp = nullptr;
        m_BlocksRequired = 0;

        m_BackupPopup = nullptr;
        initScreenScaling(position);

        m_bRenderBlackout = false;

        state = DialogState::Active;
        return success;
    }

    void ProfileSelectDialog::refreshProfileList()
    {
        bool Found = false;
        int  Profile1;

        // get the hash for the selected profile
        if (m_Type == PROFILE_SELECT_OVERWRITE) {
            Profile1 = stateMachine->getSelectedProfile(stateMachine->getPendingProfileIndex());
        } else {
            Profile1 = stateMachine->getSelectedProfile(0);
        }

        // get the profile list
        std::vector<profile_info*>& ProfileNames = stateMachine->getProfileList();

        // store the current selection
        int CurrentSelection = profileList->GetSelection();

        // clear the list
        profileList->DeleteAllItems();

        auto* manager = getUIManger();

        // get the current list from the memcard manager
        //g_UIMemCardMgr.GetProfileNames(ProfileNames);
        /*
                // fill it with the profile information
                for (int i = 0; i < ProfileNames.GetCount(); i++) {
                    if (ProfileNames[i]->bDamaged) {

                        // add the profile to the list
                        profileList->AddItem(g_StringTableMgr("ui", "IDS_CORRUPT"), i, PROFILE_CORRUPT);
                        profileList->SetItemColor(i, XCOLOR_RED);

                    } else if (ProfileNames[i]->Ver != PROFILE_VERSION_NUMBER) {

                        // add the profile to the list
                        //m_pProfileList->AddItem( g_StringTableMgr( "ui", "IDS_BAD_VERSION" ), i, PROFILE_EXPIRED ); // not for retail
                        profileList->AddItem(g_StringTableMgr("ui", "IDS_CORRUPT"), i, PROFILE_CORRUPT);
                        profileList->SetItemColor(i, XCOLOR_RED);

                    } else {
                        // add the profile to the list
                        profileList->AddItem(ProfileNames[i]->Name, i, PROFILE_OK);
                    }

                    // look for a match for the selected profile
                    if (Profile1 != 0) {
                        if (ProfileNames[i]->Hash == Profile1) {
                            if (CurrentSelection == -1) {
                                CurrentSelection = i;
                            }
                            Found = true;
                        }
                    }
                }
        */
        // add a create option
        m_CreateIndex = ProfileNames.size();
        profileList->AddItem(manager->lookupString("ui", "IDS_PROFILE_CREATE_NEW"), m_CreateIndex);

        if ((profileList->GetItemCount() > CurrentSelection) && (CurrentSelection >= 0)) {
            profileList->SetSelection(CurrentSelection);
        } else {
            profileList->SetSelection(0);
        }

        // populate profile info
        int SelIndex = profileList->GetSelection();
        if (SelIndex == m_CreateIndex) {
            // new profile, blank fields
            m_pCardSlot->setLabel(manager->lookupString("ui", "IDS_NULL"));
            m_pInfoCreationDate->setLabel(L"---");
            m_pInfoModifiedDate->setLabel(L"---");
        } else {
            // set the profile info
            if (ProfileNames[SelIndex]->CardID == 0) {
                m_pCardSlot->setLabel(manager->lookupString("ui", "IDS_PROFILE_CARD_SLOT_1"));
            } else {
                m_pCardSlot->setLabel(manager->lookupString("ui", "IDS_PROFILE_CARD_SLOT_2"));
            }

            auto* cd = localtime(&ProfileNames[SelIndex]->CreationDate);
            char buf[64];
            snprintf(buf, 63, "IDS_MONTH%d", cd->tm_mon);
            std::wstring Month = manager->lookupString("ui", buf);

            wchar_t buf2[128];
            swprintf ( buf2, 127, L"%02i:%02i:%02i %s %02i", cd->tm_hour, cd->tm_min, cd->tm_sec, Month.c_str(), cd->tm_mday );
            m_pInfoCreationDate->setLabel(buf2);

            cd = localtime(&ProfileNames[SelIndex]->ModifiedDate);
            
            snprintf(buf, 63, "IDS_MONTH%d", cd->tm_mon);
            Month = manager->lookupString("ui", buf);
            swprintf ( buf2, 127, L"%02i:%02i:%02i %s %02i", cd->tm_hour, cd->tm_min, cd->tm_sec, Month.c_str(), cd->tm_mday );
            m_pInfoModifiedDate->setLabel(buf2);
        }
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

                gotoControl(profileList);
                getUIManger()->setScreenHighlight(profileList->getPosition());

                auto* manager = getUIManger();
                if (!manager->isScreenOn()) {
                    // enable the frame
                    clearFlag(WF_DISABLED);
                    manager->setScreenOn(true);
                }
            }
        }

        getUIManger()->updateGlowBar(deltaTime);

        if (m_ProfileEntered) {
            m_ProfileEntered = false;

            if (m_ProfileOk) {
                m_ProfileOk = false;

                // check for duplicate name entry
                /*
                for (int p = 0; p < profileList->GetItemCount(); p++) {
                    if (x_wstrcmp(m_pProfileList->GetItemLabel(p), m_ProfileName) == 0) {
                        // open duplicate name error popup
                        irect r = g_UiMgr->GetUserBounds(g_UiUserID);
                        m_PopUp = (dlg_popup*)g_UiMgr->OpenDialog(m_UserID, "popup", r, NULL, ui_win::WF_VISIBLE | ui_win::WF_BORDER | ui_win::WF_DLG_CENTER | WF_INPUTMODAL | ui_win::WF_USE_ABSOLUTE);
                        m_PopUpType = POPUP_TYPE_BADNAME;

                        // set nav text
                        xwstring navText(g_StringTableMgr("ui", "IDS_NAV_OK"));
                        m_pNavText->SetFlag(ui_win::WF_VISIBLE, FALSE);

                        m_PopUp->Configure(g_StringTableMgr("ui", "IDS_PROFILE_DUPLICATE_NAME"),
                                           TRUE,
                                           FALSE,
                                           FALSE,
                                           g_StringTableMgr("ui", "IDS_PROFILE_DUPLICATE_NAME_MSG"),
                                           navText,
                                           &m_PopUpResult);

                        return;
                    }
                }

                // store the new profile name
                g_StateMgr.InitPendingProfile(0);
                player_profile& NewProfile = g_StateMgr.GetPendingProfile();
                NewProfile.SetProfileName(xstring(m_ProfileName));
*/
                // go to the profile options screen
                state = DialogState::Active;
                return;
            } else {
                // re-enable dialog
                state = DialogState::Active;
            }
        }
        int index = profileList->GetSelectedItemData();
        if (m_PopUp) {
            // TODO
        } else if (m_BackupPopup) {
            //UpdateBackupPopup();
        } else {
            if (state == DialogState::Active) {
                refreshProfileList();

                // get current index
                index = profileList->GetSelectedItemData();

                // TODO: update nav text
            }
        }
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
