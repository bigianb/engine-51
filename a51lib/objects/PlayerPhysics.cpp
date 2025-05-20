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

static float       SlowYawMultiplier = 2.0f;
static float       FastYawMultiplier = 5.2f;
static float       StickModeChange = 0.99f;
static float       InitialYawMultiplier = 0.05f; // the smaller the faster start with a big stick change
static const float s_DistanceAtR25 = 700.0f;

float HumanSpeedFactor = 1.000f;
float MutantSpeedFactor = 1.000f;

tweak_handle LookSpring_ReturnSpeedTweak("LookSpring_ReturnSpeed");

// multi-player movement tweaks
tweak_handle MP_RunSpeedFactor_NormalTweak("MP_RunSpeedFactor_Normal");
tweak_handle MP_StrafeSpeedFactor_NormalTweak("MP_StrafeSpeedFactor_Normal");
tweak_handle MP_RunSpeedFactor_MutantTweak("MP_RunSpeedFactor_Mutant");
tweak_handle MP_StrafeSpeedFactor_MutantTweak("MP_StrafeSpeedFactor_Mutant");

void player::UpdateRotation(const float& rDeltaTime)
{
    Radian OldPitch = m_Pitch;
    Radian OldYaw = m_Yaw;

    float fAimYawOffset = R_0;

    // make sure we load in tweaks in case they've changed
    LoadAimAssistTweaks();

    CalculatePitchLimits(rDeltaTime);
    CalculateRotationAccelerationFactors(rDeltaTime);
    UpdateAimAssistance(rDeltaTime);
    UpdateAimOffset(rDeltaTime);

    //
    // If the current weapon had zoom enable, adjust the Yaw according to the X FOV.
    //
    new_weapon* pWeaponObj = GetCurrentWeaponPtr();

    if (pWeaponObj) {
        if (pWeaponObj->IsZoomEnabled()) {
            m_fYawValue /= (m_OriginalViewInfo.XFOV / pWeaponObj->GetXFOV());
            m_fYawValue *= pWeaponObj->GetZoomMovementMod();
            m_fPitchValue *= pWeaponObj->GetZoomMovementMod();
        }
    }

    if (rDeltaTime < FLT_MIN) {
        assert(0);
    }

    if (m_AimAssistData.TargetGuid != 0) {
        static float s_AimAssistTune = 1.06f;

        fAimYawOffset = m_YawAimOffset * m_fStrafeValue * rDeltaTime * (-s_DistanceAtR25 / m_AimAssistData.LOFPtDist)
                        //* ( -s_DistanceAtR25 / m_DistanceToAimAssistTarget )
                        * m_AimAssistData.TurnDampeningT //m_AimAssistPct
                        * s_AimAssistTune;
    }

    const float AbsYawValue = abs(m_fYawValue);
    const float YawChange = abs(m_fYawValue - m_fPreviousYawValue);

    static float MaxYawChange = 0.0f;
    if (YawChange > MaxYawChange) {
        MaxYawChange = YawChange;
    }

    if ((AbsYawValue > 0.0f) && (AbsYawValue < StickModeChange)) {
        float P = m_fYawValue * SlowYawMultiplier * rDeltaTime;
        m_Yaw += P;
        m_YawAccelFactor = P / (m_fYawValue * FastYawMultiplier * rDeltaTime * m_fCurrentYawAimModifier);
        m_YawAccelFactor = std::min(1.0f, m_YawAccelFactor);
        m_YawAccelFactor = std::max(0.0f, m_YawAccelFactor);
    } else {
        if (YawChange >= StickModeChange) {
            // The stick just moved really far, really fast, give ourselves a nice kick start rather than a soft start
            m_YawAccelFactor = std::max(m_YawAccelFactor, rDeltaTime / (m_YawAccelTime * InitialYawMultiplier));
            m_YawAccelFactor = std::min(1.0f, m_YawAccelFactor);
            m_YawAccelFactor = std::max(0.0f, m_YawAccelFactor);
        }

        float P = m_fYawValue * FastYawMultiplier * rDeltaTime * m_YawAccelFactor * m_fCurrentYawAimModifier;
        m_Yaw += P;
    }

    m_Yaw += fAimYawOffset;

    m_Pitch += m_fPitchValue * rDeltaTime * m_fPitchStickSensitivity * m_PitchAccelFactor * m_fCurrentPitchAimModifier;
    m_Pitch = std::min(m_PitchMax, std::max(m_PitchMin, m_Pitch));

    if (m_bInTurret) {
        // Make sure we're within the turret's boundaries
        Object* pObj;
        float   MinAngleDiff = x_MinAngleDiff(m_Yaw, OldYaw);
        Vector3 EyePos(m_Turret.AnchorL2W.GetTranslation());
        EyePos.y += 150.0f;

        // Left
        if ((MinAngleDiff > 0.0f) // rotating left
            && (pObj = objectManager->GetObjectByGuid(m_Turret.LeftBoundaryGuid))) {
            const Vector3 Pos(pObj->GetPosition());
            const Vector3 ToPos(Pos - EyePos);
            const float   LeftYawBound = ToPos.GetYaw();

            if ((x_MinAngleDiff(m_Yaw, LeftYawBound) > 0.0f)       // Yaw is to the left of the bound
                && (x_MinAngleDiff(LeftYawBound, OldYaw) >= 0.0f)) // OldYaw is to the right of the bound
            {
                // We've just crossed the boundary
                m_Yaw = LeftYawBound;
            }
        }

        // Right
        if ((MinAngleDiff < 0.0f) // rotating right
            && (pObj = objectManager->GetObjectByGuid(m_Turret.RightBoundaryGuid))) {
            const Vector3 Pos(pObj->GetPosition());
            const Vector3 ToPos(Pos - EyePos);
            const float   RightYawBound = ToPos.GetYaw();

            if ((x_MinAngleDiff(m_Yaw, RightYawBound) < 0.0f)       // Yaw is to the right of the bound
                && (x_MinAngleDiff(RightYawBound, OldYaw) <= 0.0f)) // OldYaw is to the left of the bound
            {
                // We've just crossed the boundary
                m_Yaw = RightYawBound;
            }
        }

        MinAngleDiff = x_MinAngleDiff(m_Pitch, OldPitch);
        // Upper
        if ((MinAngleDiff < 0.0f) // rotating up
            && (pObj = objectManager->GetObjectByGuid(m_Turret.UpperBoundaryGuid))) {
            const Vector3 Pos(pObj->GetPosition());
            const Vector3 ToPos(Pos - EyePos);
            const float   UpperPitchBound = ToPos.GetPitch();

            if ((x_MinAngleDiff(m_Pitch, UpperPitchBound) < 0.0f)       // Pitch is above the bound
                && (x_MinAngleDiff(UpperPitchBound, OldPitch) <= 0.0f)) // OldPitch is below the bound
            {
                // We've just crossed the boundary
                m_Pitch = UpperPitchBound;
            }
        }

        // Lower
        if ((MinAngleDiff > 0.0f) // rotating down
            && (pObj = objectManager->GetObjectByGuid(m_Turret.LowerBoundaryGuid))) {
            const Vector3 Pos(pObj->GetPosition());
            const Vector3 ToPos(Pos - EyePos);
            const float   LowerPitchBound = ToPos.GetPitch();

            if ((x_MinAngleDiff(m_Pitch, LowerPitchBound) > 0.0f)       // Pitch is below the bound
                && (x_MinAngleDiff(LowerPitchBound, OldPitch) >= 0.0f)) // OldPitch is above the bound
            {
                // We've just crossed the boundary
                m_Pitch = LowerPitchBound;
            }
        }
    }

    /* IJB
        if( g_IngamePad[m_ActivePlayerPad].GetLogical( ingame_pad::LOOK_VERTICAL   ).IsValue )
        {
            // do stuff?
        }
        else
        {

            player_profile& p = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));

            // if lookspring is on, recenter vertically over time
            if( p.m_bLookspringOn )
            {
                float SpringSpeed = LookSpring_ReturnSpeedTweak.GetF32();
                m_Pitch -= (m_Pitch * SpringSpeed * rDeltaTime);
                m_Pitch  = std::min( m_PitchMax, std::max( m_PitchMin, m_Pitch ) );
            }
        }
    */

    UpdateCameraShake(rDeltaTime);
}

//===========================================================================
void player::CalculateRigOffset(float DeltaTime)
{
    CalculateStrafeRigOffset(DeltaTime);
    CalculateMoveRigOffset(DeltaTime);

    // Figure out where to put the rig in relation to the camera.
    Vector3 vDesiredOffsetStrafe(m_fCurrentStrafeRigOffset, 0.0f, 0.0f);
    Vector3 vDesiredOffsetMove(0.0f, 0.0f, m_fCurrentMoveRigOffset);

    vDesiredOffsetStrafe.RotateY(m_EyesYaw);
    vDesiredOffsetMove.RotateY(m_EyesYaw);

    m_vRigOffset = vDesiredOffsetStrafe + vDesiredOffsetMove;
}

//===========================================================================

void player::CalculateStrafeRigOffset(float DeltaTime)
{
    bool  bMovingLeft = false;
    bool  bMovingRight = false;
    float PreviousStrafeOffset = m_fCurrentStrafeRigOffset;

    // The desired offsets for where the controller is currently placed.
    float DesiredStrafeOffset = m_fRigMaxStrafeOffset * m_fStrafeValue;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon && pWeapon->IsZoomEnabled()) {
        DesiredStrafeOffset = 0.0f;
    }

    // Update the offsets.
    if (DesiredStrafeOffset < m_fCurrentStrafeRigOffset) {
        m_fCurrentStrafeRigOffset -= (m_fRigStrafeOffsetVelocity * DeltaTime);
        bMovingLeft = true;
    } else if (DesiredStrafeOffset > m_fCurrentStrafeRigOffset) {
        m_fCurrentStrafeRigOffset += (m_fRigStrafeOffsetVelocity * DeltaTime);
        bMovingRight = true;
    }

    if (bMovingLeft) {
        m_fCurrentStrafeRigOffset = std::max(m_fCurrentStrafeRigOffset, DesiredStrafeOffset);
    }
    if (bMovingRight) {
        m_fCurrentStrafeRigOffset = std::min(m_fCurrentStrafeRigOffset, DesiredStrafeOffset);
    }

    if (DesiredStrafeOffset == 0.0f) {
        if (PreviousStrafeOffset <= 0.0f && m_fCurrentStrafeRigOffset >= 0.0f) {
            m_fCurrentStrafeRigOffset = 0.0f;
        }
        if (PreviousStrafeOffset >= 0.0f && m_fCurrentStrafeRigOffset <= 0.0f) {
            m_fCurrentStrafeRigOffset = 0.0f;
        }
    }
}

//===========================================================================

void player::CalculateMoveRigOffset(float DeltaTime)
{
    bool  bMovingForward = false;
    bool  bMovingBackward = false;
    float PreviousMoveOffset = m_fCurrentMoveRigOffset;

    // The desired offsets for where the controller is currently placed.
    float DesiredMoveOffset = m_fRigMaxMoveOffset * m_fMoveValue;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon && pWeapon->IsZoomEnabled()) {
        DesiredMoveOffset = 0.0f;
    }

    // Update the offsets.
    if (DesiredMoveOffset < m_fCurrentMoveRigOffset) {
        m_fCurrentMoveRigOffset -= (m_fRigMoveOffsetVelocity * DeltaTime);
        bMovingForward = true;
    } else if (DesiredMoveOffset > m_fCurrentMoveRigOffset) {
        m_fCurrentMoveRigOffset += (m_fRigMoveOffsetVelocity * DeltaTime);
        bMovingBackward = true;
    }

    if (bMovingForward) {
        m_fCurrentMoveRigOffset = std::max(m_fCurrentMoveRigOffset, DesiredMoveOffset);
    } else if (bMovingBackward) {
        m_fCurrentMoveRigOffset = std::min(m_fCurrentMoveRigOffset, DesiredMoveOffset);
    }

    if (DesiredMoveOffset == 0.0f) {
        if (PreviousMoveOffset <= 0.0f && m_fCurrentMoveRigOffset >= 0.0f) {
            m_fCurrentMoveRigOffset = 0.0f;
        }
    }
}

//===========================================================================
// YAW
void player::CalculateLookHorozOffset(float DeltaTime)
{
    bool  bLookingLeft = false;
    bool  bLookingRight = false;
    float PreviousLookOffset = m_CurrentHorozRigOffset;

    // The desired offsets for where the controller is currently placed.
    float DesiredHorozOffset = m_RigLookMaxHorozOffset * m_fYawValue;

    // Update the offsets.
    if (DesiredHorozOffset < m_CurrentHorozRigOffset) {
        m_CurrentHorozRigOffset -= (m_RigLookHorozVelocity * DeltaTime);
        bLookingLeft = true;
    } else if (DesiredHorozOffset > m_CurrentHorozRigOffset) {
        m_CurrentHorozRigOffset += (m_RigLookHorozVelocity * DeltaTime);
        bLookingRight = true;
    }

    if (bLookingLeft) {
        m_CurrentHorozRigOffset = std::max(m_CurrentHorozRigOffset, DesiredHorozOffset);
    } else if (bLookingRight) {
        m_CurrentHorozRigOffset = std::min(m_CurrentHorozRigOffset, DesiredHorozOffset);
    }

    if (DesiredHorozOffset == 0.0f) {
        if (PreviousLookOffset <= 0.0f && m_CurrentHorozRigOffset >= 0.0f) {
            m_CurrentHorozRigOffset = 0.0f;
        }
    }
}

//===========================================================================
// PITCH
void player::CalculateLookVertOffset(float DeltaTime)
{
    bool  bLookingUp = false;
    bool  bLookingDown = false;
    float PreviousLookOffset = m_CurrentVertRigOffset;

    // The desired offsets for where the controller is currently placed.
    float DesiredVertOffset = m_RigLookMaxVertOffset * m_fPitchValue;

    // Update the offsets.
    if (DesiredVertOffset < m_CurrentVertRigOffset) {
        m_CurrentVertRigOffset -= (m_RigLookVertVelocity * DeltaTime);
        bLookingUp = true;
    } else if (DesiredVertOffset > m_CurrentVertRigOffset) {
        m_CurrentVertRigOffset += (m_RigLookVertVelocity * DeltaTime);
        bLookingDown = true;
    }

    if (bLookingUp) {
        m_CurrentVertRigOffset = std::max(m_CurrentVertRigOffset, DesiredVertOffset);
    } else if (bLookingDown) {
        m_CurrentVertRigOffset = std::min(m_CurrentVertRigOffset, DesiredVertOffset);
    }

    if (DesiredVertOffset == 0.0f) {
        if (PreviousLookOffset <= 0.0f && m_CurrentVertRigOffset >= 0.0f) {
            m_CurrentVertRigOffset = 0.0f;
        }
    }
}

//===========================================================================

void player::CalculateRotationAccelerationFactors(float DeltaTime)
{
    float AccelerationTime = m_YawAccelTime;

    // Update the yaw acceleration factor
    if (abs(m_fYawValue) >= FLT_MIN) {
        m_YawAccelFactor += (DeltaTime / AccelerationTime);
        m_YawAccelFactor = std::min(1.0f, m_YawAccelFactor);
    } else {
        m_YawAccelFactor = 0.0f;
    }
    // Update the pitch acceleration factor
    if (abs(m_fPitchValue) >= FLT_MIN) {
        m_PitchAccelFactor += (DeltaTime / m_PitchAccelTime);
        m_PitchAccelFactor = std::min(1.0f, m_PitchAccelFactor);
    } else {
        m_PitchAccelFactor = 0.0f;
    }
}

//===========================================================================
// The stick position determines the maximum turn rate
// The stick position also determines the turn acceleration

void player::UpdateRotationRates(float DeltaTime)
{
    (void)DeltaTime;
}

void player::CalculatePitchLimits(const float& rDeltaTime)
{
    m_PitchMin += 2.f;
    m_PitchMax += 2.f;
    m_DesiredPitchMin += 2.f;
    m_DesiredPitchMax += 2.f;

    //Handle changes to m_DesiredPitchMin
    if (m_PitchMin < m_DesiredPitchMin) {
        m_PitchMin = std::min(m_PitchMin + m_fPitchChangeSpeed * rDeltaTime, m_DesiredPitchMin);
    } else if (m_PitchMin > m_DesiredPitchMin) {
        m_PitchMin = std::max(m_PitchMin - m_fPitchChangeSpeed * rDeltaTime, m_DesiredPitchMin);
    }

    //Handle changes to m_DesiredPitchMax
    if (m_PitchMax < m_DesiredPitchMax) {
        m_PitchMax = std::min(m_PitchMax + m_fPitchChangeSpeed * rDeltaTime, m_DesiredPitchMax);
    } else if (m_PitchMax > m_DesiredPitchMax) {
        m_PitchMax = std::max(m_PitchMax - m_fPitchChangeSpeed * rDeltaTime, m_DesiredPitchMax);
    }

    m_PitchMin -= 2.f;
    m_PitchMax -= 2.f;

    m_DesiredPitchMin -= 2.f;
    m_DesiredPitchMax -= 2.f;
}

//=========================================================================

// SB:
// Scales velocity and speed with respect to plane normal:
// NOTE: Velocity and speed are handled separately because Velocity has DeltaTime
// baked into it and speed does not - don't ask me why...
static void ScaleVelocityComponent(const Vector3& PlaneNormal,
                                   float          PerpScale,
                                   float          ParaScale,
                                   Vector3&       Velocity,
                                   float&         Speed)
{
    // Compute current speed squared, and exit if not moving
    float PrevSpeedSqr = Velocity.LengthSquared();
    if (PrevSpeedSqr < 0.0001f) {
        return;
    }

    // Compute components into and along collision plane
    Vector3 PerpVel = PlaneNormal * v3_Dot(PlaneNormal, Velocity);
    Vector3 ParaVel = Velocity - PerpVel;

    // Compute new velocity, taking scaling into account
    Velocity = (PerpScale * PerpVel) + (ParaScale * ParaVel);

    // Scale speed in proportion with velocity change
    float CurrSpeedSqr = Velocity.LengthSquared();
    if (CurrSpeedSqr >= 0.0001f) {
        float PrevSpeed = sqrt(PrevSpeedSqr);
        float CurrSpeed = sqrt(CurrSpeedSqr);
        Speed *= CurrSpeed / PrevSpeed;
    } else {
        Speed = 0.0f;
    }
}

//=========================================================================

void player::ScaleVelocity(const Vector3& PlaneNormal, float PerpScale, float ParaScale)
{
    // Apply to forward and side components
    ScaleVelocityComponent(PlaneNormal, PerpScale, ParaScale, m_ForwardVelocity, m_fForwardSpeed);
    ScaleVelocityComponent(PlaneNormal, PerpScale, ParaScale, m_StrafeVelocity, m_fStrafeSpeed);
}

//==============================================================================

void player::CalculateForwardVelocity(const Vector3& rViewZ, const float& rDeltaTime)
{
    //maximum velocity for current controller input
    float fCurMaxForwardSpeed = m_MaxFowardVelocity * m_fMoveValue;
    m_fPrevForwardSpeed = m_fForwardSpeed;

    //trying not to move or just switched directions
    if (fCurMaxForwardSpeed == 0.0f || ReversingMoveDirection(fCurMaxForwardSpeed)) {
        if (m_fForwardSpeed > 0.0f) {
            m_fForwardSpeed = std::max(0.0f, -m_fForwardAccel * m_fDecelerationFactor * rDeltaTime + m_fForwardSpeed);
        }

        else if (m_fForwardSpeed < 0.0f) {
            m_fForwardSpeed = std::min(0.0f, m_fDecelerationFactor * m_fForwardAccel * rDeltaTime + m_fForwardSpeed);
        }
    }

    else if (fCurMaxForwardSpeed > 0.0f) {
        if (fCurMaxForwardSpeed >= m_fForwardSpeed) {
            m_fForwardSpeed = std::min(fCurMaxForwardSpeed, m_fForwardAccel * rDeltaTime + m_fForwardSpeed);
        } else {
            m_fForwardSpeed = std::max(fCurMaxForwardSpeed, -m_fForwardAccel * m_fDecelerationFactor * rDeltaTime + m_fForwardSpeed);
        }

    }

    else {
        if (fCurMaxForwardSpeed <= m_fForwardSpeed) {
            m_fForwardSpeed = std::max(fCurMaxForwardSpeed, -m_fForwardAccel * rDeltaTime + m_fForwardSpeed);
        } else {
            m_fForwardSpeed = std::min(fCurMaxForwardSpeed, m_fForwardAccel * m_fDecelerationFactor * rDeltaTime + m_fForwardSpeed);
        }
    }

    m_ForwardVelocity = rViewZ * (rDeltaTime * m_fForwardSpeed);
/* IJB
    if (g_MPTweaks.Active) {
        if (IsMutated()) {
            m_ForwardVelocity *= MP_RunSpeedFactor_MutantTweak.GetF32();
            m_ForwardVelocity *= MutantSpeedFactor;
        } else {
            m_ForwardVelocity *= MP_RunSpeedFactor_NormalTweak.GetF32();
            m_ForwardVelocity *= HumanSpeedFactor;
        }
    }
*/
    m_ForwardVelocity *= g_PerceptionMgr.GetForwardSpeedFactor();
}

//==============================================================================

bool player::ReversingMoveDirection(const float& fMaxForward)
{
    return ((m_fForwardSpeed > 0.0f && fMaxForward < 0.0f) || (m_fForwardSpeed < 0.0f && fMaxForward > 0.0f));
}

//==============================================================================

bool player::HasSpeedReversed()
{
    return ((m_fPrevForwardSpeed >= 0.0f && m_fForwardSpeed < 0.0f) || (m_fPrevForwardSpeed <= 0.0f && m_fForwardSpeed > 0.0f));
}

//==============================================================================

void player::CalculateStrafeVelocity(const Vector3& rViewX, const float& rDeltaTime)
{
    //maximum velocity for current controller input
    float fCurMaxStrafeSpeed = m_MaxStrafeVelocity * m_fStrafeValue;

    //trying not to move or just switched directions
    if (fCurMaxStrafeSpeed == 0.0f || ReversingStrafeDirection(fCurMaxStrafeSpeed)) {
        if (m_fStrafeSpeed > 0.0f) {
            m_fStrafeSpeed = std::max(0.0f, -m_fStrafeAccel * m_fDecelerationFactor * rDeltaTime + m_fStrafeSpeed);
        }

        else if (m_fStrafeSpeed < 0.0f) {
            m_fStrafeSpeed = std::min(0.0f, m_fDecelerationFactor * m_fStrafeAccel * rDeltaTime + m_fStrafeSpeed);
        }
    }

    //trying to move right
    else if (fCurMaxStrafeSpeed < 0.0f) {
        //speeding up - growing negative
        if (fCurMaxStrafeSpeed <= m_fStrafeSpeed) {
            m_fStrafeSpeed = std::max(fCurMaxStrafeSpeed, -m_fStrafeAccel * rDeltaTime + m_fStrafeSpeed);
        }
        //slowing down - growing positive
        else {
            m_fStrafeSpeed = std::min(fCurMaxStrafeSpeed, m_fDecelerationFactor * m_fStrafeAccel * rDeltaTime + m_fStrafeSpeed);
        }
    }

    //trying to move left
    else {
        //speeding up - growing positive
        if (fCurMaxStrafeSpeed >= m_fStrafeSpeed) {
            m_fStrafeSpeed = std::min(fCurMaxStrafeSpeed, m_fStrafeAccel * rDeltaTime + m_fStrafeSpeed);
        }
        //slowing down - growing negative :
        else {
            m_fStrafeSpeed = std::max(fCurMaxStrafeSpeed, -m_fStrafeAccel * m_fDecelerationFactor * rDeltaTime + m_fStrafeSpeed);
        }
    }

    m_StrafeVelocity = rViewX * (rDeltaTime * m_fStrafeSpeed);
/* IJB
    if (g_MPTweaks.Active) {
        if (IsMutated()) {
            m_StrafeVelocity *= MP_StrafeSpeedFactor_MutantTweak.GetF32();
            m_StrafeVelocity *= MutantSpeedFactor;
        } else {
            m_StrafeVelocity *= MP_StrafeSpeedFactor_NormalTweak.GetF32();
            m_StrafeVelocity *= HumanSpeedFactor;
        }
    }
        */
}

//==============================================================================

bool player::ReversingStrafeDirection(const float& fMaxStrafe)
{
    return ((m_fStrafeSpeed > 0.0f && fMaxStrafe < 0.0f) || (m_fStrafeSpeed < 0.0f && fMaxStrafe > 0.0f));
}

//==============================================================================

float player::GetSpeed()
{
    return m_fStrafeSpeed + m_fForwardSpeed;
}

float player::GetCurrentVelocity()
{
    //return highest of these two
    return std::max(abs(m_fStrafeSpeed), abs(m_fForwardSpeed));
}

float player::GetMaxVelocity()
{
    //return highest of these two
    return std::max(m_MaxFowardVelocity, m_MaxStrafeVelocity);
}

// Returns guid of ladder if player is intersecting a ladder
guid player::IsInLadderField()
{
    // Lookup character physics
    const character_physics& Physics = m_Physics;

    // Search for being inside a ladder
    objectManager->SelectBBox(Object::ATTR_COLLIDABLE, GetBBox(), Object::TYPE_LADDER_FIELD);
    slot_id SlotID = objectManager->StartLoop();
    while (SlotID != SLOT_NULL) {
        // Lookup object and quit loop
        ladder_field* pLadder = (ladder_field*)objectManager->GetObjectBySlot(SlotID);
        assert(pLadder);
/* IJB
        // Overlapping this ladder?
        if ((pLadder->GetGuid() != m_JumpedOffLadderGuid)            // did we just jump off this ladder?
            && pLadder->DoesCylinderIntersect(Physics.GetPosition(), // are we in the field?
                                              Physics.GetColHeight(),
                                              Physics.GetColRadius())) {
            objectManager->EndLoop();
            return pLadder->GetGuid();
        }
*/
        // Check next object
        SlotID = objectManager->GetNextResult(SlotID);
    }
    objectManager->EndLoop();

    // No ladder found
    return 0;
}

//===========================================================================

// Ladder tweakables
static float LADDER_CLIMB_SPEED = 300.0f;  // Vertical speed
static float LADDER_STRAFE_SPEED = 100.0f; // Horizontal speed
static float LADDER_FLIP_UP_ANGLE = 25.0f; // Angle at which to swap up/down when facing ladder
static float LADDER_AT_TOP_OFFSET = 10.0f; // Dismount distance from top
static float LADDER_AT_BOT_OFFSET = 10.0f; // Dismount distance from bottom
static float LADDER_JUMP_OFF_VEL = 150.0f; // Push away vel when jumping
//static float LADDER_MOVING_AWAY_VEL   = 300.0f ;   // Vel threshold for detecting moving away from ladder

bool player::UpdateLadderMovement(float DeltaTime)
{
/* IJB
    // Lookup physics to use
    character_physics& Physics = m_Physics;

    // Default to not being on a ladder
    m_bOnLadder = false;
    m_LadderOutDir.Zero();
    Physics.SetUseGravity(true);

    // On a ladder?
    guid LadderGuid = IsInLadderField();
    if (!LadderGuid) {
        return false;
    }

    // Get ladder object
    const ladder_field* pLadder = (ladder_field*)objectManager->GetObjectByGuid(LadderGuid);
    assert(pLadder);

    // Lookup ladder object info
    const Matrix4& LadderL2W = pLadder->GetL2W();

    // Setup local direction vectors
    Vector3 Out(0, 0, 1);
    Vector3 Up(0, 1, 0);
    Vector3 Side(1, 0, 0);

    // Compute ladder world direction vectors
    Vector3 LadderOut = LadderL2W.RotateVector(Out);
    Vector3 LadderUp = LadderL2W.RotateVector(Up);
    Vector3 LadderSide = LadderL2W.RotateVector(Side);

    // Compute ladder climb plane
    plane LadderOutPlane;
    LadderOutPlane.Setup(pLadder->GetPosition(), LadderOut);
    float Dist = LadderOutPlane.Distance(GetPosition());

    // Behind the ladder? (eg. when entering the ladder from the top of a ledge)
    if (Dist < 0) {
        return false;
    }

    // Compute player facing forward direction
    Vector3 PlayerForward(Out);
    PlayerForward.RotateX(m_Pitch);
    PlayerForward.RotateY(m_Yaw);

    // Compute player running direction after getting off ladder
    Vector3 PlayerRun(Out * m_fMoveValue);
    PlayerRun.RotateY(m_Yaw);

    // Flip up direction if player is looking down the ladder
    bool bLookingDownLadder = (LadderUp.Dot(PlayerForward) < x_cos(DEG_TO_RAD(90.0f + LADDER_FLIP_UP_ANGLE)));
    if (bLookingDownLadder) {
        LadderUp = -LadderUp;
    }

    // Flip side direction if player is looking out from ladder
    if (LadderOut.Dot(PlayerForward) < 0) {
        LadderSide = -LadderSide;
    }

    // Compute up/down/side movement from input
    Vector3 UpDownVel = LadderUp * m_fMoveValue * DeltaTime * LADDER_CLIMB_SPEED;
    Vector3 SideVel = LadderSide * m_fStrafeValue * DeltaTime * LADDER_STRAFE_SPEED;

    // Get ladder and feet info
    float Top = pLadder->GetTop();
    float Bottom = pLadder->GetBottom();
    float Feet = GetPosition().GetY();

    // If moving down and past bottom of ladder, push the player off ready for dismount
    if ((UpDownVel.GetY() < 0) && (Feet < (Bottom + LADDER_AT_BOT_OFFSET))) {
        SideVel += PlayerRun * DeltaTime * LADDER_CLIMB_SPEED;
    }

    // Running into the ladder?
    if (LadderOut.Dot(PlayerRun) < 0) {
        // If near top and moving up, push the player off ready dismount
        if ((UpDownVel.GetY() > 0) && (Feet > (Top - LADDER_AT_TOP_OFFSET))) {
            SideVel += PlayerRun * DeltaTime * LADDER_CLIMB_SPEED;
        }
    } else {
        // If pulling away from the ladder, pulling back on the stick, and looking down, then let go!
        if ((PlayerRun.Dot(LadderOut) > 0) && (m_fMoveValue < 0) && (bLookingDownLadder)) {
            return false;
        }

        // If the player is NOT facing the ladder and moving up, then stop from going off the top
        if ((UpDownVel.GetY() > 0) && (Feet > (Top - LADDER_AT_TOP_OFFSET))) {
            UpDownVel.Zero();
        }
    }

    // Compute final vel
    Vector3 FinalVel = UpDownVel + SideVel;

    // Clear velocity so player doesn't shoot up/down if no input
    Physics.ZeroVelocity();

    // Turn off gravity
    Physics.SetUseGravity(false);

    // Update
    Physics.Advance(Physics.GetPosition() + FinalVel, DeltaTime);
    OnMove(Physics.GetPosition());

    // Record player is on a ladder
    m_bOnLadder = true;
    m_Physics.SetGroundTracking(false);
    m_LastLadderGuid = LadderGuid;
    m_LadderOutDir = LadderOut;
*/
    return true;
}

void player::Jump()
{
    // Lookup physics
    character_physics& Physics = m_Physics;

    // Make sure we're not able to jump as high when ascending a hill
    float VerticalVelocity = Physics.GetVelocity().GetY();
    float JumpVelocity = m_JumpVelocity;
/* IJB
    if (g_MPTweaks.Active) {
        VerticalVelocity = 0.0f;
        JumpVelocity *= g_MPTweaks.JumpSpeed;
    }
*/
    if (VerticalVelocity > 0.0f) {
        JumpVelocity -= VerticalVelocity;
        JumpVelocity = std::max(0.0f, JumpVelocity);
    }

    // Jump off a ladder?
    if (m_bOnLadder) {
        m_bOnLadder = false;
        m_JumpedOffLadderGuid = m_LastLadderGuid;
        Vector3 Vel = Physics.GetVelocity();
        Vel += m_LadderOutDir * LADDER_JUMP_OFF_VEL;
        Physics.SetVelocity(Vel);
    } else {
        // Jump vertically
        Physics.Jump(JumpVelocity);
    }
}

//===========================================================================

void player::HitJumpPad(const Vector3& Velocity,
                        float          DeltaTime,
                        float          AirControl,
                        bool           BoostOnly,
                        bool           ReboostOnly,
                        bool           Instantaneous,
                        guid           JumpPadGuid)
{
    m_Physics.Fling(Velocity, DeltaTime, AirControl,
                    BoostOnly, ReboostOnly,
                    Instantaneous, JumpPadGuid);

    if (Instantaneous && !ReboostOnly) {
/* IJB
        if (!(m_WayPointFlags & WAYPOINT_TELEPORT_FX)) {
            slot_id Slot = objectManager->GetFirst(TYPE_JUMP_PAD);
            while (Slot != SLOT_NULL) {
                Object* pObject = objectManager->GetObjectBySlot(Slot);
                assert(pObject);
                assert(pObject->GetType() == TYPE_JUMP_PAD);
                Vector3 Gap = pObject->GetPosition() - GetPosition();
                if (Gap.LengthSquared() < 250.0f) {
                    jump_pad* pJumpPad = (jump_pad*)pObject;
                    pJumpPad->PlayJump();
                    break;
                }
                Slot = objectManager->GetNext(Slot);
            }

            m_NetDirtyBits |= WAYPOINT_BIT;
            m_WayPointFlags |= WAYPOINT_JUMP_PAD_FX;
            m_WayPointTimeOut = 0;
        }
*/
    }
}

void player::UpdateFellFromAltitude()
{
    // Use the current position if we have moved upwards or are not falling
    float Altitude = GetPosition().GetY();
    if ((Altitude > m_FellFromAltitude) || !m_bFalling) {
        m_FellFromAltitude = Altitude;
    }
}
