#include "profileSelectDialog.h"
#include "../UIButton.h"
#include "../UIFont.h"
#include "../../system/Renderer.h"
#include "../../state/PlayerProfile.h"

#include <ctime>
#include <cassert>

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

        profileList->clearFlag(Window::WF_VISIBLE);
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

        m_pCreationDate = (Text*)findChildById(IDC_PROFILE_CREATE_DATE);
        m_pModifiedDate = (Text*)findChildById(IDC_PROFILE_MODIFIED_DATE);
        m_pInfoCreationDate = (Text*)findChildById(IDC_PROFILE_INFO_CREATE_DATE);
        m_pInfoModifiedDate = (Text*)findChildById(IDC_PROFILE_INFO_MODIFIED_DATE);

        m_pCreationDate->setUseSmallText();
        m_pModifiedDate->setUseSmallText();
        m_pInfoCreationDate->setUseSmallText();
        m_pInfoModifiedDate->setUseSmallText();

        m_pCreationDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pModifiedDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pInfoCreationDate->setLabelFlags(Font::h_left | Font::v_center);
        m_pInfoModifiedDate->setLabelFlags(Font::h_left | Font::v_center);

        m_pCreationDate->clearFlag(Window::WF_VISIBLE);
        m_pModifiedDate->clearFlag(Window::WF_VISIBLE);
        m_pInfoCreationDate->clearFlag(Window::WF_VISIBLE);
        m_pInfoModifiedDate->clearFlag(Window::WF_VISIBLE);

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

        // store the current selection
        int CurrentSelection = profileList->GetSelection();

        // clear the list
        profileList->DeleteAllItems();

        auto* manager = getUIManger();

        stateMachine->readProfiles();

        // fill it with the profile information
        int i=0;
        for (auto& profileInfo : stateMachine->getProfileList()) {
            if (profileInfo.bDamaged || profileInfo.Ver != PROFILE_VERSION_NUMBER) {
                profileList->AddItem(manager->lookupString("ui", "IDS_CORRUPT"), i, PROFILE_CORRUPT);
                profileList->SetItemColor(i, COLOR_RED);
            } else {
                profileList->AddItem(profileInfo.Name, i, PROFILE_OK);
            }

            // look for a match for the selected profile
            if (Profile1 != 0) {
                if (profileInfo.Hash == Profile1) {
                    if (CurrentSelection == -1) {
                        CurrentSelection = i;
                    }
                    Found = true;
                }
            }
            ++i;
        }
        // add a create option
        m_CreateIndex = stateMachine->getProfileList().size();
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
            m_pInfoCreationDate->setLabel(L"---");
            m_pInfoModifiedDate->setLabel(L"---");
        } else {
            auto& profileItem = stateMachine->getProfileList()[SelIndex];
            auto* cd = localtime(&profileItem.CreationDate);
            char  buf[64];
            snprintf(buf, 63, "IDS_MONTH%d", cd->tm_mon + 1);
            std::wstring Month = manager->lookupString("ui", buf);

            wchar_t buf2[128];
            swprintf(buf2, 127, L"%02i:%02i:%02i %ls %02i", cd->tm_hour, cd->tm_min, cd->tm_sec, Month.c_str(), cd->tm_mday);
            m_pInfoCreationDate->setLabel(buf2);

            cd = localtime(&profileItem.ModifiedDate);

            snprintf(buf, 63, "IDS_MONTH%d", cd->tm_mon + 1);
            Month = manager->lookupString("ui", buf);
            swprintf(buf2, 127, L"%02i:%02i:%02i %ls %02i", cd->tm_hour, cd->tm_min, cd->tm_sec, Month.c_str(), cd->tm_mday);
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
                profileList->setFlag(Window::WF_VISIBLE);
                // m_pProfileDetails   ->setFlag(Window::WF_VISIBLE);
                m_pCreationDate->setFlag(Window::WF_VISIBLE);
                m_pModifiedDate->setFlag(Window::WF_VISIBLE);
                m_pInfoCreationDate->setFlag(Window::WF_VISIBLE);
                m_pInfoModifiedDate->setFlag(Window::WF_VISIBLE);
                navText->setFlag(Window::WF_VISIBLE);

                gotoControl(profileList);
                getUIManger()->setScreenHighlight(profileList->getPosition());
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
            int index = profileList->GetSelectedItemData();
            if( index < m_CreateIndex ){
                if( m_Type == PROFILE_SELECT_OVERWRITE )
                {
                    assert(false); // TODO
                } else {
                    // init the pending profile
                    stateMachine->initPendingProfile( 0 ); // always player 0 in campaign

                    // get the profile list
                    auto& ProfileNames = stateMachine->getProfileList();

                    // store the id of the selected profile
                    stateMachine->setSelectedProfile( 0, ProfileNames[index].Hash );

                    // attempt to load the selected profile
                    //m_iCard = ProfileNames[index].CardID;
                    //g_UIMemCardMgr.LoadProfile( *ProfileNames[index], 0, this, &dlg_profile_select::OnLoadProfileCB );

                    // change the dialog state to wait for the memcard
                    //state = DIALOG_STATE_WAIT_FOR_MEMCARD;

                    // hack to bypass memcard
                    stateMachine->setProfileNotSaved( stateMachine->getPendingProfileIndex(), false );
                    stateMachine->activatePendingProfile();

                    state = DialogState::Select;
                }
            }
        }
    }
}