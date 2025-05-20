
#include "Player.h"
#include "../inputMgr/GamePad.h"
//#include "ParticleEmiter.h"
//#include "render/PostEffectMgr.h"
//#include "SpawnPoint.h"
#include "Event.h"
#include "sound/EventSoundEmitter.h"
//#include "..\support\templatemgr\TemplateMgr.hpp"
//#include "../characters/Character.h"
//#include "Characters\Conversation_Packet.hpp"
//#include "GameLib\StatsMgr.hpp"
//#include "GameLib\RenderContext.hpp"
//#include "Dictionary\global_dictionary.hpp"
//#include "WeaponSniper.h"
#include "ThirdPersonCamera.h"
//#include "WeaponSMP.h"
#include "Corpse.h"
//#include "NetworkMgr/NetObjMgr.hpp"
//#include "NetworkMgr/Voice/VoiceMgr.hpp"
//#include "Ladders/Ladder_Field.h"
//#include "GrenadeProjectile.h"
//#include "GravChargeProjectile.h"
//#include "JumpingBeanProjectile.h"
//#include "render/LightMgr.h"
//#include "Door.h"
#include "Projector.h"
//#include "WeaponMutation.h"
//#include "StateMgr\StateMgr.hpp"
//#include "NetworkMgr\GameMgr.hpp"
#include "HudObject.h"
#include "../characters/ActorEffects.h"
//#include "Configuration/GameConfig.hpp"
//#include "turret.h"
//#include "WeaponShotgun.h"
//#include "Gamelib/DebugCheats.hpp"
//#include "FocusObject.h"
#include "../PerceptionMgr.h"
#include "LoreObject.h"
//#include "Camera.h"
#include "../tweakManager/TweakMgr.h"

#include <algorithm>

static const float s_idle_timeout_min             = 5.f;
static const float s_ilde_timout_max              = 8.f;
static const float s_DebounceDuration             = 0.5f;

tweak_handle TapRefireRefreshSeconds_Tweak( "TapRefireRefreshSeconds" );

void player::BeginState()
{
    m_TimeInState = 0.0f;

    switch( m_CurrentAnimState )
    {
    case ANIM_STATE_SWITCH_TO:          BeginSwitchTo();        break;
    case ANIM_STATE_SWITCH_FROM:        BeginSwitchFrom();      break;
    case ANIM_STATE_IDLE:               BeginIdle();            break;
    case ANIM_STATE_RUN:                BeginRun();             break;
    case ANIM_STATE_PICKUP:             BeginPickup();          break;
    case ANIM_STATE_DISCARD:            BeginDiscard();         break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_FIRE:               BeginFire();            break;
    case ANIM_STATE_ALT_FIRE:           BeginAltFire();         break;
    case ANIM_STATE_GRENADE:            BeginGrenade();         break;
    case ANIM_STATE_ALT_GRENADE:        BeginAltGrenade();      break;
    
    case ANIM_STATE_MELEE:              BeginMelee();           break;

    case ANIM_STATE_MUTATION_SPEAR:
    case ANIM_STATE_MELEE_FROM_CENTER:  
    case ANIM_STATE_MELEE_FROM_UP:
    case ANIM_STATE_MELEE_FROM_DOWN:
    case ANIM_STATE_MELEE_FROM_RIGHT:
    case ANIM_STATE_MELEE_FROM_LEFT:
        BeginMelee_Special(m_CurrentAnimState);   
        break;

    case ANIM_STATE_COMBO_BEGIN:        BeginCombo();           break;
    case ANIM_STATE_COMBO_HIT:          BeginCombo_Hit();       break;
    case ANIM_STATE_COMBO_END:          BeginCombo_End();       break;

        //------------------------------------------------------------------------------
    case ANIM_STATE_RELOAD:             BeginReload();          break;
    case ANIM_STATE_RELOAD_IN:          BeginReloadIn();        break;
    case ANIM_STATE_RELOAD_OUT:         BeginReloadOut();       break;
    case ANIM_STATE_RAMP_UP:            BeginRampUp();          break;
    case ANIM_STATE_RAMP_DOWN:          BeginRampDown();        break;
    case ANIM_STATE_ALT_RAMP_UP:        BeginAltRampUp();       break;
    case ANIM_STATE_ALT_RAMP_DOWN:      BeginAltRampDown();     break;
    case ANIM_STATE_HOLD:               BeginHold();            break;
    case ANIM_STATE_ALT_HOLD:           BeginAltHold();         break;
    case ANIM_STATE_ZOOM_IN:            BeginZoomIn();          break;
    case ANIM_STATE_ZOOM_OUT:           BeginZoomOut();         break;
    case ANIM_STATE_ZOOM_IDLE:          BeginZoomIdle();        break;
    case ANIM_STATE_ZOOM_RUN:           BeginZoomRun();         break;
    case ANIM_STATE_ZOOM_FIRE:          BeginZoomFire();        break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_DEATH:              BeginDeath();           break;
    case ANIM_STATE_MISSION_FAILED:     BeginMissionFailed();   break;
    case ANIM_STATE_FALLING_TO_DEATH:   BeginFallingToDeath();  break;
    case ANIM_STATE_CINEMA:             BeginCinema();          break;
    case ANIM_STATE_CHANGE_MUTATION:    BeginMutationChange();  break;
    case ANIM_STATE_UNDEFINED:                                  break;
    default:
        //x_throw( "Don't know how to begin this player state." );
        break;
    }

}

//------------------------------------------------------------------------------

void player::EndState()
{
    switch( m_CurrentAnimState )
    {
    case ANIM_STATE_SWITCH_TO:      EndSwitchTo();          break;
    case ANIM_STATE_SWITCH_FROM:    EndSwitchFrom();        break;
    case ANIM_STATE_IDLE:           EndIdle();              break;
    case ANIM_STATE_RUN:            EndRun();               break;
    case ANIM_STATE_PICKUP:         EndPickup();            break;
    case ANIM_STATE_DISCARD:        EndDiscard();           break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_FIRE:           EndFire();              break;
    case ANIM_STATE_ALT_FIRE:       EndAltFire();           break;
    case ANIM_STATE_GRENADE:        EndGrenade();           break;
    case ANIM_STATE_ALT_GRENADE:    EndAltGrenade();        break;
    case ANIM_STATE_MELEE:          EndMelee();             break;

    case ANIM_STATE_MUTATION_SPEAR:
    case ANIM_STATE_MELEE_FROM_CENTER:  
    case ANIM_STATE_MELEE_FROM_UP:
    case ANIM_STATE_MELEE_FROM_DOWN:
    case ANIM_STATE_MELEE_FROM_RIGHT:
    case ANIM_STATE_MELEE_FROM_LEFT:
        EndMelee_Special(m_CurrentAnimState);   
        break;

    case ANIM_STATE_COMBO_BEGIN:        EndCombo();         break;
    case ANIM_STATE_COMBO_HIT:          EndCombo_Hit();     break;
    case ANIM_STATE_COMBO_END:          EndCombo_End();     break;

        //------------------------------------------------------------------------------
    case ANIM_STATE_RELOAD:         EndReload();            break;
    case ANIM_STATE_RELOAD_IN:      EndReloadIn();          break;
    case ANIM_STATE_RELOAD_OUT:     EndReloadOut();         break;
    case ANIM_STATE_RAMP_UP:        EndRampUp();            break;
    case ANIM_STATE_RAMP_DOWN:      EndRampDown();          break;
    case ANIM_STATE_ALT_RAMP_UP:    EndAltRampUp();         break;
    case ANIM_STATE_ALT_RAMP_DOWN:  EndAltRampDown();       break;
    case ANIM_STATE_HOLD:           EndHold();              break;
    case ANIM_STATE_ALT_HOLD:       EndAltHold();           break;
    case ANIM_STATE_ZOOM_IN:        EndZoomIn();            break;
    case ANIM_STATE_ZOOM_OUT:       EndZoomOut();           break;
    case ANIM_STATE_ZOOM_IDLE:      EndZoomIdle();          break;
    case ANIM_STATE_ZOOM_RUN:       EndZoomRun();           break;
    case ANIM_STATE_ZOOM_FIRE:      EndZoomFire();          break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_DEATH:          EndDeath();             break;
    case ANIM_STATE_MISSION_FAILED: EndMissionFailed();     break;
    case ANIM_STATE_CHANGE_MUTATION:EndMutationChange();    break;
    case ANIM_STATE_FALLING_TO_DEATH:EndFallingToDeath();   break;
    case ANIM_STATE_CINEMA:         EndCinema();            break;
    case ANIM_STATE_UNDEFINED:                              break;
    default:
        //x_throw( "Don't know how to End this player state." );
        break;
    } 
}

//------------------------------------------------------------------------------

void player::UpdateState( const float& rDeltaTime )
{
#ifdef ksaffel
    //x_printfxy(2,0,"AnimState: %i",m_CurrentAnimState);
#endif
    m_TimeInState += rDeltaTime;

    switch( m_CurrentAnimState )
    {
    case ANIM_STATE_SWITCH_TO:      UpdateSwitchTo      (rDeltaTime);   break;
    case ANIM_STATE_SWITCH_FROM:    UpdateSwitchFrom    (rDeltaTime);   break;
    case ANIM_STATE_IDLE:           UpdateIdle          (rDeltaTime);   break;
    case ANIM_STATE_RUN:            UpdateRun           (rDeltaTime);   break;
    case ANIM_STATE_PICKUP:         UpdatePickup        (rDeltaTime);   break;
    case ANIM_STATE_DISCARD:        UpdateDiscard       (rDeltaTime);   break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_FIRE:           UpdateFire          (rDeltaTime);   break;
    case ANIM_STATE_ALT_FIRE:       UpdateAltFire       (rDeltaTime);   break;
    case ANIM_STATE_GRENADE:        UpdateGrenade       (rDeltaTime);   break;
    case ANIM_STATE_ALT_GRENADE:    UpdateAltGrenade    (rDeltaTime);   break;
    case ANIM_STATE_MELEE:          UpdateMelee         (rDeltaTime);   break;

    case ANIM_STATE_MUTATION_SPEAR:
    case ANIM_STATE_MELEE_FROM_CENTER:  
    case ANIM_STATE_MELEE_FROM_UP:
    case ANIM_STATE_MELEE_FROM_DOWN:
    case ANIM_STATE_MELEE_FROM_RIGHT:
    case ANIM_STATE_MELEE_FROM_LEFT:
        UpdateMelee_Special(rDeltaTime, m_CurrentAnimState);   
        break;

    case ANIM_STATE_COMBO_BEGIN:        UpdateCombo(rDeltaTime);        break;
    case ANIM_STATE_COMBO_HIT:          UpdateCombo_Hit(rDeltaTime);    break;
    case ANIM_STATE_COMBO_END:          UpdateCombo_End(rDeltaTime);    break;

        //------------------------------------------------------------------------------
    case ANIM_STATE_RELOAD:         UpdateReload        (rDeltaTime);   break;
    case ANIM_STATE_RELOAD_IN:      UpdateReloadIn      (rDeltaTime);   break;
    case ANIM_STATE_RELOAD_OUT:     UpdateReloadOut     (rDeltaTime);   break;
    case ANIM_STATE_RAMP_UP:        UpdateRampUp        (rDeltaTime);   break;
    case ANIM_STATE_RAMP_DOWN:      UpdateRampDown      (rDeltaTime);   break;
    case ANIM_STATE_ALT_RAMP_UP:    UpdateAltRampUp     (rDeltaTime);   break;
    case ANIM_STATE_ALT_RAMP_DOWN:  UpdateAltRampDown   (rDeltaTime);   break;
    case ANIM_STATE_HOLD:           UpdateHold          (rDeltaTime);   break;
    case ANIM_STATE_ALT_HOLD:       UpdateAltHold       (rDeltaTime);   break;
    case ANIM_STATE_ZOOM_IN:        UpdateZoomIn        (rDeltaTime);   break;
    case ANIM_STATE_ZOOM_OUT:       UpdateZoomOut       (rDeltaTime);   break;
    case ANIM_STATE_ZOOM_IDLE:      UpdateZoomIdle      (rDeltaTime);   break;
    case ANIM_STATE_ZOOM_RUN:       UpdateZoomRun       (rDeltaTime);   break;
    case ANIM_STATE_ZOOM_FIRE:      UpdateZoomFire      (rDeltaTime);   break;
        //------------------------------------------------------------------------------
    case ANIM_STATE_DEATH:          UpdateDeath         (rDeltaTime);   break;
    case ANIM_STATE_MISSION_FAILED: UpdateMissionFailed (rDeltaTime);   break;
    case ANIM_STATE_CHANGE_MUTATION:UpdateMutationChange(rDeltaTime);   break;
    case ANIM_STATE_FALLING_TO_DEATH:UpdateFallingToDeath(rDeltaTime);  break;
    case ANIM_STATE_CINEMA:         UpdateCinema(rDeltaTime);           break;
    case ANIM_STATE_UNDEFINED:                                          break;
    default:
        //x_throw( "Don't know how to Update this player state." );
        break;
    }
}

void player::BeginDeath()
{
    m_DeathTime = 0.0f;

    if ( !GetThirdPersonCamera() )
    {
        m_AnimStage = 1;
        m_PosOverrideCamera = m_AnimPlayer.GetBonePosition( m_iCameraBone );


        //set the animation for the weapon
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        bool bMutationWeapon = false;
        if ( pWeapon )
        {
            if ( pWeapon->GetType() == Object::TYPE_WEAPON_MUTATION )
            {
                bMutationWeapon = true;
            }

                if ( bMutationWeapon )
                {
                    const AnimGroup& CurAnimGroup( pWeapon->GetCurrentAnimGroup() );
                    pWeapon->SetAnimation( CurAnimGroup.GetAnimIndex( "MUT_WPN_Death01" ), 0.0f , false );
                }
                else
                {
                    state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_SWITCH_FROM];
                    pWeapon->SetAnimation( State.WeaponAnim[ANIM_PRIORITY_DEFAULT] , 0.0f , false );
                }
                pWeapon->ResetWeapon();
            
        }    
        else
        {
            // LOG_ERROR( "player::BeginDeath", "Unable to resolve current weapon." );
        }

        if ( !bMutationWeapon )
        {
            //get gun off screen
            SetAnimation( ANIM_STATE_SWITCH_FROM , ANIM_PRIORITY_DEFAULT , 0.f );
        }
    }
}

//------------------------------------------------------------------------------

void player::UpdateDeath( const float& DeltaTime )
{
    m_DeathTime += DeltaTime; 

    bool PrimaryPressed = (bool)g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_PRIMARY ).WasValue;

    if ( GetThirdPersonCamera() )
    {
        UpdateThirdPersonCamera();
        if( PrimaryPressed )
        {
            m_bWantToSpawn = true;

            m_bRespawnButtonPressed = true;
        }
    }
    else
    {

        //make sure override position need leaves player bounding box
        //used for falling so death cam keeps falling...
        m_PosOverrideCamera.y = std::min( GetBBox().max.y, m_PosOverrideCamera.y );
        m_PosOverrideCamera.y = std::max( GetBBox().min.y, m_PosOverrideCamera.y );

        //make sure camera is on ground
        m_AnimPlayer.SetPosition( m_PosOverrideCamera );

        //make sure anim pitch adjusts to correct position
        Radian rPitch = m_AnimPlayer.GetPitch();
        if ( rPitch > 0 )
        {
            rPitch = std::max(0.0f, (rPitch - (R_45*DeltaTime)));
        }
        else if ( rPitch < 0 )
        {
            rPitch = std::min(0.0f, (rPitch + (R_45*DeltaTime)));
        }
        m_AnimPlayer.SetPitch(rPitch);

        if ((m_AnimStage == 1) || m_bIsMutated)
        {
            //rotate the weapon
            new_weapon* pWeapon = GetCurrentWeaponPtr();
            if (pWeapon)
            {
                pWeapon->SetRotation( m_AnimPlayer.GetPitch() , m_AnimPlayer.GetYaw() );
                OnMoveWeapon();
            }
        }

        if (m_AnimStage == 3)
        {
            // IJB - how do we do this?
            //PrimaryPressed |= input_WasPressed( INPUT_MOUSE_BTN_L );

            if( PrimaryPressed )
            {
                m_bWantToSpawn = true;
                m_bRespawnButtonPressed = true;
            }
            return;
        }

        //advance death stage
        if ( m_AnimPlayer.IsAtEnd() )
        {
            m_AnimStage++;
            if (m_AnimStage == 2)
            {
                //start death anim
                SetAnimation( ANIM_STATE_DEATH , ANIM_PRIORITY_DEFAULT, 0.0f );
            }
        }
    }
}

void player::EndDeath()
{
    corpse* pCorpse = (corpse*)objectManager->GetObjectByGuid( m_CorpseGuid );
    if( pCorpse )
        pCorpse->SetPermanent( false );

    m_CorpseGuid  = 0;
    m_AnimStage     = 0;
    m_DeathType     = DEATH_BY_ANIM;

    if( GetThirdPersonCamera() )
    {
        objectManager->DestroyObjectEx( GetThirdPersonCameraGuid(), true );
        m_ThirdPersonCameraGuid = 0;
    }
}

void player::BeginMutationChange()
{
    m_AnimStage = 1;

    //get gun off screen
    SetAnimation( ANIM_STATE_SWITCH_FROM , ANIM_PRIORITY_DEFAULT , 0.f );

    //set the animation for the weapon
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_SWITCH_FROM];
        pWeapon->SetAnimation( State.WeaponAnim[ANIM_PRIORITY_DEFAULT] , 0.f , false );
        pWeapon->ResetWeapon();
    }    
    else
    {
        // LOG_ERROR( "player::BeginMutationChange", "Unable to resolve current weapon." );
    }
}

//------------------------------------------------------------------------------

void player::UpdateMutationChange( float DeltaTime )
{
    //update stun effect

    // Let's handle the weapon switch from stuff.
    if (m_AnimStage == 1 || m_AnimStage == 3 )
    {
        //rotate the weapon
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if( pWeapon )
        {
            pWeapon->SetRotation( m_AnimPlayer.GetPitch() , m_AnimPlayer.GetYaw() );
            OnMoveWeapon();
        }
    }

    //Now we have to upate the AnimStage.  Once reload ends, 
    if ( m_AnimPlayer.IsAtEnd() )
    {
        m_AnimStage++;
        if (m_AnimStage == 2)
        {
            //start now we want the change strain animation.
            SetAnimation( ANIM_STATE_CHANGE_MUTATION , ANIM_PRIORITY_DEFAULT, 0.0f );
        }

        if ( m_AnimStage == 3 )
        {
            SetCurrentStrain( );

            // Force a weapon switch back to the same spot.
            m_NextWeaponItem = GetNextAvailableWeapon2( CYCLE_LEFT );
            m_NextWeaponItem = GetNextAvailableWeapon2( CYCLE_RIGHT );

            m_PrevWeaponItem     = m_CurrentWeaponItem;
            m_CurrentWeaponItem  = m_NextWeaponItem;
            m_NextWeaponItem     = INVEN_NULL;

            new_weapon* pWeapon = GetCurrentWeaponPtr();
            assert( pWeapon );
            pWeapon->SetupRenderInformation( );
            pWeapon->SetRotation( m_AnimPlayer.GetPitch() , m_AnimPlayer.GetYaw() );
            OnMoveWeapon();

            //start now we want the change strain animation.
            SetAnimation( ANIM_STATE_SWITCH_TO , ANIM_PRIORITY_DEFAULT, 0.0f );

            //set the animation for the weapon
            state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_SWITCH_TO];
            pWeapon->SetAnimation( State.WeaponAnim[ANIM_PRIORITY_DEFAULT] , 0.f , false );
        }

        if ( m_AnimStage == 4 )
        {
            SetAnimState( ANIM_STATE_IDLE );
        }
    }
}

//------------------------------------------------------------------------------

void player::EndMutationChange()
{
    m_AnimStage = 0;
}

void player::BeginSwitchTo()
{
    // Make sure the weapon we are switching to is visible
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->SetVisible(true);

        // set up the proper render state
        pWeapon->SetRenderState(new_weapon::RENDER_STATE_PLAYER);        

        // update for things like the meson cannon that need to restart particles and such
        pWeapon->BeginSwitchTo();
    }

    // make sure if we've switched weapons manually or by picking one up that we clear our combo.
    ClearCombo();

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // Set appropriate 'switch to' animation
    SetAnimation( ANIM_STATE_SWITCH_TO , ANIM_PRIORITY_DEFAULT , 0.0f );
}

void player::UpdateSwitchTo( const float& DeltaTime )
{
    (void)DeltaTime;

    if ( m_AnimPlayer.IsAtEnd() )
    {
        // we don't reload, go ahead and play transition animation
        if( !ReloadWeapon( new_weapon::AMMO_PRIMARY ) )
        {   
            // Play the idle or run.
            SetAnimState( GetMotionTransitionAnimState() );
        }
    }
}

void player::EndSwitchTo()
{
    LoadAimAssistTweakHandles();

    if ( m_bIsMutated )
    {
        // make sure we have lost our gloves
        const float Gloves = m_Inventory2.GetAmount( INVEN_GLOVES );
        if ( Gloves > 0.0f )
        {
            m_Inventory2.RemoveAmount( INVEN_GLOVES, Gloves );
        }
    }

    // Make sure the weapon knows we are done playing the switch anim
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->EndSwitchTo();
    }
}

void player::BeginSwitchFrom()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // Set appropriate 'switch from' animation
    if( !m_bDead )
    {
        SetAnimation( ANIM_STATE_SWITCH_FROM , ANIM_PRIORITY_DEFAULT , 0.f );
    }

    // see if we need to throw this weapon away (if it's dual)
    new_weapon *pNextWeapon = GetWeaponPtr(m_NextWeaponItem);

    // if this is a dual, we're out of ammo and we're switching weapons, get rid of it.
    // this means we probably switched weapons before the discard could play
    if( pNextWeapon && (pNextWeapon->GetAmmoCount( pNextWeapon->GetPrimaryAmmoPriority() ) <= 0) )
    {
        SetupDualWeaponDiscard(m_NextWeaponItem);
    }

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->ClearZoom();

        // make sure we set weapon to idle.  For weapons that recharge even while not active weapon.
        pWeapon->BeginIdle();

        // notify the weapon of a switch from.  For anything that needs to kill effects outright or whatever.
        pWeapon->BeginSwitchFrom();
    }

    ResetStickSensitivity();
}

void player::UpdateSwitchFrom( const float& DeltaTime )
{
    (void)DeltaTime;

    if ( m_AnimPlayer.IsAtEnd() )
    {
        ForceNextWeapon();
    }
}

void player::EndSwitchFrom()
{
}

void player::BeginIdle()
{
    //Set the animation timers and 
    m_fAnimationTime = 0.f;
    m_fMaxAnimTime = x_frand( s_idle_timeout_min , s_ilde_timout_max );

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && !pWeapon->GetIdleMode() )
    {
        pWeapon->BeginIdle();

        // set up the proper render state
        pWeapon->SetRenderState( new_weapon::RENDER_STATE_PLAYER );
    }

    // Play the default idle animation for this state
    SetAnimation( ANIM_STATE_IDLE , ANIM_PRIORITY_DEFAULT );
}

void player::HandleFireInput( bool IsAlternateFire )
{   
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // if your arms are hidden, don't fire.
    if( !AllowedToFire() )
    {
        return;
    }

    if ( pWeapon )
    {
        if ( pWeapon->GetType() == TYPE_WEAPON_MUTATION )
        {
            if ( (IsAlternateFire && !m_bSecondaryMutationFireEnabled) ||
                (!IsAlternateFire && !m_bPrimaryMutationFireEnabled) )
            {
                // we can't use this shot now -- not allowed
                return;
            }

            /* IJB
            // do we have enough juice to fire?
            if( !GetMutationMeleeWeapon()->CanFire(IsAlternateFire) )
            {
                return;
            }
                */
        }
    }
    else
    {
        // IJB g_AudioMgr.Play( "NoAmmo" );
        return;
    }


    // KSS -- FIXME -- Disable secondary fire for Gamer's Day as per Daryl.
    if( pWeapon->GetType() == Object::TYPE_WEAPON_MSN && IsAlternateFire )
    {
        return;
    }

    if( pWeapon->CanFire(IsAlternateFire) )
    {
        // if it's the gauss, ramp up
        animation_state RampUpState = IsAlternateFire ? ANIM_STATE_ALT_RAMP_UP : ANIM_STATE_RAMP_UP;

        if( IsAnimStateAvailable2( m_CurrentWeaponItem, RampUpState ) )
        {
            SetAnimState( RampUpState );
        }
        else
        {
            if ( m_CurrentWeaponItem == INVEN_WEAPON_TRA )
            {
                // we're ignoring input for turret alt fire (machine gun), because we want
                // it disabled
                if ( !IsAlternateFire )
                {
                    SetAnimState( ANIM_STATE_FIRE );
                }
            }
            else
            {
                SetAnimState( IsAlternateFire ? ANIM_STATE_ALT_FIRE : ANIM_STATE_FIRE );
            }
        }
    }
    else
    {
        if( pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority() ) )
        {
            SetAnimState( ANIM_STATE_RELOAD );
        }
        else
        {
            // IJB g_AudioMgr.Play( "NoAmmo" );
        }
    }
}

//------------------------------------------------------------------------------

void player::UpdateIdle( const float& DeltaTime )
{
    // don't allow player to switch weapons, zoom in, attack, etc.
    if( !m_bHidePlayerArms )
    {
        // Check if we need to go back to a firing state
        if( IsFiring() )
        {
            HandleFireInput( false );
        }    
        else if( IsAltFiring() )
        {
            // Get a reference to the state that we are considering
            state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_ALT_FIRE];

            // Can we fire the secondary weapon?
            if( State.nWeaponAnims > 0 )
            {
                HandleFireInput( true );
            }
            else
            {
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ZOOM_IN ) )            
                {
                    SetAnimState( ANIM_STATE_ZOOM_IN );
                }
            }
        }

        bool bUsedFocusObject = UseFocusObject();
        bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

        // Reloading?
        if( bPressedReload && !NearMutagenReservoir() )
        {
            ReloadWeapon(new_weapon::AMMO_SECONDARY, false);
        }
    }

    // Check if we need to change to another motion animation.
    if ( m_Physics.GetVelocity().LengthSquared() > m_fMinRunSpeed )
    {
        SetAnimState( GetMotionTransitionAnimState() );
        return;
    }

    // Watch the animation timers and player's motion.  Switch the animation if necessary.
    m_fAnimationTime += DeltaTime;

    // Wait until the current animtion ends, then switch the animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_IDLE];

        // Are we allowed to switch to a different Idle animation?
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if( pWeapon && pWeapon->CanSwitchIdleAnim() &&
            ( m_fAnimationTime > m_fMaxAnimTime ) )
        {
            // If we're currently playing the default animation, check if different idle animations
            // exist if so then switch to one of them, otherwise restart the idle state.
            if( (m_AnimPlayer.GetAnimIndex() == State.PlayerAnim[ANIM_PRIORITY_DEFAULT]) && 
                (State.nPlayerAnims > 1) )
            {
                int nRandAnimIndex = x_irand( 1 , State.nPlayerAnims - 1 );

                if( m_CurrentAnimStateIndex == nRandAnimIndex )
                    nRandAnimIndex = 0;

                SetAnimation( ANIM_STATE_IDLE , nRandAnimIndex );

                //Set the animation timers and 
                m_fAnimationTime = 0.f;
                m_fMaxAnimTime = x_frand( s_idle_timeout_min , s_ilde_timout_max );
            }
            else
            {
                SetAnimation( ANIM_STATE_IDLE , ANIM_PRIORITY_DEFAULT );
            }
        }
        else
        {
            SetAnimation( ANIM_STATE_IDLE , ANIM_PRIORITY_DEFAULT );
        }
    }
}

void player::EndIdle()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( (m_NextAnimState != ANIM_STATE_RUN) && pWeapon )
        pWeapon->EndIdle();
}

void player::BeginRun()
{
    // If the previous state is walking, we need to blend into the run animation
    float fBlendTime = 0.0f;
    if( m_PreviousAnimState == ANIM_STATE_IDLE )
    {
        fBlendTime = DEFAULT_BLEND_TIME;
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && !pWeapon->GetIdleMode() )
        pWeapon->BeginIdle();

    // Player is running, set the animation accordingly
    SetAnimation( ANIM_STATE_RUN , 0 , fBlendTime );    
}

//------------------------------------------------------------------------------

void player::UpdateRun( const float& DeltaTime )
{
    // don't allow player to switch weapons, zoom in, attack, etc.
    if( !m_bHidePlayerArms )
    {
        // Check if we need to go back to a firing state
        if( IsFiring() )
        {
            HandleFireInput( false );
        }
        else if( IsAltFiring() )
        {
            // Get a reference to the state that we are considering
            state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_ALT_FIRE];

            // Can we fire the secondary weapon?
            if( State.nWeaponAnims > 0 )
            {
                HandleFireInput( true );
            }
            else
            {
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ZOOM_IN ) )
                {
                    SetAnimState( ANIM_STATE_ZOOM_IN );
                }
            }
        }

        bool bUsedFocusObject = UseFocusObject();
        bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

        // Reloading?
        if( bPressedReload && !NearMutagenReservoir() )
        {
            ReloadWeapon(new_weapon::AMMO_PRIMARY, false);
        }
    }

    // Check if we need to change to another motion animation.
    if ( m_Physics.GetVelocity().LengthSquared() < m_fMinRunSpeed )
    {
        SetAnimState( GetMotionTransitionAnimState() );
    }
}

//------------------------------------------------------------------------------

void player::EndRun()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( (m_NextAnimState != ANIM_STATE_IDLE) && pWeapon )
        pWeapon->EndIdle();
}

void player::BeginPickup()
{
    // Set previous weapon and current weapon, clear next weapon
    m_PrevWeaponItem     = m_CurrentWeaponItem;
    m_CurrentWeaponItem  = m_NextWeaponItem;
    m_NextWeaponItem     = INVEN_NULL;

    // zero out the reticle radius
    m_ReticleRadius             = 0.0f;
    m_ReticleGrowSpeed          = 0.0f;
    m_AimAssistData.bReticleOn  = false;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->SetupRenderInformation( );
        pWeapon->SetRotation( m_AnimPlayer.GetPitch() , m_AnimPlayer.GetYaw() );
        OnMoveWeapon();
    }
    else
    {
        SetAnimState( ANIM_STATE_UNDEFINED );
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // Set appropriate 'switch from' animation
    SetAnimation( ANIM_STATE_PICKUP , ANIM_PRIORITY_DEFAULT , 0.f );

    if ( pWeapon )
    {
        pWeapon->SetVisible( true );
        pWeapon->ClearZoom();
    }
}

//------------------------------------------------------------------------------

void player::UpdatePickup( const float& DeltaTime )
{
    if ( m_AnimPlayer.IsAtEnd() )
    {
        // Bring the new weapon up.
        SetAnimState( GetMotionTransitionAnimState() );

    }
}

//------------------------------------------------------------------------------

void player::EndPickup()
{
}

void player::BeginDiscard()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_DISCARD , ANIM_PRIORITY_DEFAULT );
}

//------------------------------------------------------------------------------

void player::UpdateDiscard( const float& DeltaTime )
{
    if ( m_AnimPlayer.IsAtEnd() )
    {
        // not a dual weapon, discard normal weapon
        if( !SetupDualWeaponDiscard(m_CurrentWeaponItem) )
        {
            if( m_Inventory2.HasItem( m_CurrentWeaponItem ) )
            {
                m_Inventory2.RemoveAmount( m_CurrentWeaponItem, 1.0f );
            }

            // Set previous weapon and current weapon, clear next weapon
            m_PrevWeaponItem     = m_CurrentWeaponItem;
            m_CurrentWeaponItem  = m_NextWeaponItem;
            m_NextWeaponItem     = INVEN_NULL;
        }

        // zero out the reticle radius
        m_ReticleRadius             = 0.0f;
        m_ReticleGrowSpeed          = 0.0f;
        m_AimAssistData.bReticleOn  = false;

        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if( pWeapon )
        {
            pWeapon->SetupRenderInformation( );
            pWeapon->SetRotation( m_AnimPlayer.GetPitch() , m_AnimPlayer.GetYaw() );
            OnMoveWeapon();

            // see if we need to reload after discarding other weapon
            if( !ReloadWeapon(new_weapon::AMMO_PRIMARY) )
            {
                // Bring the new weapon up.
                SetAnimState( ANIM_STATE_IDLE );
            }
        }
        else
        {
            SetAnimState( ANIM_STATE_UNDEFINED );
        }
    }
}

//------------------------------------------------------------------------------
void player::EndDiscard()
{
    LoadAimAssistTweakHandles();
}

void player::BeginFire()
{
    // Generate the percentages used to determine what firing animations are being played.
    GenerateFiringAnimPercentages();

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon  )
    {
        pWeapon->BeginPrimaryFire();
    }

    // we can't tap fire again until release
    m_bCanTapFire = false;

    // reset timer
    m_TapRefireTime = 0.0f;

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // If we have more than one fire anim then play a different one each time we fire.    
    state_anims& WeaponState    = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_FIRE];

    SetAnimation( ANIM_STATE_FIRE, ANIM_PRIORITY_DEFAULT, 0.0f );
}

//------------------------------------------------------------------------------
void player::UpdateFire( const float& DeltaTime )
{
    m_TapRefireTime += DeltaTime;

    m_LastTimeWeaponFired = (float)x_GetTimeSec();   
    
    // If the current weapon needs to be reloaded, set the next state to reload
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
        {
            // Discard
            SetAnimState( ANIM_STATE_DISCARD );
            m_NextAnimState = ANIM_STATE_DISCARD;
        }
        else
        {
            m_NextAnimState =  GetMotionTransitionAnimState();
        }
    }

    // Is the player still holding the fire button?
    bool   bFiring = IsFiring();

    // Do we need to reload?  If the weapon has a ramp down play that first.
    // If we run out of ammo and can't reload go to the Idle state.
    if( m_NextAnimState == ANIM_STATE_RELOAD )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }
        return;
    }
    else if( (m_NextAnimState == ANIM_STATE_IDLE) || (m_NextAnimState == ANIM_STATE_RUN) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }

        // see if we need to reload
        ReloadWeapon(new_weapon::AMMO_PRIMARY);

        return;
    }

    if ( m_NextAnimState != ANIM_STATE_DISCARD )
    {
        // When the currently playing animation ends, need to set another animation
        if( bFiring )
        {
            // Still firing away.
            if( m_AnimPlayer.IsAtEnd() || 
                (pWeapon->CanFastTapFire() && m_bCanTapFire && (m_TapRefireTime >= TapRefireRefreshSeconds_Tweak.GetF32())) )
            {
                // Start fire animation
                state_anims& WeaponState = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_FIRE];
                if( WeaponState.nWeaponAnims > 1 )
                {
                    if( m_LastFireAnimStateIndex >= WeaponState.nWeaponAnims )
                        m_LastFireAnimStateIndex = 0;

                    SetAnimation( ANIM_STATE_FIRE, m_LastFireAnimStateIndex, 0.0f );
                    m_LastFireAnimStateIndex++;
                }
                else
                {
                    SetAnimation( ANIM_STATE_FIRE, ANIM_PRIORITY_DEFAULT, 0.0f );
                }
                
                // we can't tap fire again until release
                m_bCanTapFire = false;

                // reset timer
                m_TapRefireTime = 0.0f;
            }
        }
        else
        {
            // we've relased the button, signal a tap fire.
            m_bCanTapFire = true;
        
            // can we interrupt primary fire?
            if( pWeapon && pWeapon->CanIntereptPrimaryFire( m_CurrentAnimIndex ) )
            {
                // We stopped firing, set appropriate animation state
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RAMP_DOWN ) ) 
                {
                    SetAnimState( ANIM_STATE_RAMP_DOWN );
                }
                else
                {
                    SetAnimState( GetMotionTransitionAnimState() );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------

void player::EndFire()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->EndPrimaryFire();
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;
}

void player::BeginAltFire()
{
    // Generate the percentages used to determine what firing animations are being played.
    GenerateFiringAnimPercentages();

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // tell the weapon we are starting to fire (currently only for meson cannon).
    if( pWeapon )
    {
        pWeapon->BeginAltFire();
    }

    // Set appropriate idle firing animation
    SetAnimation( ANIM_STATE_ALT_FIRE , ANIM_PRIORITY_DEFAULT, 0.0f );
}

//------------------------------------------------------------------------------

void player::UpdateAltFire( const float& DeltaTime )
{
    // Time to reload or go to fire empty.
    // If the current weapon needs to be reloaded, set the next state to reload
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon 
        && (   pWeapon->GetAmmoCount( pWeapon->GetSecondaryAmmoPriority() ) <= 0) 
        && (m_AnimPlayer.IsAtEnd()) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
        {
            // Discard
            SetAnimState( ANIM_STATE_DISCARD );
            m_NextAnimState = ANIM_STATE_DISCARD;
        }
        else
        {
            m_NextAnimState =  GetMotionTransitionAnimState();
        }
    }
    else if ( pWeapon 
        && ((pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0) 
        && (m_AnimPlayer.IsAtEnd())) )
    {
        if ( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
        {
            // Discard
            SetAnimState( ANIM_STATE_DISCARD );
            m_NextAnimState = ANIM_STATE_DISCARD;
        }
        else
        {
            // If we can reload, do so.
            // Otherwise, go immediately to alt fire empty
            if ( pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority() ) )
                m_NextAnimState = ANIM_STATE_RELOAD;
            else
                m_NextAnimState =  GetMotionTransitionAnimState();
        }
    }

    // When the currently playing animation ends, need to set another animation
    bool bFiring = false;
    if( IsAltFiring() )
        bFiring = true;

    // Do we need to reload?  If the weapon has a ramp down play that first.
    // If we run out of ammo and can't reload go to the Idle state.
    if( m_NextAnimState == ANIM_STATE_RELOAD )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_ALT_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }
        return;
    }
    else if( (m_NextAnimState == ANIM_STATE_IDLE) || (m_NextAnimState == ANIM_STATE_RUN) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_ALT_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }

        // see if we need to reload
        ReloadWeapon(new_weapon::AMMO_SECONDARY);

        return;
    }

    if ( m_NextAnimState != ANIM_STATE_DISCARD )
    {
        bool bCheckRampDown = false;
        // When the currently playing animation ends, need to set another animation
        if( bFiring )
        {
            // Still firing away.
            if( m_AnimPlayer.IsAtEnd() )
            {
                if( pWeapon->CanAltChainFire() )
                {
                    //s Sitch to running / firing state?
                    int nAnimIndex = GetNextFiringAnimIndex();
                    SetAnimation( ANIM_STATE_ALT_FIRE, nAnimIndex , 0.f );
                }
                else
                {
                    bCheckRampDown = true;
                }
            }
        }
        else
        {
            bCheckRampDown = true;
        }

        // check if we need to ramp down or transition
        if( bCheckRampDown )
        {
            if( pWeapon && pWeapon->CanIntereptSecondaryFire( m_CurrentAnimIndex ) )
            {
                // We stopped firing, set appropriate animation state
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_DOWN ) ) 
                {
                    SetAnimState( ANIM_STATE_ALT_RAMP_DOWN );
                }
                else// if( m_AnimPlayer.IsAtEnd() )
                {
                    SetAnimState( GetMotionTransitionAnimState() );
                }
            }
        }
    }
}

//------------------------------------------------------------------------------

void player::EndAltFire()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->EndAltFire();
    }        

    m_NextAnimState = ANIM_STATE_UNDEFINED;
}

void player::BeginGrenade()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // Set appropriate grenade throwing animation.
    SetAnimation( ANIM_STATE_GRENADE , ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }
}

//------------------------------------------------------------------------------

void player::UpdateGrenade( const float& DeltaTime )
{
    // When the currently playing animation ends, need to set another animation
    if ( m_AnimPlayer.IsAtEnd() )
    {
        // see if we need to throw this weapon away (if it's dual)
        new_weapon *pWeapon = GetWeaponPtr(m_CurrentWeaponItem);

        // if this is a dual, we're out of ammo and we've thrown a grenade, get rid of it.        
        if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0) )
        {
            if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
            {
                // Discard
                SetAnimState( ANIM_STATE_DISCARD );
                m_NextAnimState = ANIM_STATE_DISCARD;
                return;
            }
        }
        
        // see if we interrupted a reload
        if( !ReloadWeapon(new_weapon::AMMO_PRIMARY) )
        {
            SetAnimState( GetMotionTransitionAnimState() );
        }
    }
}

void player::EndGrenade()
{
}

void player::BeginAltGrenade()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    // Set appropriate grenade throwing animation.
    SetAnimation( ANIM_STATE_ALT_GRENADE , ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }
}

//------------------------------------------------------------------------------

void player::UpdateAltGrenade( const float& DeltaTime )
{
    (void)DeltaTime;

    // When the currently playing animation ends, need to set another animation
    if ( m_AnimPlayer.IsAtEnd() )
    {
        // see if we need to throw this weapon away (if it's dual)
        new_weapon *pWeapon = GetWeaponPtr(m_CurrentWeaponItem);

        // if this is a dual, we're out of ammo and we've thrown a grenade, get rid of it.        
        if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0) )
        {
            if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
            {
                // Discard
                SetAnimState( ANIM_STATE_DISCARD );
                m_NextAnimState = ANIM_STATE_DISCARD;

                return;
            }            
        }

        // see if we interrupted a reload
        if( !ReloadWeapon(new_weapon::AMMO_PRIMARY) )
        {
            SetAnimState( GetMotionTransitionAnimState() );
        }
    }
}

void player::EndAltGrenade()
{

}

void player::BeginMelee()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    SetAnimation( ANIM_STATE_MELEE, ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }

    // just make sure we have a clean slate
    ClearCombo();
}

void player::UpdateMelee( const float& DeltaTime )
{
    // When the currently playing animation ends, need to set another animation
    if ( m_AnimPlayer.IsAtEnd() )
    {
        // try to reload the weapon (just in case we interrupted it)
        if( !ReloadWeapon(new_weapon::AMMO_PRIMARY) )
        {
            SetAnimState( GetMotionTransitionAnimState() );
        }
    }
}

void player::EndMelee()
{
    m_PlayMeleeSound = true;
    
    // clear combo stuff
    if( !m_bHitCombo || !m_bLastMeleeHit )
    {
        ClearCombo();
    }
    // clear flag
    m_bHitCombo = false;
}

//------------------------------------------------------------------------------
void player::BeginCombo()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    m_ComboCount++;

    // clear flags
    m_bCanRequestCombo = false;
    m_bHitCombo = false;

    SetAnimation( ANIM_STATE_COMBO_BEGIN, ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }
}

void player::UpdateCombo( const float& rDeltaTime )
{
    // When the currently playing animation ends, need to set another animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        if( !m_bHitCombo || !m_bLastMeleeHit )
        {
            SetAnimState(ANIM_STATE_COMBO_END);
        }
        else
        {
            SetAnimState(ANIM_STATE_COMBO_HIT);
        }
    }
}

void player::EndCombo()
{
    // clear combo stuff
    if( !m_bHitCombo || !m_bLastMeleeHit )
    {
        ClearCombo();
    }

    m_PlayMeleeSound = true;

    // clear flag
    m_bHitCombo = false;
}

void player::ClearCombo()
{
    m_ComboCount = 0;
    m_bHitCombo = false;
    m_bLastMeleeHit = false;
}

void player::BeginCombo_Hit()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    m_ComboCount++;

    // clear flags
    m_bCanRequestCombo = false;
    m_bHitCombo = false;

    SetAnimation( ANIM_STATE_COMBO_HIT, ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }
}

void player::UpdateCombo_Hit( const float& rDeltaTime )
{
    // When the currently playing animation ends, need to set another animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        SetAnimState(ANIM_STATE_COMBO_END);
    }
}

void player::EndCombo_Hit()
{
    // clear combo stuff
    if( !m_bHitCombo || !m_bLastMeleeHit )
    {
        ClearCombo();
    }

    m_PlayMeleeSound = true;
}


void player::BeginCombo_End()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    SetAnimation( ANIM_STATE_COMBO_END, ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }
}

void player::UpdateCombo_End( const float& rDeltaTime )
{
    // When the currently playing animation ends, need to set another animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        // try to reload the weapon (just in case we interrupted it)
        if( !ReloadWeapon(new_weapon::AMMO_PRIMARY) )
        {
            SetAnimState( GetMotionTransitionAnimState() );
        }
    }
}

void player::EndCombo_End()
{
    ClearCombo();
}


void player::BeginMelee_Special( const animation_state& AnimState )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->ClearZoom();
    }

    m_bMeleeLunging = true;
}

//------------------------------------------------------------------------------

void player::UpdateMelee_Special( const float& DeltaTime, const animation_state& AnimState )
{
    // When the currently playing animation ends, need to set another animation
    if ( m_AnimPlayer.IsAtEnd() )
    {
        SetAnimState( GetMotionTransitionAnimState() );
    }
}

//------------------------------------------------------------------------------

void player::EndMelee_Special( const animation_state& AnimState )
{
    // clear FOV effect
    m_ViewInfo.XFOV = m_OriginalViewInfo.XFOV;

    // tell the weapon we are done
    // IJB GetMutationMeleeWeapon()->SetMeleeComplete(true);

    m_bMeleeLunging = false;
}

void player::BeginReload()
{
    // lookup the weapon we are using.
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // Make sure the weapon we are switching to is visible
    if( pWeapon )
    {
        pWeapon->SetVisible(true);

        // started reload, initialize flag to false so that we know the reload hasn't finished
        pWeapon->SetReloadCompleted(false);
    }   

    if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RELOAD_IN ) && m_NeedRelaodIn )
    {
        SetAnimState( ANIM_STATE_RELOAD_IN );
    }
    else
    {
        m_NextAnimState = ANIM_STATE_UNDEFINED;

        // Set appropriate RELOAD animation
        SetAnimation( ANIM_STATE_RELOAD , ANIM_PRIORITY_DEFAULT );
    }
}

//------------------------------------------------------------------------------

void player::UpdateReload( const float& DeltaTime )
{
    // don't allow player to switch weapons, zoom in, attack, etc.
    if( m_bHidePlayerArms )
    {
        return;
    }

    //check if we need to go back to a firing state
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( IsFiring() && 
        (pWeapon && pWeapon->IsWeaponReady( pWeapon->GetPrimaryAmmoPriority() )) )
    {
        // if it's the gauss, ramp up
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RAMP_UP ) &&
             m_CurrentWeaponItem != INVEN_WEAPON_MESON_CANNON )
        {
            SetAnimState( ANIM_STATE_RAMP_UP );
        }
        else
        {
            SetAnimState( ANIM_STATE_FIRE );
        }
    }
    else if ( IsAltFiring() &&
        (pWeapon && pWeapon->IsWeaponReady( pWeapon->GetSecondaryAmmoPriority() )) &&
         m_CurrentWeaponItem != INVEN_WEAPON_MESON_CANNON)
    {
        // We have to go through the reload outs.
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_UP ) )
        {
            SetAnimState( ANIM_STATE_ALT_RAMP_UP );
        }
        else if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_FIRE ) )
        {
            SetAnimState( ANIM_STATE_ALT_FIRE );
        }
        else if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ZOOM_IN ) )
        {
            SetAnimState( ANIM_STATE_ZOOM_IN );
        }
    }

    bool bReloaded = false;

    /* IJB
    if( pWeapon && pWeapon->IsReloadCompleted() && 
        !pWeapon->IsKindOf(weapon_shotgun::GetRTTI()) ) // ignore this for shotgun, it behaves differently 
    {
        // we completed the reload, we can fire without worrying about 
        // interrupting the reload just because the "look good" part of the anim is still playing
        pWeapon->Reload( new_weapon::AMMO_PRIMARY );
        pWeapon->Reload( new_weapon::AMMO_SECONDARY );
        bReloaded = true;

    }
*/
    if( pWeapon && m_AnimPlayer.IsAtEnd() )
    {
        // if we haven't reloaded yet, go for it
        if( !bReloaded )
        {
            pWeapon->Reload( new_weapon::AMMO_PRIMARY );
            pWeapon->Reload( new_weapon::AMMO_SECONDARY );
        }

        if( pWeapon->ContinueReload() )
        {
            //  x_DebugMsg( "Playing Reload Anim\n" );
            SetAnimation( ANIM_STATE_RELOAD , ANIM_PRIORITY_DEFAULT );
        }
        else
        {
            // Set to new animation state.
            if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RELOAD_OUT ) )
                SetAnimState( ANIM_STATE_RELOAD_OUT );
            else
                SetAnimState( GetMotionTransitionAnimState() );
        }
    }

    UseFocusObject();
}

void player::EndReload()
{
    m_NeedRelaodIn = true;
}

void player::BeginReloadIn()
{
    // Set appropriate animation
    SetAnimation( ANIM_STATE_RELOAD_IN , ANIM_PRIORITY_DEFAULT );
}

void player::UpdateReloadIn( const float& DeltaTime )
{
    (void)DeltaTime;

    if ( m_AnimPlayer.IsAtEnd() )
    {
        SetAnimState( ANIM_STATE_RELOAD );
    }
}

void player::EndReloadIn()
{
    m_NeedRelaodIn = false;
}

void player::BeginReloadOut()
{
    // Set appropriate animation
    SetAnimation( ANIM_STATE_RELOAD_OUT , ANIM_PRIORITY_DEFAULT );
}

void player::UpdateReloadOut( const float& DeltaTime )
{
    (void)DeltaTime;

    if ( m_AnimPlayer.IsAtEnd() )
    {
        SetAnimState( GetMotionTransitionAnimState() );
    }
}

void player::EndReloadOut()
{

}

void player::BeginRampUp()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;

    SetAnimation( ANIM_STATE_RAMP_UP , ANIM_PRIORITY_DEFAULT );
}

void player::UpdateRampUp( const float& DeltaTime )
{
    // If the 'fire' button is being pressed, we go to ANIM_STATE_FIRE,
    // otherwise, we to to AMIM_STATE_RAMP_DOWN
    if ( IsFiring() )
    {        
        if ( m_AnimPlayer.IsAtEnd() )
        {
            if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_HOLD ) )
            {
                // Switch to hold anim
                SetAnimState( ANIM_STATE_HOLD );
            }
            else
            {
                SetAnimState( ANIM_STATE_FIRE );
            }
        }
    }
    else
    {

        // Get the hand and the weapon parametric values.
        float RampPercent = m_AnimPlayer.GetFrameParametric();
        float WeaponRampPer = 1.0f;
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if ( pWeapon )
            WeaponRampPer = pWeapon->GetFrameParametric();

        SetAnimState( ANIM_STATE_RAMP_DOWN );
        //x_DebugMsg( "Ramp Down percent [%f] Weapon [%f]\n", RampPercent, WeaponRampPer );

        // Set the weapons and the guns start frame.
        m_AnimPlayer.SetFrameParametric( (1.0f-RampPercent) );
        if ( pWeapon )
            pWeapon->SetFrameParametric( (1.0f-WeaponRampPer) );
    }
}

void player::EndRampUp()
{
}

void player::BeginRampDown()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        pWeapon->SetVisible(true);
    }   

    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_RAMP_DOWN , ANIM_PRIORITY_DEFAULT );
}

void player::UpdateRampDown( const float& DeltaTime )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if( m_AnimPlayer.IsAtEnd() )
    {
        bool bUsedFocusObject = UseFocusObject();
        bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

        if( bPressedReload && !NearMutagenReservoir() )
        {
            // If we can reload, do so.
            // Otherwise, play the idle animation.
            if ( pWeapon && pWeapon->CanReload( new_weapon::AMMO_PRIMARY ) )
            {
                m_NextAnimState = ANIM_STATE_RELOAD;
            }
            else
            {
                m_NextAnimState = GetMotionTransitionAnimState();
            }
        }
        else if( pWeapon && IsFiring() )
        {
            if( pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0 )
            {
                // If we can reload, do so.
                // Otherwise, play the idle animation.
                if ( pWeapon->CanReload( new_weapon::AMMO_PRIMARY ) )
                {
                    m_NextAnimState = ANIM_STATE_RELOAD;
                }
                else
                {
                    m_NextAnimState = GetMotionTransitionAnimState();
                }
            }
            else
            {
                // Do we need to handle any ramping?
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RAMP_UP ) )
                {
                    m_NextAnimState = ANIM_STATE_RAMP_UP;
                }
                else
                {
                    m_NextAnimState = ANIM_STATE_FIRE;
                }
            }
        }
        else
        {
            m_NextAnimState = GetMotionTransitionAnimState();
        }

        SetAnimState( m_NextAnimState );
    }
}

void player::EndRampDown()
{
}

void player::BeginAltRampUp()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_ALT_RAMP_UP , ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // tell the weapon we are ramping up (currently only for meson cannon).
    if( pWeapon )
    {
        pWeapon->BeginAltRampUp();
    }
}

void player::UpdateAltRampUp( const float& DeltaTime )
{
    // If the 'fire' button is being pressed, we go to ANIM_STATE_FIRE,
    // otherwise, we to to AMIM_STATE_RAMP_DOWN
    if ( IsAltFiring() )
    {
        if ( m_AnimPlayer.IsAtEnd() )
        {
            if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_HOLD ) )
            {
                // Switch to hold anim
                SetAnimState( ANIM_STATE_ALT_HOLD );
            }
            else
            {
                SetAnimState( ANIM_STATE_ALT_FIRE);
            }
        }
    }
    else
    {
        SetAnimState(ANIM_STATE_ALT_FIRE);
    }
}

//------------------------------------------------------------------------------

void player::EndAltRampUp()
{
    bool bGoingIntoHold   = (m_NextAnimState == ANIM_STATE_ALT_HOLD);
    bool bSwitchingWeapon = (m_NextAnimState == ANIM_STATE_SWITCH_FROM);

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // tell the weapon we are finished ramping up (currently only for meson cannon).
    if( pWeapon )
    {
        pWeapon->EndAltRampUp(bGoingIntoHold, bSwitchingWeapon);
    }
}

void player::BeginAltRampDown()
{
    float fBlendTime;
    if ( m_PreviousAnimState == ANIM_STATE_ALT_RAMP_UP )
    {
        fBlendTime = DEFAULT_BLEND_TIME;
    }
    else
    {
        fBlendTime = 0.f;
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_ALT_RAMP_DOWN, ANIM_PRIORITY_DEFAULT, fBlendTime );
}

void player::UpdateAltRampDown( const float& DeltaTime )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if( m_AnimPlayer.IsAtEnd() )
    {
        bool bUsedFocusObject = UseFocusObject();
        bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

        if( bPressedReload && !NearMutagenReservoir() )
        {
            // Time to reload or go to fire empty.
            // If the current weapon needs to be reloaded, set the next state to reload
            // We are out of ammo, check if we have a clip left.  If not then go to idle.
            if( pWeapon && pWeapon->CanReload( new_weapon::AMMO_SECONDARY ) )
            {
                m_NextAnimState = ANIM_STATE_RELOAD;
            }
            else
            {
                m_NextAnimState = GetMotionTransitionAnimState();
            }
        }
        else if( IsAltFiring() )
        {
            if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetSecondaryAmmoPriority() ) <= 0) )
            {
                // We are out of ammo, check if we have a clip left.  If not then go to idle.
                if( pWeapon->CanReload( new_weapon::AMMO_SECONDARY ) )
                {
                    m_NextAnimState = ANIM_STATE_RELOAD;
                }
                else
                {
                    m_NextAnimState = GetMotionTransitionAnimState();
                }
            }
            else
            {
                // Do we need to handle any ramping?
                if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_UP ) )
                {
                    m_NextAnimState = ANIM_STATE_ALT_RAMP_UP;
                }
                else
                {
                    m_NextAnimState = ANIM_STATE_ALT_FIRE;
                }
            }

        }
        else
        {
            m_NextAnimState = GetMotionTransitionAnimState();
        }

        SetAnimState( m_NextAnimState );
    }
}

void player::EndAltRampDown()
{
}

void player::BeginHold()
{
    m_AnimStage = 1;
    m_WpnHoldTime = 0.0f;

    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_HOLD , ANIM_PRIORITY_DEFAULT );
}

void player::UpdateHold( const float& DeltaTime )
{
    m_WpnHoldTime += DeltaTime;

    if (m_AnimStage == 1)
    {
        // Isthe player still holding the fire button?
        bool bFiring = IsFiring();

        if ( !bFiring )
        {
            m_AnimStage++;
            SetAnimState( ANIM_STATE_FIRE );
        }
        else
        {
            // Wait until the current animtion ends, then switch the animation
            if( m_AnimPlayer.IsAtEnd() )
            {
                state_anims& State  = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_HOLD];
                int nAnimIndex      = ANIM_PRIORITY_DEFAULT;

                // If we have multiple hold animation, increase the anim index as time increases.
                if ( m_AnimPlayer.GetAnimIndex() == State.PlayerAnim[ANIM_PRIORITY_DEFAULT] && State.nPlayerAnims > 1 )
                {
                    nAnimIndex = (int)((m_WpnHoldTime/m_MaxAnimWeaponHoldTime) * (float)(State.nPlayerAnims - 1));
                }

                SetAnimation( ANIM_STATE_HOLD, nAnimIndex );
            }
        }
    }
    else
    {
        if ( m_AnimPlayer.IsAtEnd() )
        {
            SetAnimState( ANIM_STATE_RAMP_DOWN );
        }
    }
}

void player::EndHold()
{
    m_WpnHoldTime = 0.0f;
}

void player::BeginAltHold()
{
    m_AnimStage = 1;
    m_WpnHoldTime = 0.0f;

    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_ALT_HOLD, ANIM_PRIORITY_DEFAULT );
}

void player::UpdateAltHold( const float& DeltaTime )
{
    m_WpnHoldTime += DeltaTime;

    if (m_AnimStage == 1)
    {
        // Is the player still holding the fire button?
        bool bFiring = IsAltFiring();

        if ( !bFiring )
        {
            SetAnimState( ANIM_STATE_ALT_FIRE );
        }
        else
        {
            // Wait until the current animtion ends, then switch the animation
            if( m_AnimPlayer.IsAtEnd() )
            {
                state_anims& State  = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_ALT_HOLD];
                int nAnimIndex      = ANIM_PRIORITY_DEFAULT;

                // If we have multiple hold animation, increase the anim index as time increases.
                if ( m_AnimPlayer.GetAnimIndex() == State.PlayerAnim[ANIM_PRIORITY_DEFAULT] && State.nPlayerAnims > 1 )
                {
                    nAnimIndex = (int)((m_WpnHoldTime/m_MaxAnimWeaponHoldTime) * (float)(State.nPlayerAnims - 1));
                }

                SetAnimation( ANIM_STATE_ALT_HOLD, nAnimIndex );
            }
        }
    }
    else
    {
        if ( m_AnimPlayer.IsAtEnd() )
        {
            SetAnimState( ANIM_STATE_ALT_RAMP_DOWN );
        }
    }
}

void player::EndAltHold()
{
    m_WpnHoldTime = 0.0f;
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // tell the weapon we are ending the hold (currently only for meson cannon).
    if( pWeapon )
    {
        bool bSwitchingWeapon = (m_NextAnimState == ANIM_STATE_SWITCH_FROM);

        if( !bSwitchingWeapon )
        {
            BeginAltFire();
        }

        pWeapon->EndAltHold( bSwitchingWeapon );
    }
}

void player::BeginFallingToDeath()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetupThirdPersonCamera();

    if ( m_pLoco && UsingLoco() )
    {
        m_pLoco->SetState( loco::STATE_IDLE );
        m_pLoco->PlayDeathAnim( loco::ANIM_DEATH_EXPLOSION );
    }
}

void player::UpdateFallingToDeath( float DeltaTime )
{
    (void)DeltaTime;

    if ( GetThirdPersonCamera() )
    {
        UpdateThirdPersonCamera();
        GetThirdPersonCamera()->MoveTowardsPitch( -m_Physics.GetVelocity().GetPitch() );
    }
}

void player::EndFallingToDeath()
{
}

void player::BeginZoomIn()
{
    m_NextAnimState = ANIM_STATE_UNDEFINED;
    // Play the zoom in animation for this state
    SetAnimation( ANIM_STATE_ZOOM_IN , ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if( pWeapon )
    {
        pWeapon->IncrementZoom();
        pWeapon->ZoomInComplete(false);
    }
}

void player::UpdateZoomIn( const float& DeltaTime )
{
    (void)DeltaTime;

    // Wait until the current animtion ends, then switch the animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        float     VelocitySquared = m_Physics.GetVelocity().LengthSquared();
        if( VelocitySquared < m_fMinRunSpeed )
            SetAnimState( ANIM_STATE_ZOOM_IDLE );
        else
            SetAnimState( ANIM_STATE_ZOOM_RUN );
    }
}

void player::EndZoomIn()
{   
    LoadAimAssistTweakHandles();
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if( pWeapon )
    {
        pWeapon->ZoomInComplete(true);
    }
}

void player::EndZoomState()
{
    SetAnimState(ANIM_STATE_ZOOM_OUT);
    m_NextAnimState = ANIM_STATE_IDLE;
}

void player::BeginZoomOut()
{
    assert( m_NextAnimState != ANIM_STATE_UNDEFINED );

    // Play the zoom in animation for this state
    SetAnimation( ANIM_STATE_ZOOM_OUT , ANIM_PRIORITY_DEFAULT );

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if ( pWeapon )
    {
        while ( pWeapon->IncrementZoom() > 0 )
            ;
    }
}

void player::UpdateZoomOut( const float& DeltaTime )
{
    (void)DeltaTime;
    assert( m_NextAnimState != ANIM_STATE_UNDEFINED );

    // Wait until the current animtion ends, then switch the animation
    if( m_AnimPlayer.IsAtEnd() )
    {
        SetAnimState( m_NextAnimState );    
    }
}

void player::EndZoomOut()
{
    LoadAimAssistTweakHandles();
}

void player::BeginZoomIdle()
{
    // Play the zoom in animation for this state
    SetAnimation( ANIM_STATE_ZOOM_IDLE , ANIM_PRIORITY_DEFAULT );

    // this is a zoom idle, but, we're still idling
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && !pWeapon->GetIdleMode() )
        pWeapon->BeginIdle();
}

void player::UpdateZoomIdle( const float& DeltaTime )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // Check if we need to go back to a firing state
    if( IsFiring() )
    {
        if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) > 0) )
        {
            SetAnimState( ANIM_STATE_ZOOM_FIRE );
        }
        else
        {
            if( pWeapon && pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority() ) )
            {
                SetAnimState( ANIM_STATE_ZOOM_OUT );
                m_NextAnimState = ANIM_STATE_RELOAD;
            }
            else
            {
                // IJB  g_AudioMgr.Play( "NoAmmo" );
            }
        }
    }
    else if ( IsAltFiring() && (m_DebounceTime > s_DebounceDuration) )
    {
        // If we're at our max zoom, then zoom out, otherwise zoom some more
        if ( pWeapon->GetZoomStep() == pWeapon->GetnZoomSteps() )
        {
            SetAnimState( ANIM_STATE_ZOOM_OUT );
            m_NextAnimState = ANIM_STATE_IDLE;
        }
        else
        {
            if ( pWeapon )
            {
                pWeapon->IncrementZoom();
                m_DebounceTime = 0.0f;

                // zoomed, update tweaks
                LoadAimAssistTweakHandles();
            }
        }
    }

    bool bUsedFocusObject = UseFocusObject();
    bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

    // Reloading?
    if( bPressedReload && !NearMutagenReservoir() )
    {
        // Time to reload or go to fire empty.
        // If the current weapon needs to be reloaded, set the next state to reload
        if( pWeapon 
            && (   (pWeapon->CanReload( pWeapon->GetSecondaryAmmoPriority() )) 
            || (pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority()   ))) )
        {
            SetAnimState( ANIM_STATE_ZOOM_OUT );
            m_NextAnimState = ANIM_STATE_RELOAD;
        }
    }

    // Check if we need to change to another motion animation.
    if ( m_Physics.GetVelocity().LengthSquared() > m_fMinRunSpeed )
    {
        SetAnimState( ANIM_STATE_ZOOM_RUN );
        return;
    }

    // Watch the animation timers and player's motion.  Switch the animation if necessary.
    m_fAnimationTime += DeltaTime;

    if ( m_fAnimationTime > m_fMaxAnimTime )
    {
        // Wait until the current animtion ends, then switch the animation
        if( m_AnimPlayer.IsAtEnd() )
        {
            SetAnimation( ANIM_STATE_ZOOM_IDLE , ANIM_PRIORITY_DEFAULT );
        }
    }

    ReloadWeapon(new_weapon::AMMO_PRIMARY);
}

void player::EndZoomIdle()
{
    // this is a zoom idle, but, we're still idling
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( (m_NextAnimState != ANIM_STATE_RUN) && pWeapon )
        pWeapon->EndIdle();
}

void player::BeginZoomRun()
{
    // Play the zoom in animation for this state
    SetAnimation( ANIM_STATE_ZOOM_RUN , ANIM_PRIORITY_DEFAULT );

    // this is a zoom idle, but, we're still idling
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && !pWeapon->GetIdleMode() )
        pWeapon->BeginIdle();
}

void player::UpdateZoomRun( const float& DeltaTime )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // Check if we need to go back to a firing state
    if( IsFiring() )
    {
        if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) > 0) )
        {
            SetAnimState( ANIM_STATE_ZOOM_FIRE );
        }
        else
        {
            if( pWeapon && pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority() ) )
            {
                SetAnimState( ANIM_STATE_ZOOM_OUT );
                m_NextAnimState = ANIM_STATE_RELOAD;
            }
            else
            {
                // IJB g_AudioMgr.Play( "NoAmmo" );
            }
        }
    }
    else if ( IsAltFiring() && (m_DebounceTime > s_DebounceDuration) )
    {
        // If we're at our max zoom, then zoom out, otherwise zoom some more
        if ( pWeapon->GetZoomStep() == pWeapon->GetnZoomSteps() )
        {
            SetAnimState( ANIM_STATE_ZOOM_OUT );
            m_NextAnimState = ANIM_STATE_IDLE;
        }
        else
        {
            if ( pWeapon )
            {
                pWeapon->IncrementZoom();
                m_DebounceTime = 0.0f;

                // zoomed, update tweaks
                LoadAimAssistTweakHandles();
            }
        }
    }

    bool bUsedFocusObject = UseFocusObject();
    bool bPressedReload   = !bUsedFocusObject && (g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_RELOAD ).WasValue > 0.0f); 

    // Reloading?
    if( bPressedReload && !NearMutagenReservoir() )
    {
        // Time to reload or go to fire empty.
        // If the current weapon needs to be reloaded, set the next state to reload
        if( pWeapon 
            && (   (pWeapon->CanReload( pWeapon->GetSecondaryAmmoPriority() )) 
            || (pWeapon->CanReload( pWeapon->GetPrimaryAmmoPriority()   ))) )
        {
            SetAnimState( ANIM_STATE_ZOOM_OUT );
            m_NextAnimState = ANIM_STATE_RELOAD;
        }
    }

    // Check if we need to change to another motion animation.
    if ( m_Physics.GetVelocity().LengthSquared() < m_fMinRunSpeed )
    {
        SetAnimState( ANIM_STATE_ZOOM_IDLE );
        return;
    }

    // Watch the animation timers and player's motion.  Switch the animation if necessary.
    m_fAnimationTime += DeltaTime;

    if ( m_fAnimationTime > m_fMaxAnimTime )
    {
        // Wait until the current animtion ends, then switch the animation
        if( m_AnimPlayer.IsAtEnd() )
        {
            SetAnimation( ANIM_STATE_ZOOM_RUN , ANIM_PRIORITY_DEFAULT );
        }
    }

    ReloadWeapon(new_weapon::AMMO_PRIMARY);
}

void player::EndZoomRun()
{
    // this is a zoom idle, but, we're still idling
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( (m_NextAnimState != ANIM_STATE_RUN) && pWeapon )
        pWeapon->EndIdle();
}

void player::BeginZoomFire()
{
    // Generate the percentages used to determine what firing animations are being played.
    GenerateFiringAnimPercentages();

    // we can't tap fire again until release
    m_bCanTapFire = false;

    // reset timer
    m_TapRefireTime = 0.0f;

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if ( pWeapon )
    {
        pWeapon->BeginPrimaryFire();   
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;
    SetAnimation( ANIM_STATE_ZOOM_FIRE, ANIM_PRIORITY_DEFAULT, 0.0f );
}


void player::UpdateZoomFire( const float& DeltaTime )
{
    m_TapRefireTime += DeltaTime;

    // If the current weapon needs to be reloaded, set the next state to reload
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon && (pWeapon->GetAmmoCount( pWeapon->GetPrimaryAmmoPriority() ) <= 0) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_DISCARD ) )
        {
            // Discard
            SetAnimState( ANIM_STATE_DISCARD );
            m_NextAnimState = ANIM_STATE_DISCARD;
        }
        else
        {
            m_NextAnimState =  GetMotionTransitionAnimState();
        }
    }

    // Is the player still holding the fire button?
    bool bFiring = IsFiring();

    // Do we need to reload?  If the weapon has a ramp down play that first.
    // If we run out of ammo and can't reload go to the Idle state.
    if( m_NextAnimState == ANIM_STATE_RELOAD )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_ALT_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }
        return;
    }
    else if( (m_NextAnimState == ANIM_STATE_IDLE) || (m_NextAnimState == ANIM_STATE_RUN) )
    {
        if( IsAnimStateAvailable2( m_CurrentWeaponItem, ANIM_STATE_RAMP_DOWN ) )
        {
            SetAnimState( ANIM_STATE_RAMP_DOWN );
        }
        else
        {
            SetAnimState( m_NextAnimState );
        }

        // see if we need to reload
        ReloadWeapon(new_weapon::AMMO_PRIMARY);

        // Special case:  We are zoomed in and out of ammo.
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if( pWeapon )
        {
            // Clear zoom ourselves because no reload is going to occur.
            if( ( pWeapon->GetAmmoAmount( new_weapon::AMMO_PRIMARY ) == 0 ) )
            {
                pWeapon->ClearZoom();
            }
        }

        return;
    }

    // When the currently playing animation ends, need to set another animation
    if( bFiring )
    {
        // Still firing away.
        if( m_AnimPlayer.IsAtEnd() || 
            (pWeapon->CanFastTapFire() && m_bCanTapFire && (m_TapRefireTime >= TapRefireRefreshSeconds_Tweak.GetF32())) )
        {
            state_anims& WeaponState = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][ANIM_STATE_ZOOM_FIRE];
            if( WeaponState.nWeaponAnims > 1 )
            {
                if( m_LastFireAnimStateIndex >= WeaponState.nWeaponAnims )
                    m_LastFireAnimStateIndex = 0;

                SetAnimation( ANIM_STATE_ZOOM_FIRE, m_LastFireAnimStateIndex, 0.0f );
                m_LastFireAnimStateIndex++;
            }
            else
            {
                SetAnimation( ANIM_STATE_ZOOM_FIRE, ANIM_PRIORITY_DEFAULT, 0.0f );
            }

            // we can't tap fire again until release
            m_bCanTapFire = false;

            // reset timer
            m_TapRefireTime = 0.0f;
        }        
    }
    else
    {
        // we've relased the button, signal a tap fire.
        m_bCanTapFire = true; 
    
        if( pWeapon && pWeapon->CanIntereptPrimaryFire( m_CurrentAnimIndex ) )
        {
            float     VelocitySquared = m_Physics.GetVelocity().LengthSquared();
            if( VelocitySquared < m_fMinRunSpeed )
                SetAnimState( ANIM_STATE_ZOOM_IDLE );
            else
                SetAnimState( ANIM_STATE_ZOOM_RUN );
        }
    }
}

void player::EndZoomFire()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {
        pWeapon->EndPrimaryFire();
    }        
}

void player::SetAnimState( animation_state AnimState )
{
    // don't set the state if we're already in it
    if ( m_CurrentAnimState == AnimState )
        return;


    if ( m_bDead && (AnimState != ANIM_STATE_MISSION_FAILED) )
    {      
        AnimState = ANIM_STATE_DEATH;
    }

    if (   (m_CurrentWeaponItem == INVEN_WEAPON_MUTATION    ) 
        && (m_CurrentAnimState  == ANIM_STATE_SWITCH_TO     )
        && (AnimState           != ANIM_STATE_SWITCH_FROM   )
        && (m_Health.GetHealth() > 0.0f)
        && (!m_bIsMutated       || !m_bIsMutantVisionOn     ) )
    {
        // This is a catch-all for the rare instances
        // when the change to mutation is inturrupted
        // by some other code. When this happens, we
        // need to force the rest of the mutation 
        // change.
        m_bIsMutated        = true;
        m_bIsMutantVisionOn = true;
        
        // Send over net
        SetMutated( true );
    }

    m_NextAnimState = AnimState;

    //now end the state that we're in.
    EndState();

    //Set new state
    m_PreviousAnimState = m_CurrentAnimState;
    m_CurrentAnimState = AnimState;

    BeginState();
}

//==============================================================================


//==============================================================================
//==============================================================================
//  ANIM_STATE_CINEMA
//==============================================================================
//==============================================================================
void player::BeginCinema()
{
    // KSS -- new cinema code
    {
        // clear out correction delta
        m_Cinema.m_ViewCorrectionDelta.Zero();        
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    HudObject* Hud = GetHud();

    if( Hud )
    {
        Hud->SetupLetterBox( true, 0.8f );
    }

    // Setup current look direction
    m_Cinema.m_CurrentLookDir.set(0,0,1);
    m_Cinema.m_CurrentLookDir.RotateX( GetPitch() );
    m_Cinema.m_CurrentLookDir.RotateY( GetYaw() );

    m_Cinema.m_CurrentBlendInTime = 0.0f;

    m_Physics.SetSolveActorCollisions( false );
    m_Physics.SetLocoCollisionOn( false );
    m_Physics.SetLocoGravityOn(false) ;
    // Kick the weapon into idle
    {
        //Set the animation timers and 
        m_fAnimationTime = 0.f;
        m_fMaxAnimTime = x_frand( s_idle_timeout_min , s_ilde_timout_max );

        m_NextAnimState = ANIM_STATE_UNDEFINED;

        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if( pWeapon && !pWeapon->GetIdleMode() )
        {
            // begin idle, but, tell the weapon this isn't a "normal" idle (i.e. a cinematic)
            pWeapon->BeginIdle(false);
            pWeapon->ClearZoom();
        }

        // Play the default idle animation for this state
        SetAnimation( ANIM_STATE_IDLE , ANIM_PRIORITY_DEFAULT );
    }
    // set our zone to the camera thinghy.
    Object* pCamera = objectManager->GetObjectByGuid( m_Cinema.m_CinemaCameraGuid );
    if( pCamera )
    {
        SetZone1(pCamera->GetZone1());
        SetZone2(pCamera->GetZone2());
        InitZoneTracking();
    }
}

//------------------------------------------------------------------------------

// This is roughly the offset height from the players feet to his eye
Vector3 g_PlayerViewOffset( 0.0f, -198.6f, 0.0f );

void player::UpdateCinema( float DeltaTime )
{
    //
    // Note: we may want to add acceleration into this.
    //

    if( m_Cinema.m_bCinemaOn==false )
    {
        // Make sure zone flag is cleared ready for next cinema
        m_Cinema.m_bPlayerZoneInitialized = false;

        // Goto idle
        SetAnimState( ANIM_STATE_IDLE );       
        return;
    }

    // KSS -- new cinema code
    
    // Get the camera position and yaw and keep player near
    Object* pObject = objectManager->GetObjectByGuid( m_Cinema.m_CinemaCameraGuid );
    if( pObject )
    {
        // SB: Keep camera info for "player::ComputeView"
        m_Cinema.m_CameraV2W  = pObject->GetL2W();
        m_Cinema.m_CameraXFOV = m_ViewInfo.XFOV;
        
        // SB: If this is a real camera object, then get the field of view also
        /* IJB
        object_ptr<camera> pCamera = objectManager->GetObjectByGuid( m_Cinema.m_CinemaCameraGuid );
        if( pCamera )
        {
            m_Cinema.m_CameraXFOV = pCamera->GetView().GetXFOV();
        }
        */
        // Get info from camera V2W
        Vector3 CameraPos = m_Cinema.m_CameraV2W.GetTranslation();
        Radian3 CameraRot = m_Cinema.m_CameraV2W.GetRotation();
        
        // Setup player rotation from camera
        SetYaw( CameraRot.yaw );
        SetPitch( CameraRot.pitch );

        Matrix4 L2W;
        L2W.Identity();
        L2W.RotateY( CameraRot.yaw );
        L2W.Translate( CameraPos + g_PlayerViewOffset );

        OnTransform( L2W );
        m_Yaw = L2W.GetRotation().yaw;
        m_AnimPlayer.SetYaw( m_fCurrentYawOffset + m_Yaw );
        OnMoveViewPosition( L2W.GetTranslation() );
        
        // Update the eyes so that zone tracking works
        m_EyesPosition  = CameraPos;
        
        // Teleport player's zone to camera zone?
        if( !m_Cinema.m_bPlayerZoneInitialized )
        {
            // Initialize all zone related members to be where the camera is
            m_ZoneTracker.SetMainZone( (uint8_t)pObject->GetZone1() );
            m_ZoneTracker.SetZone2   ( (uint8_t)pObject->GetZone2() );
            m_ZoneTracker.SetPosition( pObject->GetPosition() );
            SetZone1( (uint8_t)pObject->GetZone1() );
            SetZone2( (uint8_t)pObject->GetZone2() );
            
            // Flag as initialized
            m_Cinema.m_bPlayerZoneInitialized = true;
        }

        // Update zone tracking to follow the camera        
        UpdateZoneTrack();
    }

    // Make camera look at a specific object?
    {
        // Get position of target object to look at
        Object* pObj = objectManager->GetObjectByGuid( m_Cinema.m_LookAtTargetGuid );
        if( pObj )
        {
            // Setup current look direction
            m_Cinema.m_CurrentLookDir.set(0,0,1);
            m_Cinema.m_CurrentLookDir.RotateX( GetPitch() );
            m_Cinema.m_CurrentLookDir.RotateY( GetYaw() );

            // Compute a new desired look direction
            Vector3 TargetPos = pObj->GetPosition();
            m_Cinema.m_DesiredLookDir = TargetPos - GetEyesPosition();
            m_Cinema.m_DesiredLookDir.Normalize();

            // Interpolate current direction toward desired direction

            // Get angle difference between the vectors
            Radian Angle = v3_AngleBetween( m_Cinema.m_CurrentLookDir, m_Cinema.m_DesiredLookDir );
            assert( Angle >= 0 );

            // Compute axis to rotate around.
            // If we are 180 degrees apart then rotate around Y.
            Vector3 Axis = m_Cinema.m_CurrentLookDir.Cross(m_Cinema.m_DesiredLookDir);
            if( Angle > R_179 )
            {
                Axis.set(0,1,0);
            }

            Axis.Normalize();

            // set up blend interpolation
            float AngleModifierT = 1.0f;

            // no divides by zero or ridiculously small blendin times
            if( m_Cinema.m_BlendInTime >= FLT_MIN )
            {
                AngleModifierT = m_Cinema.m_CurrentBlendInTime / m_Cinema.m_BlendInTime;

                // The closer we get to BlendInTime, the more of the angle difference we want to take.        
                AngleModifierT = std::clamp(AngleModifierT, 0.0f, 1.0f);

                m_Cinema.m_CurrentBlendInTime += DeltaTime;

                // make sure we don't get a weird number
                m_Cinema.m_CurrentBlendInTime = std::clamp(m_Cinema.m_CurrentBlendInTime, 0.0f, m_Cinema.m_BlendInTime);
            }

            // Rotate current towards desired but use the AngleModifierT to ramp speed 
            // (i.e. if m_CurrentBlenInTime = m_BlendInTime then AngleModifierT = 1)
            Angle =  (AngleModifierT * Angle);
            Quaternion Q(Axis,Angle);
            m_Cinema.m_CurrentLookDir = Q * m_Cinema.m_CurrentLookDir;

            // Pull out pitch and yaw from new look direction and feed to player
            Radian Pitch;
            Radian Yaw;
            m_Cinema.m_CurrentLookDir.GetPitchYaw( Pitch, Yaw );
            
            // SB: Keep camera info for "player::ComputeView"
            Vector3 CameraPos = m_Cinema.m_CameraV2W.GetTranslation();
            Radian3 CameraRot = m_Cinema.m_CameraV2W.GetRotation();
            CameraRot.pitch = Pitch;
            CameraRot.yaw   = Yaw;
            m_Cinema.m_CameraV2W.Setup( Vector3( 1.0f, 1.0f, 1.0f ),
                                        CameraRot,
                                        CameraPos );
            
            // Tell player the new pitch and yaw
            SetPitch( Pitch );
            SetYaw( Yaw );
        }
    }
}

void player::EndCinema()
{
    // Clear cinema values
    m_Cinema.m_LookAtTargetGuid = 0;

    // KSS -- new cinema code
    {
        // Snap player to ground
        {
            Vector3 Pos = GetPosition();

            Vector3 S = Pos + Vector3(0,150,0);
            Vector3 E = Pos + Vector3(0,-150,0);
            g_CollisionMgr.SphereSetup( GetGuid(), S, E, 5.0f );    // Use sphere in-case of cracks!
            g_CollisionMgr.UseLowPoly();
            g_CollisionMgr.CheckCollisions( 
                Object::TYPE_ALL_TYPES,                                 // these types
                Object::ATTR_COLLIDABLE,                                // these attributes
                Object::ATTR_PLAYER | Object::ATTR_CHARACTER_OBJECT );  // not these attributes

            if( g_CollisionMgr.m_nCollisions > 0 )
            {
                // Put player slightly above the collision point
                Pos = g_CollisionMgr.m_Collisions[0].Point + Vector3( 0.0f, 0.5f, 0.0f );
            }

            // Move player to final safe pos
            {
                Matrix4 L2W;
                L2W.Identity();
                L2W.RotateY(m_Yaw);
                L2W.Translate(Pos);

                OnTransform( L2W );
                OnMoveViewPosition( L2W.GetTranslation() );
                UpdateZoneTrack();
            }
        }

        // are we using view correction?
        if( m_Cinema.m_bUseViewCorrection )
        {
            // Setup offset to blend from cinema eye position back to player eye position
            m_Cinema.m_ViewCorrectionDelta = m_EyesPosition - GetDefaultViewPos();
        }
        else
        {
            // just make sure
            m_Cinema.m_ViewCorrectionDelta.Zero();
        }
    }

    HudObject* Hud = GetHud();
    m_Physics.SetSolveActorCollisions( true );
    m_Physics.SetLocoCollisionOn( true );
    m_Physics.SetLocoGravityOn(true) ;

    if( Hud )
    {
        Hud->SetupLetterBox( false, 0.8f );
    }

    //
    // Do we need to turn the flashlight back on?
    //
    if ( m_bUsingFlashlightBeforeCinema )
    {
        SetFlashlightActive( true );
        m_bUsingFlashlightBeforeCinema = false;
    }
}

//==============================================================================

void player::BeginMissionFailed()
{
}

//==============================================================================

float DisplayTextTime = 3.0f;
float FadeOutTime = 2.0f;

void player::UpdateMissionFailed( float DeltaTime )
{
    const float LastTimeInState = m_TimeInState - DeltaTime;
    if ( IN_RANGE( LastTimeInState, DisplayTextTime, m_TimeInState ) )
    {
        // IJB g_PostEffectMgr.StartScreenFade( Colour(0,0,0,255), FadeOutTime );
    }
    else if ( IN_RANGE( LastTimeInState, DisplayTextTime + FadeOutTime, m_TimeInState ) )
    {

        // IJB pGameLogic->PlayerDied( m_NetSlot, m_NetSlot, 0 );

        bool PrimaryPressed = (bool)g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::ACTION_PRIMARY ).WasValue;
        // IJB PrimaryPressed |= input_WasPressed( INPUT_MOUSE_BTN_L );

        if( PrimaryPressed )
        {
            m_bWantToSpawn = true;
            m_bRespawnButtonPressed = true;
        }
    }
}

void player::EndMissionFailed()
{
    m_CorpseGuid    = 0;
    m_AnimStage     = 0;
    m_DeathType     = DEATH_BY_ANIM;
}
