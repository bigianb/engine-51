#include "campaignMenuDialog.h"
#include "../UIButton.h"
#include "../UIFont.h"
#include "../../system/Renderer.h"
#include "../../state/PlayerProfile.h"
#include "../../state/MapList.h"
#include <ctime>
#include <cassert>

namespace ui
{
    CampaignMenuDialog::CampaignMenuDialog()
    {
    }

    CampaignMenuDialog::~CampaignMenuDialog()
    {
        destroy();
    }

    static ControlTemplate controls[] =
        {
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_NEW_CAMPAIGN, "IDS_CAMPAIGN_MENU_NEW_CAMPAIGN", "button", 80, 60, 120, 40, 0, 0, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_RESUME_CAMPAIGN, "IDS_CAMPAIGN_MENU_RESUME_CAMPAIGN", "button", 80, 100, 120, 40, 0, 1, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_EDIT_PROFILE, "IDS_CAMPAIGN_MENU_EDIT_PROFILE", "button", 80, 140, 120, 40, 0, 2, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_LORE, "IDS_CAMPAIGN_MENU_LORE", "button", 80, 180, 120, 40, 0, 3, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_SECRETS, "IDS_CAMPAIGN_MENU_SECRETS", "button", 80, 220, 120, 40, 0, 4, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_EXTRAS, "IDS_CAMPAIGN_MENU_EXTRAS", "button", 80, 260, 120, 40, 0, 5, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_LEVEL_SELECT, "IDS_CAMPAIGN_MENU_LEVEL_SELECT", "button", 80, 300, 120, 40, 0, 6, 1, 1, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
            {CampaignMenuDialog::IDC_CAMPAIGN_MENU_NAV_TEXT, "IDS_NULL", "text", 0, 0, 0, 0, 0, 0, 0, 0, Window::WF_VISIBLE | Window::WF_SCALE_XPOS | Window::WF_SCALE_XSIZE},
    };

    static DialogTemplate thisDialogTemplate =
        {
            "IDS_CAMPAIGN_MENU",
            1, 9,
            sizeof(controls) / sizeof(ControlTemplate),
            &controls[0],
            0};

    static Dialog* dlg_factory(User* user, Manager* manager, DialogTemplate* dialogTemplate, const IntRect& position, Window* parent, int flags, void* userData)
    {
        CampaignMenuDialog* dialog = new CampaignMenuDialog;
        dialog->create(user, manager, dialogTemplate, position, parent, flags, (StateMachine*)userData);

        return dialog;
    }

    void CampaignMenuDialog::registerDialog(Manager* manager)
    {
        manager->registerDialogClass("campaign menu", &thisDialogTemplate, &dlg_factory);
    }

    bool CampaignMenuDialog::create(User*           user,
                                    Manager*        manager,
                                    DialogTemplate* dialogTemplate,
                                    const IntRect&  position,
                                    Window*         parent,
                                    int flags, StateMachine* sm)
    {
        stateMachine = sm;
        bool success = false;
        success = Dialog::create(user, manager, dialogTemplate, position, parent, flags);

        m_pButtonNewCampaign = (Button*)findChildById(IDC_CAMPAIGN_MENU_NEW_CAMPAIGN);
        m_pButtonResumeCampaign = (Button*)findChildById(IDC_CAMPAIGN_MENU_RESUME_CAMPAIGN);
        m_pButtonEditProfile = (Button*)findChildById(IDC_CAMPAIGN_MENU_EDIT_PROFILE);
        m_pButtonLore = (Button*)findChildById(IDC_CAMPAIGN_MENU_LORE);
        m_pButtonSecrets = (Button*)findChildById(IDC_CAMPAIGN_MENU_SECRETS);
        m_pButtonExtras = (Button*)findChildById(IDC_CAMPAIGN_MENU_EXTRAS);
        m_pButtonLevelSelect = (Button*)findChildById(IDC_CAMPAIGN_MENU_LEVEL_SELECT);
        m_pNavText = (Text*)findChildById(IDC_CAMPAIGN_MENU_NAV_TEXT);

        m_CurrHL = 0;
        m_PopUp = NULL;
        // m_PopUpResult       = DLG_POPUP_IDLE;
        m_bCheckKeySequence = false;

        m_pButtonNewCampaign->clearFlag(Window::WF_VISIBLE);
        m_pButtonResumeCampaign->clearFlag(Window::WF_VISIBLE);
        m_pButtonEditProfile->clearFlag(Window::WF_VISIBLE);
        m_pButtonLore->clearFlag(Window::WF_VISIBLE);
        m_pButtonSecrets->clearFlag(Window::WF_VISIBLE);
        m_pButtonExtras->clearFlag(Window::WF_VISIBLE);
        m_pButtonLevelSelect->clearFlag(Window::WF_VISIBLE);
        m_pNavText->clearFlag(Window::WF_VISIBLE);

        auto& Profile = stateMachine->getActiveProfile(0);
        bool  DisableCheckpoints = true;

        for (int i = 0; i < MAX_SAVED_LEVELS; i++) {
            level_check_points& Checkpoint = Profile.GetCheckpoint(i);

            if (Checkpoint.MapID != -1) {
                DisableCheckpoints = false;
                break;
            }
        }

        m_pButtonResumeCampaign->setFlag(Window::WF_DISABLED, DisableCheckpoints);

        bool      bFoundLore = false;
        int       m = 0;
        map_list* mapList = sm->getMapList();
        while ((m < mapList->GetCount()) && !bFoundLore) {
            const map_entry& Entry = *mapList->GetByIndex(m);
            int              MapID = Entry.GetMapID();

            // check if of the correct game type
            if ((Entry.GetGameType() == GAME_CAMPAIGN) && (MapID < 2000)) {
                // look up the vault by the mapID
                /*
                int VaultIndex;
                g_LoreList.GetVaultByMapID( MapID, VaultIndex );

                // see if we unlocked ANYTHING in this vault
                if( Profile.GetLoreAcquired( VaultIndex, -1 ) )
                {
                    // found some lore!
                    bFoundLore = true;
                }
                    */
            }

            // check next map
            m++;
        }

        // check if any lore or secrets are unlocked
        if (!bFoundLore) {
            // no lore, therefore no secrets either!
            m_pButtonLore->setFlag(Window::WF_DISABLED);
            m_pButtonSecrets->setFlag(Window::WF_DISABLED);
        } else {
            // check for new lore unlocked
            if (Profile.IsNewLoreUnlocked()) {
                //m_pButtonLore->enablePulse();
            }

            // some lore is unlocked, check for secrets
            if (Profile.GetNumSecretsUnlocked() == 0) {
                m_pButtonSecrets->setFlag(Window::WF_DISABLED);
            } else if (Profile.IsNewSecretUnlocked()) {
                //m_pButtonSecrets->enablePulse();
            }
        }
/*
        int iControl = stateMachine->getCurrentControl();
        if( (iControl == -1) || (gotoControl( iControl )==nullptr) )
        {
            if( DisableCheckpoints )
            {
                gotoControl( (ui_control*)m_pButtonNewCampaign );
                m_CurrentControl =  IDC_CAMPAIGN_MENU_NEW_CAMPAIGN;
            }
            else
            {
                gotoControl( (ui_control*)m_pButtonResumeCampaign );
                m_CurrentControl =  IDC_CAMPAIGN_MENU_RESUME_CAMPAIGN;
            }
        }
        else
        {
            m_CurrentControl = iControl;
        }*/
        initScreenScaling( position );
        state = DialogState::Active;
        return success;
    }

    void CampaignMenuDialog::render(Renderer& renderer, int ox, int oy)
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

    void CampaignMenuDialog::onUpdate(float deltaTime)
    {
        int highlight = -1;
        if (getUIManger()->isScreenScaling()) {
            if (!updateScreenScaling(deltaTime, false)) {
                m_pButtonNewCampaign->setFlag(Window::WF_VISIBLE);
                m_pButtonResumeCampaign->setFlag(Window::WF_VISIBLE);
                m_pButtonEditProfile->setFlag(Window::WF_VISIBLE);
                m_pButtonLore->setFlag(Window::WF_VISIBLE);
                m_pButtonSecrets->setFlag(Window::WF_VISIBLE);
                m_pButtonExtras->setFlag(Window::WF_VISIBLE);
                m_pButtonLevelSelect->setFlag(Window::WF_VISIBLE);
                m_pNavText->setFlag(Window::WF_VISIBLE);

                int iControl = stateMachine->getCurrentControl();
                if ((iControl == -1) || (gotoControl(iControl) == nullptr)) {
                    if (m_pButtonResumeCampaign->disabled()) {
                        gotoControl(m_pButtonNewCampaign);
                        m_pButtonNewCampaign->setFlag(WF_HIGHLIGHT);
                        getUIManger()->setScreenHighlight(m_pButtonNewCampaign->getPosition());
                        currentControl = IDC_CAMPAIGN_MENU_NEW_CAMPAIGN;
                    } else {
                        gotoControl(m_pButtonResumeCampaign);
                        m_pButtonResumeCampaign->setFlag(WF_HIGHLIGHT);
                        getUIManger()->setScreenHighlight(m_pButtonResumeCampaign->getPosition());
                        currentControl = IDC_CAMPAIGN_MENU_RESUME_CAMPAIGN;
                    }
                } else {
                    ui::Control* pControl = gotoControl(iControl);
                    pControl->setFlag(WF_HIGHLIGHT);
                    getUIManger()->setScreenHighlight(pControl->getPosition());
                    currentControl = iControl;
                }
            }
        }
        /*
                if (m_PopUp) {
                    switch (m_PopUpResult) {
                    case DLG_POPUP_IDLE:
                        break;

                    case DLG_POPUP_YES:
                    {
                        // clear the checkpoints
                        player_profile& Profile = g_StateMgr.GetActiveProfile(0);
                        for (s32 i = 0; i < MAX_SAVED_LEVELS; i++) {
                            // reinitialize checkpoint data
                            Profile.GetCheckpoint(i).Init(-1);
                        }
                        m_CurrentControl = IDC_CAMPAIGN_MENU_NEW_CAMPAIGN;
                        m_State = DIALOG_STATE_SELECT;
                        m_PopUp = NULL;
                        return;
                    }

                    default:
                        // Abort new campaign
                        m_PopUp = NULL;

                        // set controllers back to all
                        if (g_StateMgr.GetActiveControllerID() != -1) {
                            g_StateMgr.SetControllerRequested(g_StateMgr.GetActiveControllerID(), FALSE);
                            g_StateMgr.SetActiveControllerID(-1);
                        }

                        break;
                    }
                }
                    */

        // update button pulse
        m_pButtonLore->onUpdate(deltaTime);
        m_pButtonSecrets->onUpdate(deltaTime);

        // update the glow bar
        getUIManger()->updateGlowBar(deltaTime);

        int highLight = -1;
        if (m_pButtonNewCampaign->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonNewCampaign->getPosition());
            highLight = 0;
        } else if (m_pButtonResumeCampaign->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonResumeCampaign->getPosition());
            highLight = 1;
        } else if (m_pButtonEditProfile->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonEditProfile->getPosition());
            highLight = 2;

        } else if (m_pButtonLore->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonLore->getPosition());
            highLight = 3;

        } else if (m_pButtonSecrets->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonSecrets->getPosition());
            highLight = 4;
        } else if (m_pButtonExtras->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonExtras->getPosition());
            highLight = 5;

        } else if (m_pButtonLevelSelect->highlighted()) {
            getUIManger()->setScreenHighlight(m_pButtonLevelSelect->getPosition());
            highLight = 6;
        }

        if (highLight != m_CurrHL) {
            if (highLight != -1) {
                //g_AudioMgr.Play("Cusor_Norm");
            }

            m_CurrHL = highLight;
        }
    }

    void CampaignMenuDialog::onPadSelect(Window* win)
    {
        if (state == DialogState::Active) {
        }
    }
}
