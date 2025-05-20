
#include "Player.h"
#include "../inputMgr/GamePad.h"
// #include "objects\ParticleEmiter.hpp"
// #include "objects\Render\PostEffectMgr.hpp"
// #include "objects\SpawnPoint.hpp"
#include "Event.h"
#include "sound/EventSoundEmitter.h"
// #include "..\support\templatemgr\TemplateMgr.hpp"
// #include "characters\Character.hpp"
// #include "Characters\Conversation_Packet.hpp"
// #include "GameLib\StatsMgr.hpp"
// #include "GameLib\RenderContext.hpp"
// #include "Dictionary\global_dictionary.hpp"
// #include "objects\WeaponSniper.hpp"
#include "ThirdPersonCamera.h"
//#include "objects\WeaponSMP.hpp"
#include "Corpse.h"
// #include "NetworkMgr/NetObjMgr.hpp"
// #include "NetworkMgr/Voice/VoiceMgr.hpp"
// #include "Objects\Ladders\Ladder_Field.hpp"
// #include "Objects\GrenadeProjectile.hpp"
// #include "Objects\GravChargeProjectile.hpp"
// #include "Objects\JumpingBeanProjectile.hpp"
// #include "render\LightMgr.hpp"
// #include "Objects\Door.hpp"
#include "Projector.h"
// #include "objects\WeaponMutation.hpp"
// #include "StateMgr\StateMgr.hpp"
// #include "NetworkMgr\GameMgr.hpp"
#include "HudObject.h"
#include "../characters/ActorEffects.h"
// #include "Configuration/GameConfig.hpp"
// #include "objects\turret.hpp"
// #include "objects\WeaponShotgun.hpp"
// #include "Gamelib/DebugCheats.hpp"
// #include "objects\FocusObject.hpp"
#include "../PerceptionMgr.h"
#include "LoreObject.h"
//#include "Objects\Camera.hpp"
//#include "Characters\MutantTank\Mutant_Tank.hpp"
//#include "Objects\WeaponBBG.hpp"

#include "../tweakManager/TweakMgr.h"

#include "../xfiles/xfs.h"
#include "../xfiles/x_plus.h"
#include <algorithm>

extern bool g_MirrorWeapon;

float g_CrouchUpVelocity = 80.0f;

static float MIN_TIME_BETWEEN_MUTATION_CHANGES = 0.5f;

void player::UpdateUserInput(float DeltaTime)
{
    if (!m_bActivePlayer) {
        return;
    }

    /* IJB
if ( g_StateMgr.IsPaused() )
{

    ClearStickInput();
    g_IngamePad[m_ActivePlayerPad].ClearAllLogical();
    return;
}
*/
    if (m_ViewCinematicPlaying) {
        //Play the view cinematic
        UpdateViewCinematic(DeltaTime);
    } else if (!m_Cinema.m_bCinemaOn && (!m_bDead || !m_bCanDie)) {
        if (m_ActivePlayerPad != -1) {
            //handles button press events
            OnButtonInput();
            UpdateStickInput();
        }
    } else if (m_Cinema.m_bCinemaOn) {
        ClearStickInput();
        m_fMoveValue = 0;
        m_fStrafeValue = 0;
        m_fRawControllerYaw = 0;
        m_fRawControllerPitch = 0;
        m_fPreviousYawValue = 0;
        m_fPreviousPitchValue = 0;
        g_IngamePad[m_ActivePlayerPad].ClearAllLogical();
    }
}

void player::ClearStickInput()
{
    m_fMoveValue = 0.0f;
    m_fStrafeValue = 0.0f;
    m_fRawControllerPitch = 0.0f;
    m_fRawControllerYaw = 0.0f;
    m_fPitchValue = 0.0f;
    m_fYawValue = 0.0f;
    m_fPreviousPitchValue = 0.0f;
    m_fPreviousYawValue = 0.0f;
}

//===========================================================================

struct controller_scale_tweak
{
    Vector2 Point0;
    Vector2 Point1;
    Vector2 Direction0;
    Vector2 Direction1;
};

controller_scale_tweak g_ControllerScaleTweak = {Vector2(0.0f, 0.0f),  // Point0
                                                 Vector2(1.0f, 1.0f),  // Point1
                                                 Vector2(1.0f, 0.1f),  // Direction0
                                                 Vector2(0.0f, 3.2f)}; // Direction1

void player::ScaleYawAndPitchValues(void)
{
    //
    // We need to scale the raw controller values on a spline curve
    // The following code computes the multiplication factors for the
    // raw controller values (pitch and yaw), so we can multiply and get the new values.
    // The shape of the curve is tuned with Direction0.y and Direction1.y
    // in g_ControllerScaleTweak
    //
    float Sign = (m_fRawControllerYaw < 0.0f) ? -1.0f : 1.0f;
    float s = abs(m_fRawControllerYaw);
    float s2 = s * s;
    float s3 = s * s * s;
    float h1 = (2.0f * s3) - (3 * s2) + 1;
    float h2 = (-2.0f * s3) + (3 * s2);
    float h3 = s3 - (2.0f * s2) + s;
    float h4 = s3 - s2;

    m_fYawValue = (h1 * g_ControllerScaleTweak.Point0.y) + (h2 * g_ControllerScaleTweak.Point1.y) + (h3 * g_ControllerScaleTweak.Direction0.y) + (h4 * g_ControllerScaleTweak.Direction1.y);
    m_fYawValue *= Sign;

    Sign = (m_fRawControllerPitch < 0.0f) ? -1.0f : 1.0f;
    s = abs(m_fRawControllerPitch);
    s2 = s * s;
    s3 = s * s * s;
    h1 = (2.0f * s3) - (3 * s2) + 1;
    h2 = (-2.0f * s3) + (3 * s2);
    h3 = s3 - (2.0f * s2) + s;
    h4 = s3 - s2;

    m_fPitchValue = (h1 * g_ControllerScaleTweak.Point0.y) + (h2 * g_ControllerScaleTweak.Point1.y) + (h3 * g_ControllerScaleTweak.Direction0.y) + (h4 * g_ControllerScaleTweak.Direction1.y);
    m_fPitchValue *= Sign;

    /* IJB
        player_profile& p = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));

        u32 sensitivity_H = p.GetSensitivity(SM_X_SENSITIVITY);
        u32 sensitivity_V = p.GetSensitivity(SM_Y_SENSITIVITY);

        // Sensitivity setting range
        // |___________I__________|
        // 0           50        100
        //
        // 0   = -50%
        // 50  = normal
        // 100 = +50%
        //

        float Scalar_H = (float)((sensitivity_H - 50.0f)/100.0f);
        float Scalar_V = (float)((sensitivity_V - 50.0f)/100.0f);

        m_fYawValue     = m_fYawValue + (m_fYawValue * Scalar_H);
        m_fPitchValue   = m_fPitchValue + (m_fPitchValue * Scalar_V);
*/
}

//==============================================================================

void player::UpdateStickInput(void)
{
    assert(m_ActivePlayerPad != -1);

    m_fPreviousYawValue = m_fYawValue;
    m_fPreviousPitchValue = m_fPitchValue;

    m_fRawControllerYaw = -g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::LOOK_HORIZONTAL).IsValue;
    m_fRawControllerPitch = g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::LOOK_VERTICAL).IsValue;

    /* IJB
        // invert Y
        {
            player_profile& p = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));

            // MAB: removed invert Y global var - only check profile now
            if( p.m_bInvertY )
            {
                m_fRawControllerPitch   = -m_fRawControllerPitch;
            }
        }
    */
    if (!m_bInTurret) {
        m_fMoveValue = +g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::MOVE_FOWARD_BACKWARDS).IsValue;
        m_fStrafeValue = -g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::MOVE_STRAFE).IsValue;
        /* IJB
                {
                    static float MoveValue   = 0.0f;
                    static float StrafeValue = 0.0f;

                    if( input_IsPressed( INPUT_KBD_E ) )
                        MoveValue =  1.0f;
                    else if( input_IsPressed( INPUT_KBD_D ) )
                        MoveValue = -1.0f;
                    else
                        MoveValue = 0.0f;

                    if( input_IsPressed( INPUT_KBD_F ) )
                        StrafeValue = -1.0f;
                    else if( input_IsPressed( INPUT_KBD_S ) )
                        StrafeValue =  1.0f;
                    else
                        StrafeValue = 0.0f;

                    m_fMoveValue   += MoveValue;
                    m_fStrafeValue += StrafeValue;
                }
        */
    } else {
        m_fMoveValue = 0.0f;
        m_fStrafeValue = 0.0f;
    }

    ScaleYawAndPitchValues();
}

void player::OnButtonInput(void)
{
    // don't allow player to switch weapons, zoom in, attack, etc.
    if (m_bHidePlayerArms) {
        return;
    }
    // This is to make sure you don't lean or fire on the same button press as
    // voting or respawning, respectively.
    {
        bool PrimaryDown = (bool)g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_PRIMARY).IsValue;
        // IJB PrimaryDown |= input_IsPressed( INPUT_MOUSE_BTN_L );
        if (!PrimaryDown) {
            m_bRespawnButtonPressed = false;
        }

        if (!g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_YES).IsValue &&
            !g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_NO).IsValue) {
            m_bVoteButtonPressed = false;
        }
    }

    //
    // Handle voting menu input
    //
    if (m_VoteCanCast) {
        if (!m_VoteMode) {
            // Activate vote mode / menu.
            if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_MENU_ON).WasValue) {
                m_VoteMode = true;
                // Activate the vote menu.
                //LOG_MESSAGE( "player::OnButtonInput", "Vote menu activated." );
                return;
            }
        } else {
            // Deactivate vote mode / menu.
            if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_MENU_OFF).WasValue) {
                m_VoteMode = false;
                // Deactivate the vote menu.
                //LOG_MESSAGE( "player::OnButtonInput", "Vote menu deactivated." );
                return;
            }

            // Vote YES.
            if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_YES).WasValue) {
                m_VoteMode = false;
                m_VoteCanCast = false;
                m_bVoteButtonPressed = true;
                // Deactivate the vote menu.
                VoteCast(+1);
                //LOG_MESSAGE( "player::OnButtonInput", "Vote YES." );
                return;
            }

            // Vote NO.
            if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_NO).WasValue) {
                m_VoteMode = false;
                m_VoteCanCast = false;
                m_bVoteButtonPressed = true;
                // Deactivate the vote menu.
                VoteCast(-1);
                //LOG_MESSAGE( "player::OnButtonInput", "Vote NO." );
                return;
            }

            // Vote ABSTAIN.
            if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_VOTE_ABSTAIN).WasValue) {
                m_VoteMode = false;
                m_VoteCanCast = false;
                // Deactivate the vote menu.
                VoteCast(0);
                //LOG_MESSAGE( "player::OnButtonInput", "Vote ABSTAIN." );
                return;
            }
        }
    } else {
        // So that the vote key doesn't show up if the vote expires while it's showing.
        m_VoteMode = false;
    }

    //
    // Do nothing if stunned.
    //
    if (m_NonExclusiveStateBitFlag & NE_STATE_STUNNED) {
        return;
    }

    //update base class button input
    assert(m_ActivePlayerPad != -1);

    bool bStopCrouching = false;

    /* IJB
    player_profile& p = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));
    if (p.m_bCrouchOn) {
        if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_CROUCH).WasValue) {
            // crouch is a toggle and we're crouching so turn it off
            if (IsCrouching()) {
                bStopCrouching = true;
            } else if (!m_bInTurret) {
                // move the arms a little
                m_ArmsVelocity += Vector3(0.0f, -g_CrouchUpVelocity, 0.0f);

                // Start crouching
                SetIsCrouching(true);
            }
        }
    } else */
    if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_CROUCH).IsValue) {
        // only do this if we weren't crouching previously
        if (!IsCrouching() && !m_bInTurret) {
            // move the arms a little
            m_ArmsVelocity += Vector3(0.0f, -g_CrouchUpVelocity, 0.0f);

            // Start crouching
            SetIsCrouching(true);
        }
    } else {
        // Only do this if we are already crouching
        if (IsCrouching()) {
            bStopCrouching = true;
        }
    }

    // for whatever reason, we need to quit crouching
    if (bStopCrouching) {
        // Stop crouching
        SetIsCrouching(false);

        // move the arms a little
        m_ArmsVelocity += Vector3(0.0f, g_CrouchUpVelocity, 0.0f);
    }

    // Jump button
    bool JumpPressed = (bool)g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_JUMP).WasValue;
    // IJB JumpPressed |= input_WasPressed(INPUT_KBD_SPACE);

    if (!m_bInTurret && JumpPressed && m_bCanJump) {
        Jump();

        // Stop crouching when you jump
        SetIsCrouching(false);
    }

    // Look for 'game speak' buttons.
    OnGameSpeak();

    //
    // Toggle mutation
    //
    bool Multiplayer = false;

    //
    // mreed:
    // This is a little strange, but we need more pressure to toggle mutation when we're
    // leaning. This means it's less likely to succeed with a "WasValue" since you only get
    // one shot at it. So, when leaning, we will use "IsValue" so that we have more than
    // one chance to get the pressure needed. The side-effect of this is that we are no longer
    // debounced for mutation when leaning.
    // Luckily (?), there is a mutation frequency timer that prevents rapid mutation/demutation
    // so we'll always get through the mutation cycle before we come back. Plus, this only matters
    // when intentionally trying to mutate while leaning.
    //
    // So, this is where we get the button press values
    //
    ingame_pad::logical& Logical = g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_MUTATION);
    ingame_pad::logical& MPLogical = g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_MP_MUTATE);

    const bool  IsLeaning = abs(m_SoftLeanAmount) > 0.1f;
    const float ActionMutation = IsLeaning ? Logical.IsValue : Logical.WasValue;
    const float ActionMPMutation = IsLeaning ? MPLogical.IsValue : MPLogical.WasValue;

    const bool MutationPressed = Multiplayer
                                     ? (ActionMutation > 0.0f)
                                     : (ActionMPMutation > 0.0f);

    static float MinTimeSinceUseToThrowGrenade = 1.0f;
    static float MinTimeSinceUseToMutate = 1.0f;

    if (!m_bInTurret && m_bCanToggleMutation && !IsChangingMutation() && MutationPressed && (m_MutationChangeTime > MIN_TIME_BETWEEN_MUTATION_CHANGES) && (m_UseTime > MinTimeSinceUseToMutate)) {

        //
        // If we're leaning, we need to press harder to toggle mutation
        //
        bool HaveButtonPressureToToggleMutation = true;

        float MutationValue = 1.0f;

        if (IsLeaning) {
            // IJB MutationValue = input_GetValue(INPUT_PS2_BTN_L_UP, m_ActivePlayerPad);
        }

        //ASSERT(g_MonkeyOptions.Enabled || (MutationValue > 0.0f), "Dpad Up is zero, when toggling mutation, the mapping has changed, find Mike Reed");

        static float MinValueToToggleMutationWhileLeaning = 0.3f;
        HaveButtonPressureToToggleMutation = MutationValue > MinValueToToggleMutationWhileLeaning;

        if (HaveButtonPressureToToggleMutation) {
            // OK, our input is in order, see what we need to do
            if (m_Inventory2.HasItem(INVEN_WEAPON_MUTATION) && !IsMutated()) {
                SetupMutationChange(true);
            } else if (IsMutated()) {
                SetupMutationChange(false);
            }
        }
    }

    if (!m_bInTurret && g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_CYCLE_RIGHT).WasValue) {
        OnWeaponSwitch2(CYCLE_RIGHT);
    }
    if (!m_bInTurret && g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_CYCLE_LEFT).WasValue) {
        OnWeaponSwitch2(CYCLE_LEFT);
    }

    //
    // NOTE: TWEEK THIS, WE MIGHT WANT TO MAKE THE RAMP DOWN FIRST BEFORE DOING MELEE.
    //
    const bool MeleePressed = m_bIsMutated
                                  ? g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_MUTANT_MELEE).IsValue > 0.0f
                                  : g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_MELEE_ATTACK).IsValue > 0.0f;
    if (!m_bInTurret && MeleePressed) {
        animation_state MeleeState = ANIM_STATE_MELEE;

        // make sure we are completely mutated
        if (IsMutated()) {
            switch (m_CurrentAnimState) {
                // we don't want melee kicking off when we are switching to while we are mutated.
                // This will cause the meshes to get hosed
            case ANIM_STATE_SWITCH_TO:
            case ANIM_STATE_SWITCH_FROM:
            {
            } break;

                //////////////////////////////////////////////////////////////////////////
                // put any other mutation special cases here
                //////////////////////////////////////////////////////////////////////////

            default:
            {

                // if we are mutated, do extreme melee attack.
                if (m_bMutationMeleeEnabled) {
                    // if we are already attacking, return.
                    // NOTE: we can't expect SetAnimState to check if the anims are the same because we have 5 different ones here.
                    if (!m_bMeleeLunging) {
                        MeleeState = SetupMutationMeleeWeapon();
                        SetMeleeState(MeleeState);
                    }
                }
            } break;
            }
        } else // we aren't mutated, do normal melee stuff
        {
            if (m_ComboCount >= MAX_COMBO_HITS) {
                m_ComboCount = MAX_COMBO_HITS - 1;
                assert(0);
                return;
            }

            if (m_ComboCount == 0) {
                // if you don't do this, when you demutate you will swing your wittle human arms and it looks dumb :)
                if (m_CurrentAnimState != ANIM_STATE_SWITCH_FROM && m_CurrentAnimState != ANIM_STATE_DISCARD) {
                    if (m_bCanRequestCombo && m_bLastMeleeHit) {
                        // Still stage 0 and we're requesting to start a combo
                        SetMeleeState(ANIM_STATE_COMBO_BEGIN);
                    } else {
                        // we aren't requesting a combo yet, do initial melee
                        SetMeleeState(MeleeState);
                    }
                }
            }

            // if you can request a combo, set the flag
            if (m_bCanRequestCombo) {
                m_bHitCombo = true;
            }
        }
    }

    // don't throw a grenade if we're just exiting fly mode
    if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_THROW_GRENADE).IsValue) {
        if ((m_Inventory2.GetAmount(m_CurrentGrenadeType2) > 0) && (!IsMutated()) && AllowedToFire() && !m_bInTurret && (m_UseTime > MinTimeSinceUseToThrowGrenade)) {
            // Get a reference to the state that we are considering
            int GrenadeState = (m_CurrentGrenadeType2 == INVEN_GRENADE_FRAG) ? ANIM_STATE_GRENADE : ANIM_STATE_ALT_GRENADE;

            // if we are already throwing a grenade of any type, don't any other grenade be thrown
            if (m_CurrentAnimState != ANIM_STATE_GRENADE && m_CurrentAnimState != ANIM_STATE_ALT_GRENADE) {
                state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][GrenadeState];

                // Can we fire the secondary weapon?
                if (State.nPlayerAnims > 0) {
                    new_weapon* pWeapon = GetCurrentWeaponPtr();
                    if (pWeapon) {
                        pWeapon->ClearZoom();
                    }

                    SetAnimState((animation_state)GrenadeState);
                }
            }
        }
    }

    // flashlight button
    const bool FlashlightPressed = Multiplayer
                                       ? (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_MP_FLASHLIGHT).IsValue > 0.0f)
                                       : (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_FLASHLIGHT).WasValue > 0.0f);

    if (FlashlightPressed && !IsMutated()) {
        new_weapon* pWeapon = GetCurrentWeaponPtr();

        if (pWeapon && pWeapon->HasFlashlight()) {
            SetFlashlightActive(!IsFlashlightActive());
        } else {
            // weapon is invalid?  Turn off flashlight then
            SetFlashlightActive(false);
        }
    }

    // Use on a mutagen reservoir
    if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_USE).IsValue && NearMutagenReservoir()) {
        static const float ChangeRate = 10.0f;
        AddMutagen(ChangeRate * m_DeltaTime);

        // play the sucking sound when refilling mutagen from a super-contagious dead body.
        /* IJB
        if( m_SuckingMutagenLoopID == 0 )
        {
            m_SuckingMutagenLoopID = g_AudioMgr.Play( "SCDB_Suck_Mutagen_Loop", GetPosition(), GetZone1(), true );
        }
            */
    } else {
        /*
        if( m_SuckingMutagenLoopID != 0 )
        {
            // not refilling anymore, release ID
            g_AudioMgr.Release( m_SuckingMutagenLoopID, 1.0f );
            m_SuckingMutagenLoopID = 0;
        }
            */
    }

    if (!m_bInTurret && g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::LEAN_LEFT).IsValue && !m_bVoteButtonPressed) {
        UpdateLean(1.0f);
    } else if (!m_bInTurret && g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::LEAN_RIGHT).IsValue && !m_bVoteButtonPressed) {
        UpdateLean(-1.0f);
    } else {
        UpdateLean(0.0f);
    }
}
