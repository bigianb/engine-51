
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

static float        LargeReticleSpeed = 10.0f;
static float        SmallReticleSpeed = 300.0f;
static float        WeaponCollisionRadius = 5.0f;
static const Radian s_AimYawOffsetRateOfChange = R_180;

#define MAX_AUTO_AIM_DISTANCE 4500.0f

//=========================================================================
// EXTERNALS
//=========================================================================

extern int         g_Difficulty;
extern const char* DifficultyText[];

// tweak values
extern float AimAssist_LOF_Dist;

extern float AimAssist_Reticle_Near_Dist;
extern float AimAssist_Reticle_Far_Dist;
extern float AimAssist_Reticle_Near_Radius;
extern float AimAssist_Reticle_Far_Radius;

extern float AimAssist_Bullet_Inner_Near_Dist;
extern float AimAssist_Bullet_Inner_Far_Dist;
extern float AimAssist_Bullet_Inner_Near_Radius;
extern float AimAssist_Bullet_Inner_Far_Radius;

extern float AimAssist_Bullet_Outer_Near_Dist;
extern float AimAssist_Bullet_Outer_Far_Dist;
extern float AimAssist_Bullet_Outer_Near_Radius;
extern float AimAssist_Bullet_Outer_Far_Radius;

extern float AimAssist_Bullet_Angle;

extern float AimAssist_Turn_Inner_Near_Dist;
extern float AimAssist_Turn_Inner_Far_Dist;
extern float AimAssist_Turn_Inner_Near_Radius;
extern float AimAssist_Turn_Inner_Far_Radius;

extern float AimAssist_Turn_Outer_Near_Dist;
extern float AimAssist_Turn_Outer_Far_Dist;
extern float AimAssist_Turn_Outer_Near_Radius;
extern float AimAssist_Turn_Outer_Far_Radius;

extern float AimAssist_Turn_Damp_Near_Dist;
extern float AimAssist_Turn_Damp_Far_Dist;

// Tweak handles
extern tweak_handle AimAssist_LOF_Dist_Tweak;

extern tweak_handle AimAssist_Reticle_Near_Dist_Tweak;
extern tweak_handle AimAssist_Reticle_Far_Dist_Tweak;
extern tweak_handle AimAssist_Reticle_Near_Radius_Tweak;
extern tweak_handle AimAssist_Reticle_Far_Radius_Tweak;

extern tweak_handle AimAssist_Bullet_Inner_Near_Dist_Tweak;
extern tweak_handle AimAssist_Bullet_Inner_Far_Dist_Tweak;
extern tweak_handle AimAssist_Bullet_Inner_Near_Radius_Tweak;
extern tweak_handle AimAssist_Bullet_Inner_Far_Radius_Tweak;

extern tweak_handle AimAssist_Bullet_Outer_Near_Dist_Tweak;
extern tweak_handle AimAssist_Bullet_Outer_Far_Dist_Tweak;
extern tweak_handle AimAssist_Bullet_Outer_Near_Radius_Tweak;
extern tweak_handle AimAssist_Bullet_Outer_Far_Radius_Tweak;

extern tweak_handle AimAssist_Bullet_Angle_Tweak;

extern tweak_handle AimAssist_Turn_Inner_Near_Dist_Tweak;
extern tweak_handle AimAssist_Turn_Inner_Far_Dist_Tweak;
extern tweak_handle AimAssist_Turn_Inner_Near_Radius_Tweak;
extern tweak_handle AimAssist_Turn_Inner_Far_Radius_Tweak;

extern tweak_handle AimAssist_Turn_Outer_Near_Dist_Tweak;
extern tweak_handle AimAssist_Turn_Outer_Far_Dist_Tweak;
extern tweak_handle AimAssist_Turn_Outer_Near_Radius_Tweak;
extern tweak_handle AimAssist_Turn_Outer_Far_Radius_Tweak;

extern tweak_handle AimAssist_Turn_Damp_Near_Dist_Tweak;
extern tweak_handle AimAssist_Turn_Damp_Far_Dist_Tweak;

bool g_PlayerMeleeShakeView = true;
bool g_PlayerMeleeDoFeedback = true;

////////////////////////////////////////////////////////
// KSS -- FIXME -- move these to tweak table
// Combo tweaks
float s_MeleeShakeSpeed[MAX_COMBO_HITS] = {4.0f, 4.0f, 4.0f};
float s_MeleeShakeTime[MAX_COMBO_HITS] = {0.4f, 0.4f, 0.5f};
float s_MeleeShakeAmount[MAX_COMBO_HITS] = {3.0f, 3.0f, 4.0f};
float s_MeleeFeedbackShakeAmount[MAX_COMBO_HITS] = {1.0f, 1.0f, 1.0f};
float s_MeleeFeedbackShakeTime[MAX_COMBO_HITS] = {0.2f, 0.2f, 0.25f};
// END -- KSS -- FIXME -- move these to tweak table
////////////////////////////////////////////////////////

int        GetStartWeaponsForLevel(const char* pLevelName);
inven_item GetEquipedWeaponForLevel(const char* pLevelName);

//INVEN_WEAPON_MUTATION

// KSS -- TO ADD NEW WEAPON
mtwt s_MapToWeaponTable[] =
    {
        {"dreamlnd", 1 * 1024, INVEN_NULL, WB_DE | WB_SCN, 0, true},
        {"undrgrnd", 1 * 1024, INVEN_NULL, WB_DE | WB_SCN, 0, true},
        {"hotzone", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_SCN, WB_DE | WB_SMP | WB_SCN, true},
        {"search", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SCN, false},
        {"getbig", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SCN, false},
        {"laststnd", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_SCN, false},
        {"oneofthm", 1 * 1024, INVEN_WEAPON_SMP, WB_MM | WB_SMP | WB_DE | WB_SCN, WB_MM | WB_SCN, false},
        {"mutation", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_MM | WB_SG | WB_MP | WB_SCN, WB_DE | WB_SMP | WB_MM | WB_SCN, false},
        {"sidetrck", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_SG | WB_MM | WB_MP | WB_SCN, WB_DE | WB_SMP | WB_SG | WB_MM | WB_MP | WB_SCN, false},
        {"dr_cray", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_SCN, false},
        {"illumin", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_SCN | WB_MS, false},
        {"flynobjs", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_SCN | WB_MS, false},
        {"liespast", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_SCN | WB_JBG, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_SCN | WB_JBG | WB_MS, false},
        {"caves", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_SCN | WB_MS, false},
        {"boarding", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_SCN | WB_MS, false},
        {"ascend1", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_MC | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_SCN | WB_MS, false},
        {"ascend2", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_MC | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_MC | WB_SCN | WB_MS, false},
        {"ascend3", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_MC | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_MC | WB_SCN | WB_MS, false},
        {"exit", 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_MC | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_MC | WB_SCN | WB_MS, false},
        {nullptr, 1 * 1024, INVEN_WEAPON_SMP, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_MS | WB_BBG | WB_MC | WB_SCN, WB_DE | WB_SMP | WB_FG | WB_SG | WB_MM | WB_SR | WB_MP | WB_BBG | WB_MC | WB_SCN | WB_MS, false},
};

bool player::IsAltFiring(void)
{
    return m_bIsMutated
               ? (bool)(g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_FIRE_CONTAGION).IsValue)
               : (bool)(g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_SECONDARY).IsValue);
}

//==============================================================================
bool player::IsFiring(void)
{
    bool PrimaryDown = m_bIsMutated
                           ? (bool)g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_FIRE_PARASITES).IsValue
                           : (bool)g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_PRIMARY).IsValue;

    // IJB PrimaryDown |= input_IsPressed( INPUT_MOUSE_BTN_L );

    return (PrimaryDown && !m_bRespawnButtonPressed);
}
//==============================================================================

void player::GetProjectileHitLocation(Vector3& EndPos, bool bUseBulletAssist)
{
    Radian Pitch;
    Radian Yaw;

    // the view's rotation
    view& View = GetView();

    if (bUseBulletAssist) {
        m_AimAssistData.BulletAssistDir.GetPitchYaw(Pitch, Yaw);
    } else {
        GetEyesPitchYaw(Pitch, Yaw);
    }

    Vector3       Dest(Radian3(Pitch, Yaw, 0.0f));
    const Vector3 ViewPos = View.GetPosition();
    Dest *= MAX_AUTO_AIM_DISTANCE;
    EndPos = ViewPos + Dest;
    g_CollisionMgr.AddToIgnoreList(GetGuid());
    g_CollisionMgr.RaySetup(GetGuid(), ViewPos, EndPos);
    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_SMALL_PROJECTILES, Object::ATTR_COLLISION_PERMEABLE);

    // default modifier to full distance in case the collision manager returns no collisions
    float DistModifier = 1.0f;

    // if we don't hit anything, T is undefined
    if (g_CollisionMgr.m_nCollisions > 0) {
        DistModifier = g_CollisionMgr.m_Collisions[0].T;
    }

    // get our new end position
    EndPos = ViewPos + (DistModifier * (EndPos - ViewPos));
}

Radian3 player::GetProjectileTrajectory()
{
    // the view's rotation
    Radian  Pitch;
    Radian  Yaw;
    Vector3 StartPosition;
    bool    bUseWeaponPos = false;

    new_weapon* pWeaponObj = GetCurrentWeaponPtr();

    // StartPosition will most likely be the "firepoint" of the weapon instead of coming out of your eyes.
    // This is for weapons like the Meson Cannon where you can see the projectile and the weapon.
    // GetFiringStartPosition() will return true for weapons that overload it.  Defaults to false.
    bUseWeaponPos = pWeaponObj->GetFiringStartPosition(StartPosition);

    // if bUseWeaponPos == true, for things like meson cannon and mutation weapon, fire projectile from firing point
    // Otherwise, use bullet assist direction
    if (!bUseWeaponPos) {
        m_AimAssistData.BulletAssistDir.GetPitchYaw(Pitch, Yaw);

        // apply aim degredation
        Radian3 Rot = ApplyAimDegredation(Pitch, Yaw);

        return Rot;
    }

    Vector3 EndPosition;
    GetProjectileHitLocation(EndPosition);

    // get the direction vector from the firing start position to where our trace hit
    Vector3 ToTarget(EndPosition - StartPosition);

    // reload pitch and yaw from the Target vector
    ToTarget.GetPitchYaw(Pitch, Yaw);

    // We are firing the projectile visually from the weapon.
    Radian3 ProjectileRot = ApplyAimDegredation(Pitch, Yaw);

    return ProjectileRot;
}

Radian3 player::ApplyAimDegredation(Radian Pitch, Radian Yaw)
{
    Vector3 Dir(0, 0, 1);
    Dir.RotateX(R_6 * x_frand(-m_AimDegradation, m_AimDegradation)); // Pitch Z-axis up or down by spread angle
    Dir.RotateZ(x_frand(0, R_360));                                  // Roll dir around Z
    Dir.RotateX(Pitch);                                              // Orient around original direction
    Dir.RotateY(Yaw);

    float P, Y;
    Dir.GetPitchYaw(P, Y);

    //x_DebugMsg("%f %f %f %f %f\n",m_AimDegradation,Pitch,Yaw,P,Y);

    return Radian3(P, Y, 0);
}

//==============================================================================

guid player::GetEnemyOnReticle(void)
{
    if (ReticleOnTarget()) {
        return m_AimAssistData.TargetGuid;
    } else {
        return 0;
    }
}

//===========================================================================
guid player::GetFriendlyOnReticle(void)
{
    if (ReticleOnTarget()) {
        return m_AimAssistData.OnlineFriendlyTargetGuid;
    } else {
        return 0;
    }
}

//===========================================================================

void player::UpdateAimOffset(float DeltaTime)
{
    // Largest value for AimOffset should be R_25.  Smallest value should be 0.
    if (m_AimAssistData.TargetGuid != 0) {
        m_YawAimOffset = std::min(R_25, m_YawAimOffset + DeltaTime * s_AimYawOffsetRateOfChange);
    } else {
        m_YawAimOffset = std::max(R_0, m_YawAimOffset - DeltaTime * s_AimYawOffsetRateOfChange);
    }
}

//===========================================================================

bool player::AddAmmo2(inven_item WeaponItem, int Amount)
{
    // not sending in anything
    if (Amount == 0) {
        return false;
    }

    if (m_Inventory2.HasItem(WeaponItem)) {
        new_weapon* pWeapon = GetWeaponPtr(WeaponItem);

        if (pWeapon) {
            int nCurrentAmmoCount = pWeapon->GetTotalPrimaryAmmo();
            int nMaxAmmoCount = pWeapon->GetMaxPrimaryAmmo();

            if (nCurrentAmmoCount < nMaxAmmoCount) {
                float NewAmount = (float)Amount;

                tweak_handle AmmoScalarTweak(xfs("AmmoAmount_%s", DifficultyText[g_Difficulty]));

                float AmmoScalar = 0.0f;

                if (AmmoScalarTweak.Exists()) {
                    AmmoScalar = AmmoScalarTweak.GetF32();
                }

                // Scale ammo based on difficulty level
                // the scalar could be +/- and is a whole percentage i.e. -20
                NewAmount = Amount + (Amount * (AmmoScalar / 100.0f));

                // make sure we don't give less than 1, that's bad
                if (NewAmount < 1.0f) {
                    NewAmount = 1.0f;
                }

                // actually put ammo into weapon
                pWeapon->AddAmmoToWeapon((int)NewAmount, 0);

                // if this weapon was empty, reload it for us
                if (nCurrentAmmoCount == 0) {
                    // if this is our current weapon, do all the animations and such
                    if (WeaponItem == m_CurrentWeaponItem) {
                        ReloadWeapon(new_weapon::AMMO_PRIMARY);
                    } else {
                        // otherwise, just auto-reload it
                        pWeapon->Reload(new_weapon::AMMO_PRIMARY);
                    }
                }

                return true;
            }
        }
    }
    return false;
}

void player::DegradeAim(float fAmountToDegradeBy)
{
    m_AimDegradation = std::min(1.0f, m_AimDegradation + fAmountToDegradeBy);
}

float MinYawModifier = 0.42f;
float AimAssistDownVelocity = 100.00f;
float AimAssistUpVelocity = 1.00f;

void player::UpdateAimAssistance(float DeltaTime)
{
    // find and set the targeted guid.
    UpdateCurrentAimTarget(DeltaTime);

    int     LastNetSlot = m_TargetNetSlot;
    Vector3 LastAimOffset = m_AimOffset;

    m_TargetNetSlot = -1;

    if (m_AimAssistData.TargetGuid != 0) {
        m_fCurrentYawAimModifier = std::max(m_fCurrentYawAimModifier - AimAssistDownVelocity * DeltaTime, MinYawModifier);
        m_fCurrentPitchAimModifier = 0.5f;

        Object* pObject = objectManager->GetObjectByGuid(m_AimAssistData.TargetGuid);

        // Update the aim offset if we're aiming at another human player.
        if (pObject) {
            if (pObject->IsKindOf(actor::GetRTTI())) {
                actor& Actor = actor::GetSafeType(*pObject);
                int    NetSlot = Actor.net_GetSlot();
                if (IN_RANGE(0, NetSlot, 31) && !Actor.IsDead()) {
                    m_TargetNetSlot = NetSlot;
                    m_AimOffset = m_AimAssistData.AimDelta;
                }
            }
        }
    } else {
        m_fCurrentYawAimModifier = std::min(m_fCurrentYawAimModifier + AimAssistUpVelocity * DeltaTime, 1.0f);
        m_fCurrentPitchAimModifier = 1.0f;
    }
}

struct targetData
{
    guid  Guid;
    float distance;
};

int DataListCompareFN(targetData* pItem1, targetData* pItem2)
{
    if (pItem1->distance < pItem2->distance) {
        return -1;
    }
    return pItem1->distance > pItem2->distance;
}

float g_AimYawConstraint = R_5;
float g_AimPitchConstraint = R_3;

static inline float ComputeInterpValue(float Dist, float NearDist, float FarDist, float NearValue, float FarValue)
{
    float T = (Dist - NearDist) / (FarDist - NearDist);
    if (T < 0) {
        T = 0;
    }
    if (T > 1) {
        T = 1;
    }
    return NearValue + T * (FarValue - NearValue);
}

#define AIM_LOGGING_ENABLED 0

float AIMASSIST_PERP_SPEED_MAX = 300;
float AIMASSIST_BULLET_ASSIST_SCALE = 2.5f;
float AIMASSIST_TURN_DAMPEN_SCALE = 0.1f;
float AIMASSIST_MULTIPLAYER_SCALE = 1.5f;

bool g_bTestFriendly = true;
void player::UpdateCurrentAimTarget(float DeltaTime)
{
    (void)DeltaTime;
    const view& View = GetView();
    Vector3     Position = GetPosition();
    bool        Final_bReticleOn = false;
    float       TargetCullDot = cos(R_20);

    m_AimAssistData.BulletAssistDir = View.GetViewZ();
    m_AimAssistData.BulletAssistBestDist = FLT_MAX;
    m_AimAssistData.TurnDampeningT = 0.0f;
    m_AimAssistData.TargetGuid = 0;
    m_AimAssistData.OnlineFriendlyTargetGuid = 0;

    if (GetCurrentWeaponPtr() == nullptr) {
        return;
    }

    if (AimAssist_LOF_Dist == 0.0f) {
        return;
    }

    // Decide if we should scale up radii based on multiplayer
    float MultiplayerRadiiScale = 1.0f;

    // Decide if bullet assist should lead
    float BulletAssistLeadSpeed = 0.0f;
    {
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        /* IJB
        if( pWeapon->IsKindOf( weapon_bbg::GetRTTI() ) )
        {
            tweak_handle SpeedTweak( xfs("%s_SPEED", pWeapon->GetLogicalName()) );
            BulletAssistLeadSpeed = SpeedTweak.GetF32();
        }
            */
    }

    // Get our velocity
    Vector3 PlayerVelocity = m_Physics.GetVelocity();
    //    x_printfxy(0,1,"LS: %7.3f",BulletAssistLeadSpeed);
    //    x_printfxy(0,2,"%7.1f %7.1f %7.1f",PlayerVelocity.GetX(),PlayerVelocity.GetY(),PlayerVelocity.GetZ());

    //
    // Find LOF Collision distance
    //
    Vector3 LOFStart;
    Vector3 LOFEnd;
    Vector3 LOFDir;
    {
        LOFDir = View.GetViewZ();
        LOFStart = View.GetPosition();
        LOFEnd = LOFStart + LOFDir * AimAssist_LOF_Dist;
        g_CollisionMgr.LineOfSightSetup(GetGuid(), LOFStart, LOFEnd);
        g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PLAYER_LOS, Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING);

        // save off collision distance
        if (g_CollisionMgr.m_nCollisions > 0) {
            m_AimAssistData.LOFCollisionDist = AimAssist_LOF_Dist * g_CollisionMgr.m_Collisions[0].T;
        }
    }

    //
    // Loop through all active players
    //
    actor* pNextActor = actor::m_pFirstActive;
    while (pNextActor) {
        // Get ptr to actor and advance to next
        actor* pActor = pNextActor;
        pNextActor = pNextActor->m_pNextActive;

        if ((pActor->GetGuid() != GetGuid()) &&
            pActor->IsKindOf(actor::GetRTTI()) &&
            !pActor->IsDead()) {
            if (IsEnemyFaction(pActor->GetFaction())) {
                //
                // Get target position info
                //
                Vector3 TargetPos = pActor->GetPosition();
                Vector3 TargetDelta = TargetPos - Position;
                float   TargetDist = TargetDelta.Length();

                // throw out this guy if he's behind us
                if (LOFDir.Dot(TargetDelta) <= 0.0f) {
                    continue;
                }

                // If target is a decent distance away check if the angle
                // to his position is far away from the LOFDir
                Vector3 TargetDeltaDir = TargetDelta;
                TargetDeltaDir.Normalize();
                if (TargetDist > 1000.0f) {
                    if (LOFDir.Dot(TargetDeltaDir) <= TargetCullDot) {
                        continue;
                    }
                }

                //
                // Get Velocity of target
                //
                Vector3 TargetVelocity(0, 0, 0);
                Vector3 RelativeVelocity(0, 0, 0);
                float   PerpSpeed = 0;
                Vector3 PerpVelocity;
                float   PerpSpeedT = 0;
                {
                    loco* pLoco = pActor->GetLocoPointer();
                    if (pLoco) {
                        TargetVelocity = pLoco->m_Physics.GetVelocity();
                    }

                    RelativeVelocity = TargetVelocity - PlayerVelocity;
                    PerpVelocity = RelativeVelocity - (LOFDir.Dot(RelativeVelocity) * LOFDir);
                    PerpSpeed = PerpVelocity.Length();
                    PerpSpeedT = PerpSpeed / AIMASSIST_PERP_SPEED_MAX;
                    if (PerpSpeedT > 1) {
                        PerpSpeedT = 1;
                    }
                }

                // Compute scale for bullet assist and turn dampening
                float BulletAssistPerpSpeedScale = 1.0f + PerpSpeedT * (AIMASSIST_BULLET_ASSIST_SCALE - 1.0f);
                float TurnDampenPerpSpeedScale = 1.0f + PerpSpeedT * (AIMASSIST_TURN_DAMPEN_SCALE - 1.0f);

                //
                // Get spine information
                //
                Vector3 SpineTop;
                Vector3 SpineBot;
                {
                    pActor->GetHeadAndRootPosition(SpineTop, SpineBot);

                    // Raise top of spine since head bone is at base of head
                    SpineTop += Vector3(0, 20.0f, 0);
                }

                //
                // Get closest pt between LOF and Spine
                //
                Vector3 SpinePt;
                Vector3 LOFPt;
                Vector3 SpineLOFOffset;
                float   LOFPtT;
                float   SpinePtT;
                float   LOFPtDist;
                {
                    x_ClosestPtsOnLineSegs(LOFStart, LOFEnd, SpineTop, SpineBot, LOFPt, SpinePt, LOFPtT, SpinePtT);
                    LOFPtDist = AimAssist_LOF_Dist * LOFPtT;
                    SpineLOFOffset = (SpinePt - LOFPt);
                    m_AimAssistData.LOFSpineDist = SpineLOFOffset.Length();

                    m_AimAssistData.SpinePt = SpinePt;
                    m_AimAssistData.SpinePtT = SpinePtT;
                    m_AimAssistData.LOFPt = LOFPt;
                    m_AimAssistData.LOFPtDist = LOFPtDist;

                    // don't let it go negative (behind us) or 0
                    m_AimAssistData.LOFPtDist = std::clamp(m_AimAssistData.LOFPtDist, 1.0f, FLT_MAX);
                }

                //
                // Check distance to LOFPt against collision
                //
                bool bLOFPtBlocked = (LOFPtDist > m_AimAssistData.LOFCollisionDist);

                //
                // Check if Reticle should be on
                //
                if (bLOFPtBlocked == false) {
                    float Radius = ComputeInterpValue(TargetDist,
                                                      AimAssist_Reticle_Near_Dist,
                                                      AimAssist_Reticle_Far_Dist,
                                                      AimAssist_Reticle_Near_Radius * BulletAssistPerpSpeedScale,
                                                      AimAssist_Reticle_Far_Radius * BulletAssistPerpSpeedScale);

                    m_AimAssistData.ReticleRadius = Radius;

                    if (m_AimAssistData.LOFSpineDist <= Radius) {
                        Final_bReticleOn = true;
                    }
                }

                //
                // Compute a new BulletAssistDir if the LOF approaches closer than the previous target
                //
                if ((bLOFPtBlocked == false) && (m_AimAssistData.LOFSpineDist < m_AimAssistData.BulletAssistBestDist)) {
                    // Compute what the new bullet assist direction would be
                    float InnerRadius = ComputeInterpValue(TargetDist,
                                                           AimAssist_Bullet_Inner_Near_Dist,
                                                           AimAssist_Bullet_Inner_Far_Dist,
                                                           AimAssist_Bullet_Inner_Near_Radius * BulletAssistPerpSpeedScale * MultiplayerRadiiScale,
                                                           AimAssist_Bullet_Inner_Far_Radius * BulletAssistPerpSpeedScale * MultiplayerRadiiScale);

                    float OuterRadius = ComputeInterpValue(TargetDist,
                                                           AimAssist_Bullet_Outer_Near_Dist,
                                                           AimAssist_Bullet_Outer_Far_Dist,
                                                           AimAssist_Bullet_Outer_Near_Radius * BulletAssistPerpSpeedScale * MultiplayerRadiiScale,
                                                           AimAssist_Bullet_Outer_Far_Radius * BulletAssistPerpSpeedScale * MultiplayerRadiiScale);

                    m_AimAssistData.BulletInnerRadius = InnerRadius;
                    m_AimAssistData.BulletOuterRadius = OuterRadius;

                    // interpolate between inner and outer radius (inverse)
                    float T = x_parametric(m_AimAssistData.LOFSpineDist, OuterRadius, InnerRadius, true);
                    if (T > 0) {
                        Radian AssistAngle = T * AimAssist_Bullet_Angle;

                        // Build AimAtPoint
                        Vector3 AimAtPoint = SpinePt;

                        // Apply lead for slow projectiles
                        if ((BulletAssistLeadSpeed > 0) && (PerpSpeed > 0.0f)) {
                            // Compute approx flight time
                            float FlightTime = TargetDist / BulletAssistLeadSpeed;
                            float LeadDist = PerpSpeed * FlightTime;
                            if (LeadDist > 100.0f) {
                                LeadDist = 100.0f;
                            }
                            AimAtPoint += PerpVelocity * (LeadDist / PerpSpeed);
                            //                            x_printfxy(0,5,"LD: %6.2f",LeadDist);
                        }

                        // Rotate LOFDir toward AimAtPoint by the angle AssistAngle
                        Vector3 ToSpine = AimAtPoint - View.GetPosition();
                        Radian  Angle = v3_AngleBetween(LOFDir, ToSpine);
                        if (Angle > AssistAngle) {
                            Angle = AssistAngle;
                        }
                        Vector3 Axis = LOFDir.Cross(ToSpine);
                        Axis.Normalize();
                        Quaternion Q(Axis, Angle);

                        //
                        // Remember aim direction
                        //
                        Vector3 NewBulletAssistDir = Q * LOFDir;

                        // Only replace best if LOF is clear.  Search for a clear LOF from full bullet
                        // assist direction back to LOF direction
                        int nSegs = 4;
                        int i;
                        for (i = 0; i < nSegs; i++) {
                            float   T = (float)i / (float)(nSegs - 1);
                            Vector3 Dir = NewBulletAssistDir + T * (LOFDir - NewBulletAssistDir);
                            Dir.Normalize();

                            Vector3 EndPos = LOFStart + Dir * m_AimAssistData.LOFPtDist;
                            g_CollisionMgr.LineOfSightSetup(GetGuid(), LOFStart, EndPos);
                            g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PLAYER_LOS, Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING);

                            if (g_CollisionMgr.m_nCollisions == 0) {
                                m_AimAssistData.BulletAssistBestDist = m_AimAssistData.LOFSpineDist;
                                m_AimAssistData.TargetGuid = pActor->GetGuid();
                                m_AimAssistData.BulletAssistDir = Dir;

                                // Get the aim delta relative to the players orientation.
                                Vector3 AbsoluteDelta = LOFPt - pActor->GetPosition();

                                m_AimAssistData.AimDelta = AbsoluteDelta.Length() *
                                                           Vector3(AbsoluteDelta.GetPitch(),
                                                                   AbsoluteDelta.GetYaw() - pActor->GetYaw());

                                break;
                            }
                        }
                    }
                }

                //
                // Compute turn dampening amount
                //
                if (bLOFPtBlocked == false) {
                    float InnerRadius = ComputeInterpValue(TargetDist,
                                                           AimAssist_Turn_Inner_Near_Dist,
                                                           AimAssist_Turn_Inner_Far_Dist,
                                                           AimAssist_Turn_Inner_Near_Radius,
                                                           AimAssist_Turn_Inner_Far_Radius);

                    float OuterRadius = ComputeInterpValue(TargetDist,
                                                           AimAssist_Turn_Outer_Near_Dist,
                                                           AimAssist_Turn_Outer_Far_Dist,
                                                           AimAssist_Turn_Outer_Near_Radius,
                                                           AimAssist_Turn_Outer_Far_Radius);

                    m_AimAssistData.TurnInnerRadius = InnerRadius;
                    m_AimAssistData.TurnOuterRadius = OuterRadius;

                    // interpolate between inner and outer radius (inverse)
                    float T = x_parametric(m_AimAssistData.LOFSpineDist, OuterRadius, InnerRadius, true);

                    T *= TurnDampenPerpSpeedScale;

                    // Keep the largest amount
                    if (T > m_AimAssistData.TurnDampeningT) {
                        m_AimAssistData.TurnDampeningT = T;
                    }
                }
            } else ////////////////////////////////////////// FRIENDLY TARGET //////////////////////////////////////////
                if (g_bTestFriendly) {
                    //
                    // Get target position info
                    //
                    Vector3 TargetPos = pActor->GetPosition();
                    Vector3 TargetDelta = TargetPos - Position;
                    float   TargetDist = TargetDelta.Length();

                    // throw out this guy if he's behind us
                    if (LOFDir.Dot(TargetDelta) <= 0.0f) {
                        continue;
                    }

                    // If target is a decent distance away check if the angle
                    // to his position is far away from the LOFDir
                    Vector3 TargetDeltaDir = TargetDelta;
                    TargetDeltaDir.Normalize();
                    if (TargetDist > 1000.0f) {
                        if (LOFDir.Dot(TargetDeltaDir) <= TargetCullDot) {
                            continue;
                        }
                    }

                    //
                    // Get spine information
                    //
                    Vector3 SpineTop;
                    Vector3 SpineBot;
                    {
                        SpineBot = pActor->GetPosition();
                        SpineTop = SpineBot + Vector3(0, 150, 0);
                        /* Commented out for performance around friendlies - AndyT
                        ((character*)pActor)->GetHeadAndRootPosition( SpineTop, SpineBot );
                        // Raise top of spine since head bone is at base of head
                        SpineTop += Vector3(0,20.0f,0);
                        */
                    }

                    //
                    // Get closest pt between LOF and Spine
                    //
                    Vector3 SpinePt;
                    Vector3 LOFPt;
                    Vector3 SpineLOFOffset;
                    float   LOFPtT;
                    float   SpinePtT;
                    float   LOFPtDist;
                    float   Dist;
                    {
                        x_ClosestPtsOnLineSegs(LOFStart, LOFEnd, SpineTop, SpineBot, LOFPt, SpinePt, LOFPtT, SpinePtT);
                        LOFPtDist = AimAssist_LOF_Dist * LOFPtT;
                        SpineLOFOffset = (SpinePt - LOFPt);
                    }

                    //
                    // Check distance to LOFPt against collision
                    //
                    bool bLOFPtBlocked = (LOFPtDist > m_AimAssistData.LOFCollisionDist);

                    Dist = SpineLOFOffset.Length();

                    //
                    // Check if Reticle should be on
                    //
                    if (bLOFPtBlocked == false) {
                        float Radius = ComputeInterpValue(TargetDist,
                                                          AimAssist_Reticle_Near_Dist,
                                                          AimAssist_Reticle_Far_Dist,
                                                          AimAssist_Reticle_Near_Radius,
                                                          AimAssist_Reticle_Far_Radius);

                        // see if we're inside pill
                        if (Dist <= Radius) {
                            Final_bReticleOn = true;
                            m_AimAssistData.OnlineFriendlyTargetGuid = pActor->GetGuid();
                            break; // break out of while loop
                        }
                    }
                }
        }
    }

    //
    // Deaden the aim stick
    //
    if (m_AimAssistData.TurnDampeningT > 0) {
        // kill aim assist stick dampening at AimAssist_Turn_Damp_Near_Dist
        float DampPct = x_parametric(m_AimAssistData.LOFPtDist, AimAssist_Turn_Damp_Near_Dist, AimAssist_Turn_Damp_Far_Dist, true);

        // scale turn dampening
        m_AimAssistData.TurnDampeningT = m_AimAssistData.TurnDampeningT * DampPct;

        tweak_handle StickDampTweak("TurnDampeningT"); // 0=no turning, 1=normal turning
        float        DampMax = StickDampTweak.GetF32();
        float        StickyMult = 1.0f + (DampMax - 1.0f) * m_AimAssistData.TurnDampeningT;
        m_fYawValue *= StickyMult;
        m_fPitchValue *= StickyMult;

        //CLOG_MESSAGE( AIM_LOGGING_ENABLED, "player::UpdateCurrentAimTarget", "T:: %f", m_AimAssistData.TurnDampeningT );

    } else {
        //CLOG_MESSAGE( AIM_LOGGING_ENABLED, "player::UpdateCurrentAimTarget *CHECK*", "T: %f", m_AimAssistData.TurnDampeningT );

        // Be sure if we aren't doing aim assist for enemies that we kill turn dampening
        m_AimAssistData.TurnDampeningT = 0.0f;
    }

    //
    // Remember reticle is on the target
    //
    {
        m_AimAssistData.bReticleOn = Final_bReticleOn;
    }

    UpdateReticleRadius(DeltaTime);
}

Radian player::CalculateNecessaryAimAssistYaw(Object* pObject)
{
    // Get the vector from the view to the center of the object's bbox.
    Vector3 vViewToBoxCenter = pObject->GetColBBox().GetCenter() - m_EyesPosition;

    // Calculate the length and width that I'm going to need.
    // We may need to optimize this, because it's expensive.
    // ALSO: need to check vs. width here, not just radius.
    float fToCenterLength = vViewToBoxCenter.Length();
    //    float fBoxWidth = pObject->GetColBBox().GetRadius();
    float fBoxWidth = pObject->GetColBBox().max.x - pObject->GetColBBox().min.x;

    Radian HalfAngle = atan(fBoxWidth / fToCenterLength);

    return HalfAngle;
}

Radian player::CalculateNecessaryAimAssistPitch(Object* pObject)
{
    // Get the vector from the view to the center of the object's bbox.
    Vector3 vViewToBoxCenter = pObject->GetColBBox().GetCenter() - m_EyesPosition;

    // Calculate the length and width that I'm going to need.
    // We may need to optimize this, because it's expensive.
    // ALSO: need to check vs. width here, not just radius.
    float fToCenterLength = vViewToBoxCenter.Length();
    float fBoxHeight = pObject->GetColBBox().max.y - pObject->GetColBBox().min.y;

    Radian HalfAngle = atan(fBoxHeight / fToCenterLength);

    return HalfAngle;
}

Radian player::CalculateActualYawToTarget(Object* pObject)
{
    Vector3 vViewToBoxCenter = pObject->GetColBBox().GetCenter() - m_EyesPosition;

    // Calculate the differences in pitch
    Radian YawBox; //, WorldViewYaw;
    YawBox = vViewToBoxCenter.GetYaw();

    return x_MinAngleDiff(YawBox, m_EyesYaw);
}

Radian player::CalculateActualPitchToTarget(Object* pObject)
{
    Vector3 vViewToBoxCenter = pObject->GetColBBox().GetCenter() - m_EyesPosition;

    // Calculate the differences in pitch
    Radian PitchBox;
    PitchBox = vViewToBoxCenter.GetPitch();

    return x_MinAngleDiff(PitchBox, m_EyesPitch);
}

Radian player::GetSightYaw() const
{
    return m_EyesYaw;
}

player::animation_state player::SetupMutationMeleeWeapon()
{
    // Do not create a 3rd person camera for network ghosts.
    if (m_LocalSlot == -1) {
        return ANIM_STATE_MELEE;
    }

    // set a default
    animation_state AnimState = ANIM_STATE_MELEE;

    // make sure we have our "mutant melee weapon" out
    if (!GetMutationMeleeWeapon()) {
        return AnimState;
    }

    {
        // only do this the first time
        // IJB g_AudioMgr.Play( "Mut_Melee" );

        // get the swat anim
        AnimState = GetNextMeleeState();
    }

    m_NextAnimState = ANIM_STATE_UNDEFINED;

    SetAnimation(AnimState, ANIM_PRIORITY_DEFAULT);
    // IJB GetMutationMeleeWeapon()->Setup( GetGuid(), AnimState );

    return AnimState;
}

void player::DoTendrilCollision()
{
    tweak_handle ReachDistanceTweak("PLAYER_TendrilReachDistance");
    tweak_handle SphereRadiusTweak("PLAYER_TendrilCheckRadius");
    float        MeleeReachDistance = ReachDistanceTweak.GetF32();
    float        MeleeSphereRadius = SphereRadiusTweak.GetF32();

    Vector3 StartPos = GetView().GetPosition();
    Vector3 EndPos = StartPos + GetView().GetViewZ() * MeleeReachDistance;

    g_CollisionMgr.SphereSetup(GetGuid(), StartPos, EndPos, MeleeSphereRadius);
    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_LIVING, Object::ATTR_COLLISION_PERMEABLE);
}

weapon_mutation* player::GetMutationMeleeWeapon(void)
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // make sure this is a mutation weapon
    /* IJB
    if( pWeapon && pWeapon->IsKindOf( weapon_mutation::GetRTTI()) )
    {
        return (weapon_mutation*)pWeapon;
    }
*/
    return nullptr;
}

/*
new_weapon::reticle_radius_parameters player::GetReticleParams(  )
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if( pWeapon )
    {
        if (m_CurrentAnimState == ANIM_STATE_ZOOM_IDLE ||
            m_CurrentAnimState == ANIM_STATE_ZOOM_RUN ||
            m_CurrentAnimState == ANIM_STATE_ZOOM_FIRE)
        {
            //alt fire
            return pWeapon->GetAltReticleRadiusParameters();
        }
        else
        {
            //standard fire
            return pWeapon->GetReticleRadiusParameters();
        }
    }
    else
    {
        new_weapon::reticle_radius_parameters Params;
        Params.m_CrouchBonus            = 0.0f;
        Params.m_GrowAccel              = 0.0f;
        Params.m_MaxMovementPenalty     = 0.0f;
        Params.m_MaxRadius              = 0.0f;
        Params.m_MoveShrinkAccel        = 0.0f;
        Params.m_PenaltyForShot         = 0.0f;
        Params.m_ShotPenaltyDegradeRate = 0.0f;
        Params.m_ShotShrinkAccel        = 0.0f;

        return Params;
    }
}
*/
// ---------------------------------------------

void player::UpdateReticleRadius(float DeltaTime)
{
    /* IJB

        new_weapon* pWeapon = GetCurrentWeaponPtr();

        if( !pWeapon )
            return;

        new_weapon::reticle_radius_parameters ReticleParams = GetReticleParams();

        assert( SmallReticleSpeed > LargeReticleSpeed );
        assert( ReticleParams.m_MaxRadius > ReticleParams.m_MinRadius );

        //
        // First, figure out what the movement penalty is
        //
        float Speed = m_Physics.GetVelocity().Length();
        float DesiredRadius = ReticleParams.m_MaxRadius; // we'll apply penalties for movement and shooting to this

        if ( Speed >= SmallReticleSpeed )
        {
            DesiredRadius -= ReticleParams.m_MaxMovementPenalty;
        }
        else if ( Speed <= LargeReticleSpeed )
        {
            DesiredRadius = ReticleParams.m_MaxRadius; // no penalty from movement
        }
        else
        {
            const float RelativeSpeed     = Speed - LargeReticleSpeed;                // Speed relative to reticle speed range
            const float SpeedRange        = SmallReticleSpeed - LargeReticleSpeed;

            assert( RelativeSpeed > 0.0f );
            assert( RelativeSpeed <= SpeedRange );

            float Penalty = std::min( ReticleParams.m_MaxMovementPenalty, ((RelativeSpeed / SpeedRange) * ReticleParams.m_MaxMovementPenalty) );
            DesiredRadius = ReticleParams.m_MaxRadius - Penalty;
        }


        //
        // Next, figure out what the shooting penalty is
        //

        // Degrade shot penalty
        m_ReticleShotPenalty -= ReticleParams.m_ShotPenaltyDegradeRate * DeltaTime;
        m_ReticleShotPenalty = std::max( 0.0f, m_ReticleShotPenalty );
        DesiredRadius -= m_ReticleShotPenalty;
        DesiredRadius = std::max( ReticleParams.m_MinRadius, DesiredRadius );

        //
        // Add in the crouch bonus as needed
        //
        if ( m_bIsCrouching )
        {
            DesiredRadius += ReticleParams.m_CrouchBonus;
        }


        //
        // Now update radius speed and current radius
        //
        if ( m_ReticleRadius > DesiredRadius)
        {
            if ( m_ReticleGrowSpeed > 0.0f )
            {
                // if growing, stop
                m_ReticleGrowSpeed = 0.0f;
            }

            // we need to shrink
            const float ShrinkAccel = (m_ReticleShotPenalty > 0.0f) ? ReticleParams.m_ShotShrinkAccel : ReticleParams.m_MoveShrinkAccel;
            m_ReticleGrowSpeed -= (ShrinkAccel * DeltaTime);
        }
        else if ( m_ReticleRadius < DesiredRadius )
        {
            if ( m_ReticleGrowSpeed < 0.0f )
            {
                // if shrinking, stop
                m_ReticleGrowSpeed = 0.0f;
            }

            // we need to grow
            m_ReticleGrowSpeed += (ReticleParams.m_GrowAccel * DeltaTime);
        }

        float GrowAmount = m_ReticleGrowSpeed * DeltaTime;
        float NewRadius = m_ReticleRadius + GrowAmount;

        if (   ((m_ReticleRadius < DesiredRadius) && (NewRadius > DesiredRadius))
            || ((m_ReticleRadius > DesiredRadius) && (NewRadius < DesiredRadius)) )
        {
            // we overshot our goal, just end up there
            NewRadius = DesiredRadius;
            m_ReticleGrowSpeed = 0.0f;
        }

        m_ReticleRadius = NewRadius;

        // Clamp
        const float Max = ReticleParams.m_MaxRadius + ReticleParams.m_CrouchBonus;
        if ( m_ReticleRadius > Max )
        {
            m_ReticleRadius = Max;
            m_ReticleGrowSpeed = 0.0f;
        }
        else if ( (m_ReticleRadius < ReticleParams.m_MinRadius) && (GrowAmount < 0.0f) )
        {
            m_ReticleRadius = ReticleParams.m_MinRadius;
            m_ReticleGrowSpeed = 0.0f;
        }
    */
}

//=============================================================================

Vector3 player::GetWeaponCollisionOffset(guid WeaponGuid, const Vector3& FirePos)
{
    const Vector3 Dir(m_Pitch, m_Yaw);

    //
    // Come up with the point to use for our collision start.
    // It will be along the gun's aim axis
    //
    static const float Dist = 100.0f;
    Vector3            WeaponStalk(FirePos - (Dir * Dist));
    Vector3            PtOnA;
    Vector3            PtOnB;
    x_ClosestPtsOnLineSegs(WeaponStalk, FirePos, GetPosition(), GetPosition() + Vector3(0.0f, 200.0f, 0.0f), PtOnA, PtOnB);

    Vector3 Start(PtOnA);
    Vector3 End(FirePos - (Dir * WeaponCollisionRadius)); // back off by our radius

    g_CollisionMgr.SphereSetup(WeaponGuid, Start, End, WeaponCollisionRadius);
    g_CollisionMgr.UseLowPoly();
    g_CollisionMgr.SetMaxCollisions(1);
    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PLAYER, Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING);

    // If we hit anything, figure out where we want to be
    float DesiredCollisionOffsetScalar = 0.0f;
    if (g_CollisionMgr.m_nCollisions > 0) {
        DesiredCollisionOffsetScalar = (Start - End).Length();
        DesiredCollisionOffsetScalar *= (1.01f - g_CollisionMgr.m_Collisions[0].T);
    }

    // Move towards our goal
    static float s_WeaponSlideSpeed = 150.0f;

    float Delta = DesiredCollisionOffsetScalar - m_WeaponCollisionOffsetScalar;
    if (abs(Delta) > 0.01f) {
        if (m_LastWeaponCollisionOffsetScalar > DesiredCollisionOffsetScalar) {
            // We're moving the weapon forward, towards neutral
            // move this direction smoothly
            const float Distance = s_WeaponSlideSpeed * m_DeltaTime;
            if (abs(Delta) < Distance) {
                // we'd overshoot it, so just go there
                m_WeaponCollisionOffsetScalar = DesiredCollisionOffsetScalar;
            } else {
                if (DesiredCollisionOffsetScalar > m_WeaponCollisionOffsetScalar) {
                    Delta = Distance;
                } else {
                    Delta = -Distance;
                }

                m_WeaponCollisionOffsetScalar += Delta;
            }
        } else {
            // We're moving the weapon back out of a wall, do this instantly
            m_WeaponCollisionOffsetScalar = DesiredCollisionOffsetScalar;
        }
    } else {
        m_WeaponCollisionOffsetScalar = DesiredCollisionOffsetScalar;
    }

    Vector3 Pullback(-Dir);
    Pullback *= m_WeaponCollisionOffsetScalar;
    return Pullback;
}

//===========================================================================

void player::SetMeleeState(animation_state MeleeState)
{
    // Get a reference to the state that we are considering
    state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][MeleeState];

    // Can we fire the secondary weapon?
    if (State.nPlayerAnims > 0) {
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon) {
            pWeapon->ClearZoom();
        }

        SetAnimState(MeleeState);
    }
}

//==============================================================================

bool player::AllowedToFire(void)
{
    if (m_bHidePlayerArms) {
        return false;
    }

    if (m_Cinema.m_bCinemaOn) {
        return false;
    }

    return true;
}

//==============================================================================

void player::OnWeaponSwitch2(const cycle_direction& CycleDirection)
{
    if ((m_CurrentAnimState == ANIM_STATE_GRENADE) ||
        (m_CurrentAnimState == ANIM_STATE_ALT_GRENADE) ||
        (m_CurrentAnimState == ANIM_STATE_DISCARD)) {
        return;
    }

    if (IsMutated()) {
        return;
    }

    //player is attempting to switch weapons, for now, we'll just cycle through the weapons that are in the inventory
    SetNextWeapon2(GetNextAvailableWeapon2(CycleDirection));
}

//==============================================================================

void player::SetNextWeapon2(inven_item WeaponItem, bool ForceSwitch, bool StateChange)
{
    if ((WeaponItem == m_CurrentWeaponItem) && !ForceSwitch) {
        // No need to switch to the current weapon
        return;
    }

    m_NextWeaponItem = WeaponItem;

    // turn off the flashlight
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        pWeapon->ClearZoom();

        // make sure we clear anything the weapon is doing
        pWeapon->BeginSwitchFrom();
    } else {
        // weapon is invalid?  Turn off flashlight then
        SetFlashlightActive(false);
    }

    // Check weapon that's about to be the current weapon.
    pWeapon = GetWeaponPtr(WeaponItem);
    if (pWeapon) {
        // this new weapon doesn't have a flashlight, turn it off
        if (!pWeapon->HasFlashlight()) {
            SetFlashlightActive(false);
        }
    }

    if (StateChange && !m_bDead) {
        // Switch to the next weapon.
        SetAnimState(ANIM_STATE_SWITCH_FROM);
    }
}

//==============================================================================

bool player::ShouldSkipWeaponCycle(const inven_item& CurrentWeaponItem, const inven_item& NextWeapon)
{
    switch (CurrentWeaponItem) {
    case INVEN_WEAPON_SMP:
    {
        if (NextWeapon == INVEN_WEAPON_DUAL_SMP) {
            return true;
        }
    } break;
    case INVEN_WEAPON_DUAL_SMP:
    {
        if (NextWeapon == INVEN_WEAPON_SMP) {
            return true;
        }
    } break;

    case INVEN_WEAPON_SHOTGUN:
    {
        if (NextWeapon == INVEN_WEAPON_DUAL_SHT) {
            return true;
        }
    } break;
    case INVEN_WEAPON_DUAL_SHT:
    {
        if (NextWeapon == INVEN_WEAPON_SHOTGUN) {
            return true;
        }
    } break;
    default:
    {
        return false;
    } break;
    }

    return false;
}

//=============================================================================

inven_item player::GetNextAvailableWeapon2(const cycle_direction& CycleDirection)
{
    inven_item CurrentWeaponItem = m_CurrentWeaponItem;

    //determine if we need to add or subtract from m_CurrentWeapon
    int Direction = 1;
    if (CycleDirection == CYCLE_LEFT) {
        Direction = -1;
    }
    //select the next weapon in the list to test.
    uint32_t NextWeapon = (inven_item)(((int)CurrentWeaponItem + Direction) % INVEN_NUM_WEAPONS);
    NextWeapon = (NextWeapon > INVEN_NUM_WEAPONS) ? INVEN_NUM_WEAPONS - 1 : NextWeapon;

    // TODO: CJ: WEAPONS: Check for infinite loop?
    while ((inven_item)NextWeapon != CurrentWeaponItem) {
        // should we skip this weapon (i.e. dual SMPs switching to/from SMP)
        if (!ShouldSkipWeaponCycle(CurrentWeaponItem, (inven_item)NextWeapon)) {
            //if the weapon is in our inventory, that's is for the while loop. can't cycle to mutation weapon
            if ((NextWeapon != INVEN_WEAPON_MUTATION) && (m_Inventory2.HasItem((inven_item)NextWeapon))) {
                break;
            }
        }

        NextWeapon = (((int)NextWeapon + Direction) % INVEN_NUM_WEAPONS);
        NextWeapon = (NextWeapon > INVEN_NUM_WEAPONS) ? INVEN_NUM_WEAPONS - 1 : NextWeapon;
    }

    // Check for Dual SMP
    if (((inven_item)NextWeapon == INVEN_WEAPON_SMP) && m_Inventory2.HasItem(INVEN_WEAPON_DUAL_SMP)) {
        NextWeapon = INVEN_WEAPON_DUAL_SMP;
    } else
        // Check for Dual Shotguns
        if (((inven_item)NextWeapon == INVEN_WEAPON_SHOTGUN) && m_Inventory2.HasItem(INVEN_WEAPON_DUAL_SHT)) {
            NextWeapon = INVEN_WEAPON_DUAL_SHT;
        }

    return (inven_item)NextWeapon;
}

#define RENDER_AIM_ASSIST 0

void player::RenderAimAssistDebugInfo(void)
{
}

//===========================================================================

void player::AttachWeapon()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        Radian3 Rot(m_AnimPlayer.GetPitch(), m_AnimPlayer.GetYaw(), m_AnimPlayer.GetRoll());
        Vector3 Pos(m_AnimPlayer.GetPosition());

        Matrix4 L2W;
        L2W.Identity();
        L2W.SetRotation(Rot);
        L2W.SetTranslation(Pos);

        //( (new_weapon*)pObject )->SetRotation
        //( (new_weapon*)pObject )->OnMove
        OnTransformWeapon(L2W);
        pWeapon->SetZone1(GetZone1());
    }
}

//===========================================================================

void player::AddNewWeapon2(inven_item WeaponItem)
{
    // Already have the weapon?
    if (m_Inventory2.HasItem(WeaponItem)) {
        return;
    }

    // Add the weapon
    m_Inventory2.AddAmount(WeaponItem, 1.0f);

    if ((m_CurrentWeaponItem == INVEN_NULL) && (WeaponItem != INVEN_WEAPON_MUTATION)) {
        m_CurrentWeaponItem = WeaponItem;

        // Set the render index.
        new_weapon* pWeapon = GetWeaponPtr(WeaponItem);
        if (pWeapon) {
            pWeapon->SetupRenderInformation();
        }
    }
}

//===========================================================================

bool player::TryAddAmmo2(inven_item Item)
{
    (void)Item;
    // TODO: CJ: WEAPONS: This code needs revisting to get ammo back into the weapons

    /*
    // lookup the weapon we are using.
    player_virtual_weapon WeaponType = GetWeaponStateFromType( pWeapon->GetType() );

    // Couldn't find the weapon?
    if( WeaponType == WEAPON_UNDEFINED )
    {
    return false;
    }

    //
    // mreed: ??? what's this code trying to do, and what does it have to do with dual? 5/8/2004
    //
    // If we have the weapon, check if we are maxed out on ammo.
    if( m_bWeaponInInventory[WeaponType] )
    {
    player_weapon_obj PlayerWeaponObj = GetWeaponObjFromVirtual( WeaponType );
    object_ptr<new_weapon> WeaponObj( m_GuidWeaponArray[PlayerWeaponObj] );

    int nCurrentAmmoCount = WeaponObj.m_pObject->GetTotalPrimaryAmmo();
    int nMaxAmmoCount     = WeaponObj.m_pObject->GetMaxPrimaryAmmo();

    // refill ammo if we are low
    if( nCurrentAmmoCount < nMaxAmmoCount )
    {
    ((new_weapon*)WeaponObj.m_pObject)->SetupDualAmmo();
    return true;
    }
    }
    */
    return false;
}

//===========================================================================

bool player::ShouldSwitchToWeapon2(inven_item WeaponItem, bool bFirstPickup)
{
    if (m_bDead) {
        return false;
    }

    if (m_CurrentWeaponItem == INVEN_NULL) {
        // Player has no weapon -> Probably a good idea to switch
        return true;
    }

    // make sure that if we are mutated, we don't switch weapons, AT ALL!
    if (IsMutated()) {
        return false;
    }

    // TODO: CJ: WEAPONS: Revisit this to put the rules in place
    // mreed: temporary hack until we get all the rules defined and in place
    bool RetVal = false;

    // this is a defaulted value for the editor, in the game it will check a profile setting.
    bool bShouldCheckRating = true;

    /* IJB
    // get player profile
    player_profile& p = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));

    bShouldCheckRating = p.GetWeaponAutoSwitch();
*/

    inven_item ParentItem = new_weapon::GetParentIDForDualWeapon(WeaponItem);

    // see if this is a dual weapon
    bool bSwitchDualWeapon = false;

    if (ParentItem != INVEN_NULL) {
        // is our current weapon the parent of this new weapon?
        // if so, this means we need to switch to the dual version
        bSwitchDualWeapon = (m_CurrentWeaponItem == ParentItem);
    }

    // if weapon auto-switch is set, check weapon auto-switch ratings if this is the first time we picked it up.
    if ((bShouldCheckRating && bFirstPickup) || bSwitchDualWeapon) {
        new_weapon* pCurrentWeapon = GetCurrentWeaponPtr();
        new_weapon* pNewWeapon = GetWeaponPtr(WeaponItem);

        // if our new weapon is rated higher than current, switch to it.
        if (pNewWeapon && pCurrentWeapon && (pNewWeapon->GetAutoSwitchRating() > pCurrentWeapon->GetAutoSwitchRating())) {
            RetVal = true;
        }
    }

    return RetVal;
}

/*
void player::OnWeaponAnimInit2( inven_item WeaponItem, new_weapon* pWeapon )
{
    // If the weapons has already been initialized with the player don't do it again.
    if( m_WeaponGuids[inventory2::ItemToWeaponIndex(WeaponItem)] != 0 )
        return;

    //reset this weapon's animation table
    ResetWeaponAnimTable2( WeaponItem );

    if ( pWeapon->IsInited() )
    {
        pWeapon->SetupRenderInformation( );

        if( pWeapon->HasAnimGroup() == false )
            return;

        // Get the weapon's anim group that we're initializing.
        const anim_group& WeaponAnimGroup = pWeapon->GetCurrentAnimGroup();
        animation_state AnimIndex = ANIM_STATE_UNDEFINED;

        for ( int i = 0; i < WeaponAnimGroup.GetNAnims(); i++ )
        {
            AnimIndex = ANIM_STATE_UNDEFINED;

            const anim_info& AnimInfo   = WeaponAnimGroup.GetAnimInfo( i );
            const char*      pAnimName  = AnimInfo.GetName();

            // This animation is always there...
            if ( x_strcmp( pAnimName, "BIND_POSE" ) == 0 )
                continue;

            AnimIndex = GetAnimStateFromName( pAnimName );

            if ( AnimIndex == ANIM_STATE_UNDEFINED )
            {
                continue;
            }

            //We have valid index to the animation table, now set the values
            state_anims& State =  m_Anim[inventory2::ItemToWeaponIndex(WeaponItem)][AnimIndex];

            if( State.nWeaponAnims >= MAX_ANIM_PER_STATE )
            {
                x_try;
                x_throw( xfs( "WARNING: Too many animations of this type %s for weapon" , pAnimName ));
                x_catch_display;
                continue;
            }
            else
            {
                // TODO:  Eventually, weapons will need to contain animation sets for all
                //        player strains.  This sets all weapon animations to the human set for now.
                // Set the index of the animation
                assert(State.nWeaponAnims < MAX_ANIM_PER_STATE);
                State.WeaponAnim[State.nWeaponAnims] = i;
                State.nWeaponAnims++;
            }
        }

        pWeapon->SetupRenderInformation( );
    }
}
*/

void player::GenerateFiringAnimPercentages(void)
{
    //get the current state.
    state_anims& State = m_Anim[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)][m_CurrentAnimState];

    //set up the percentages.  if there's 4 animations: .70 / .15 / .10 / .05
    if (State.nPlayerAnims == MAX_ANIM_PER_STATE) {
        m_fAnimPriorityPercentage[0] = 0.70f;
        m_fAnimPriorityPercentage[1] = 0.85f;
        m_fAnimPriorityPercentage[2] = 0.95f;
        m_fAnimPriorityPercentage[3] = 1.00f;
    }

    //if there's 3 animations: .75 / .15 / .10 / 0
    else if (State.nPlayerAnims == 3) {
        m_fAnimPriorityPercentage[0] = 0.7f;
        m_fAnimPriorityPercentage[1] = 0.9f;
        m_fAnimPriorityPercentage[2] = 1.0f;
        m_fAnimPriorityPercentage[3] = 0.0f;
    }

    //if there's 2 animations: .75 / .25 / 0 / 0
    else if (State.nPlayerAnims == 2) {
        m_fAnimPriorityPercentage[0] = 0.75f;
        m_fAnimPriorityPercentage[1] = 1.00f;
        m_fAnimPriorityPercentage[2] = 0.00f;
        m_fAnimPriorityPercentage[3] = 0.00f;
    }

    //if there's 1 animations: 1.0 / 0 / 0 / 0
    else if (State.nPlayerAnims == 1) {
        m_fAnimPriorityPercentage[0] = 1.0f;
        m_fAnimPriorityPercentage[1] = 0.0f;
        m_fAnimPriorityPercentage[2] = 0.0f;
        m_fAnimPriorityPercentage[3] = 0.0f;
    } else {
        assert(false);
    }
}

//==============================================================================

int player::GetNextFiringAnimIndex(void)
{
    //generate a random number between 0 and 1
    float fRand = x_frand(0.0f, 1.0f);

    for (int i = 0; i < MAX_ANIM_PER_STATE; i++) {
        if (fRand < m_fAnimPriorityPercentage[i]) {
            return i;
        }
    }

    assert(false);
    return 0;
}

//==============================================================================

void player::ResetWeaponAnimTable2(inven_item WeaponItem)
{
    //  LOG_MESSAGE( "player::ResetWeaponAnimTable", "" );

    //clear the weapon animation array only.
    for (int j = 0; j < ANIM_STATE_DEATH; j++) {
        m_Anim[inventory2::ItemToWeaponIndex(WeaponItem)][j].nWeaponAnims = 0;
    }
}

//==============================================================================

void player::EmitMeleePain(void)
{
    tweak_handle ReachDistanceTweak("PLAYER_MeleeReachDistance");
    tweak_handle SphereRadiusTweak("PLAYER_MeleeSphereRadius");
    float        MeleeReachDistance = ReachDistanceTweak.GetF32();
    float        MeleeSphereRadius = SphereRadiusTweak.GetF32();

    //
    // Fire a sphere out from the eye the correct distance and
    // determine if we hit anything.
    //
    guid    DirectHitGuid = 0;
    Vector3 HitPosition;
    {
        Vector3 StartPos = GetView().GetPosition();
        Vector3 EndPos = StartPos + GetView().GetViewZ() * MeleeReachDistance;

        g_CollisionMgr.SphereSetup(GetGuid(), StartPos, EndPos, MeleeSphereRadius);
        g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_LARGE_PROJECTILES, Object::ATTR_COLLISION_PERMEABLE);

        if (g_CollisionMgr.m_nCollisions) {
            DirectHitGuid = g_CollisionMgr.m_Collisions[0].ObjectHitGuid;
            HitPosition = g_CollisionMgr.m_Collisions[0].Point;
        }
    }

    // If there was no direct hit then there's nothing left to do
    if (DirectHitGuid == 0) {
        return;
    }

    // We hit something! Play a sound!
    if (m_PlayMeleeSound && DirectHitGuid) {
        m_PlayMeleeSound = false;

        // Create an event sound emitter.
        guid    Guid = objectManager->CreateObject(event_sound_emitter::GetObjectType());
        Object* pSndEventObj = objectManager->GetObjectByGuid(Guid);

        event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pSndEventObj);

        char DescName[64];
        snprintf(DescName, 64, "Melee_%s", EventEmitter.GetMaterialName(g_CollisionMgr.m_Collisions[0].Flags));

        EventEmitter.PlayEmitter(DescName,
                                 HitPosition,
                                 GetZone1(),
                                 event_sound_emitter::SINGLE_SHOT,
                                 m_WeaponGuids[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)]);
        // hit something, set flag
        m_bLastMeleeHit = true;
    } else {
        // hit nothing, set flag
        m_bLastMeleeHit = false;
    }

    // Do shakes and feedback (skip if cloth is hit otherwise this looks weird)
    Object* pHitObject = objectManager->GetObjectByGuid(DirectHitGuid);
    if ((pHitObject) && (pHitObject->GetType() != Object::TYPE_CLOTH_OBJECT) && (pHitObject->GetType() != Object::TYPE_FLAG)) {
        if (g_PlayerMeleeShakeView) {
            ShakeView(s_MeleeShakeTime[m_ComboCount], s_MeleeShakeAmount[m_ComboCount], s_MeleeShakeSpeed[m_ComboCount]);
        }

        if (g_PlayerMeleeDoFeedback) {
            DoFeedback((s_MeleeShakeTime[m_ComboCount] / 2.5f), s_MeleeFeedbackShakeAmount[m_ComboCount]);
        }
    }

    // Build pain
    pain Pain(objectManager);

    Pain.Setup(xfs("%s_MELEE_%d", GetLogicalName(), m_ComboCount), GetGuid(), HitPosition);
    Pain.SetDirection(GetView().GetViewZ());
    Pain.SetDirectHitGuid(DirectHitGuid);
    Pain.SetCollisionInfo(g_CollisionMgr.m_Collisions[0]);
    Pain.ApplyToObject(DirectHitGuid);

    // melee impact FX
    // IJB particle_emitter::CreateProjectileCollisionEffect(g_CollisionMgr.m_Collisions[0], GetGuid());
}

//===========================================================================

const char* player::GetCurrentWeaponName(void)
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    if (pWeapon == nullptr) {
        return "";
    }

    switch (pWeapon->GetType()) {
    case Object::TYPE_WEAPON_SMP:
        return "SMP";
        // KSS -- TO ADD NEW WEAPON
    case Object::TYPE_WEAPON_SHOTGUN:
        return "SHT";
    case Object::TYPE_WEAPON_SCANNER:
        return "SCN";
    case Object::TYPE_WEAPON_SNIPER:
        return "SNI";
    case Object::TYPE_WEAPON_DESERT_EAGLE:
        return "EGL";
    case Object::TYPE_WEAPON_MSN:
        return "MSN";
    case Object::TYPE_WEAPON_BBG:
        return "BBG";
    case Object::TYPE_WEAPON_TRA:
        return "TRA";
    case Object::TYPE_WEAPON_MUTATION:
        return "MUT";
    default:
        return "";
    }
}

//=============================================================================

float player::GetLastTimeWeaponFired()
{
    return m_LastTimeWeaponFired;
}

void player::ResetWeaponFlags(void)
{
}

//===========================================================================

guid player::GetCurrentWeaponGuid2(void)
{
    return m_WeaponGuids[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)];
}

//===========================================================================

void player::SwitchWeapon2(inven_item WeaponItem)
{
    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    if (WeaponItem != m_CurrentWeaponItem) {
        m_NextWeaponItem = WeaponItem;
    }

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================

    if (WeaponItem != m_CurrentWeaponItem) {
        //set the state to ANIM_STATE_SWITCH_FROM
        SetAnimState(ANIM_STATE_SWITCH_FROM);

        // turn off the flashlight
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (!pWeapon || (!pWeapon->HasFlashlight())) {
            // weapon is invalid?  Turn off flashlight then
            SetFlashlightActive(false);
        } else if (pWeapon->HasFlashlight()) {
            SetFlashlightActive(true);
        }
    }
}

//===========================================================================

void player::OnMoveWeapon(void)
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // update weapon position
    if (pWeapon) {
        // call weapon's OnMove
        pWeapon->OnMove(m_AnimPlayer.GetPosition());

        // update zones
        pWeapon->SetZone1(GetZone1());

        // move the flashlight if it's active
        MoveFlashlight();
    }
}

void player::OnTransformWeapon(const Matrix4& L2W)
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        // call weapon's OnTransform
        pWeapon->OnTransform(L2W);

        // Update 3rd person weapon also so split screen works
        actor::MoveWeapon(false);

        // move the flashlight if it's active
        MoveFlashlight();
    }
}

void player::RemoveAllWeaponInventory(void)
{
    // Remove all the weapons from the inventory.
    for (int i = 0; i < INVEN_NUM_WEAPONS; i++) {
        m_Inventory2.SetAmount(inventory2::WeaponIndexToItem(i), 0.0f);
    }

    // Set the next weapon type and switch from the current weapon.
    m_NextWeaponItem = INVEN_NULL;
    SetAnimState(ANIM_STATE_SWITCH_FROM);
}

int player::GetWeaponRenderState(void)
{
    return new_weapon::RENDER_STATE_PLAYER;
}

/* IJB
void player::HandleBulletFlyby(bullet_projectile& Bullet)
{
    Vector3 ClosestPoint;
    Vector3 End = Bullet.GetCurrentPos();
    Vector3 Start = Bullet.GetInitialPos();
    Vector3 Velocity = Bullet.GetVelocity();
    Vector3 EarPosition = GetPosition() + Vector3(0, 100, 0);

    // Find closest point.
    ClosestPoint = EarPosition.GetClosestPToLSeg(Start, End);
    Velocity.Normalize();

    Vector3 Delta = ClosestPoint - End;
    if (Delta.Length() > 1.0f) {
        // Should we play the fly by? 5 meter limit for now...
        Vector3 EarToBullet = ClosestPoint - EarPosition;
        float   BulletDistance = EarToBullet.Length();
        if (BulletDistance < 500.0f) {
            // Look for an unused fly by...
            for (int i = 0; i < MAX_FLY_BYS; i++) {
                // Is it active?
                if (!m_BulletFlyBy[i].bIsActive) {
                    // movement over 6 meters at current time...
                    m_BulletFlyBy[i].Start = ClosestPoint - (Velocity * 400);
                    m_BulletFlyBy[i].End = ClosestPoint + (Velocity * 1200);
                    m_BulletFlyBy[i].VoiceID = g_AudioMgr.PlayVolumeClipped("BulletFlyBy", m_BulletFlyBy[i].Start, GetZone1(), true);
                    m_BulletFlyBy[i].Age = 0.0f;
                    m_BulletFlyBy[i].Lifetime = g_AudioMgr.GetLengthSeconds(m_BulletFlyBy[i].VoiceID);
                    m_BulletFlyBy[i].bIsActive = true;
                    break;
                }
            }
        }
    }
}
*/

void player::UpdateBulletSounds(float DeltaTime)
{
    // Look for active fly bys...
    for (int i = 0; i < MAX_FLY_BYS; i++) {
        // Only if its active...
        if (m_BulletFlyBy[i].bIsActive) {
            // Update lifetime...
            m_BulletFlyBy[i].Age += DeltaTime;

            // Still alive?
            /* IJB
            if ((m_BulletFlyBy[i].Age < m_BulletFlyBy[i].Lifetime) && g_AudioMgr.IsValidVoiceId(m_BulletFlyBy[i].VoiceID)) {
                float   Scale = m_BulletFlyBy[i].Age / m_BulletFlyBy[i].Lifetime;
                Vector3 Pos = m_BulletFlyBy[i].Start + Scale * (m_BulletFlyBy[i].End - m_BulletFlyBy[i].Start);
                g_AudioMgr.SetPosition(m_BulletFlyBy[i].VoiceID, Pos, GetZone1());
            } else {
             */ 
                // Kill it!
                m_BulletFlyBy[i].bIsActive = false;
            // IJB }
        }
    }
}

bool player::SetupDualWeaponDiscard(inven_item& WeaponItem)
{
    switch (WeaponItem) {
    case INVEN_WEAPON_DUAL_SMP:
    {
        // clear dual
        m_Inventory2.SetAmount(INVEN_WEAPON_DUAL_SMP, 0.0f);

        // Set next weapon to the SMP
        WeaponItem = INVEN_WEAPON_SMP;

        new_weapon* pWeapon = GetWeaponPtr(WeaponItem);

        // clear current weapon's clip ammo so that we'll have to reload once we dump the dual.
        if (pWeapon) {
            pWeapon->ClearClipAmmo();
        }
    } break;
    case INVEN_WEAPON_DUAL_SHT:
    {
        // clear dual
        m_Inventory2.SetAmount(INVEN_WEAPON_DUAL_SHT, 0.0f);

        // Set next weapon to the shotgun
        WeaponItem = INVEN_WEAPON_SHOTGUN;

        new_weapon* pWeapon = GetWeaponPtr(WeaponItem);

        // clear current weapon's clip ammo so that we'll have to reload once we dump the dual.
        if (pWeapon) {
            pWeapon->ClearClipAmmo();
        }
    } break;
    default:
    {
        // no dual discard
        return false;
    } break;
    }

    return true;
}

//=============================================================================

bool player::CheckForDualWeaponSetup(void)
{
    bool RetVal = false;

    // set up dual SMPs
    if (m_Inventory2.HasItem(INVEN_WEAPON_SMP) && !m_Inventory2.HasItem(INVEN_WEAPON_DUAL_SMP)) {
        m_Inventory2.SetAmount(INVEN_WEAPON_DUAL_SMP, 1.0f);
        SetNextWeapon2(INVEN_WEAPON_DUAL_SMP);

        new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid(m_WeaponGuids[inventory2::ItemToWeaponIndex(INVEN_WEAPON_DUAL_SMP)]);
        if (pWeapon) {
            // make sure this weapon's clip is full
            pWeapon->RefillClip(new_weapon::AMMO_PRIMARY);
        }

        // pick this SMP up, will dual wield it.
        RetVal = true;
    } else {
        if (m_Inventory2.HasItem(INVEN_WEAPON_DUAL_SMP)) {
            // already have one, don't pick it up
            RetVal = false;
        } else {
            m_Inventory2.SetAmount(INVEN_WEAPON_SMP, 1.0f);

            // don't have one at all yet, pick it up
            RetVal = true;
        }
    }

    return RetVal;
}
//==============================================================================

int player::GetAmmoFromWeaponType(inven_item Item)
{
    int Amount = 0;

    switch (Item) {
    case INVEN_WEAPON_SMP:
    case INVEN_WEAPON_SHOTGUN:
    case INVEN_WEAPON_DESERT_EAGLE:
    case INVEN_WEAPON_SNIPER_RIFLE:
    {
        // Find the weapon from item type and get the amount of max ammo per clip and return it
        new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid(m_WeaponGuids[inventory2::ItemToWeaponIndex(Item)]);
        if (pWeapon) {
            Amount = pWeapon->GetAmmoPerClip(new_weapon::AMMO_PRIMARY);
        }
    } break;
    default:
    case INVEN_WEAPON_DUAL_SMP:
    case INVEN_WEAPON_DUAL_SHT:
        //case INVEN_WEAPON_MESON_CANNON: // don't do anything for meson cannon?
    case INVEN_WEAPON_BBG: // don't do anything for BBG?
    {
        Amount = 0; // make sure
    } break;
    }

    return Amount;
}

#define LOGGING_ENABLED 0

void player::CreateAllWeaponObjects(void)
{
    //LOG_MESSAGE( "player::CreateAllWeaponObjects", "Creating weapons" );

    // Create the weapons for the player
    for (int i = 0; i < INVEN_NUM_WEAPONS; i++) {
        inven_item  WeaponItem = inventory2::WeaponIndexToItem(i);
        const char* pBlueprintName = inventory2::ItemToBlueprintName(WeaponItem);

        if (pBlueprintName) {
            // Get the weapon or create the weapon
            guid        WeaponGUID = m_WeaponGuids[i];
            new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid(WeaponGUID);
            if (pWeapon == nullptr) {
                m_WeaponGuids[i] = 0;
                // IJB WeaponGUID = g_TemplateMgr.CreateSingleTemplate(pBlueprintName, Vector3(0, 0, 0), Radian3(0, 0, 0), 0, 0);
            }

            if (WeaponGUID) {
                new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid(WeaponGUID);
                assert(pWeapon);

                pWeapon->InitWeapon(GetPosition(), new_weapon::RENDER_STATE_PLAYER, GetGuid());
                // IJB OnWeaponAnimInit2(WeaponItem, pWeapon);
                pWeapon->SetupRenderInformation();
                pWeapon->SetAnimation(ANIM_STATE_IDLE, 0.0f);
                pWeapon->BeginIdle();

                // make sure m_AmmoInCurrentClip and such are set up properly
                pWeapon->Reload(new_weapon::AMMO_PRIMARY);

                m_WeaponGuids[i] = WeaponGUID;

                //CLOG_MESSAGE( LOGGING_ENABLED, "player::CreateAllWeaponObjects", "Created: %s", inventory2::ItemToName( WeaponItem ) );
            }
        }
    }

    //----------------------------------------------------------
    // HACK: mreed since the flashlight doesn't save/restore,
    // we're killing it and setting our flashlight guid to
    // null. Then we call InitFlashlight, and everything works.
    //----------------------------------------------------------
    if (m_FlashlightGuid) {
        if (objectManager->GetObjectByGuid(m_FlashlightGuid)) {
            objectManager->DestroyObject(m_FlashlightGuid);
        }
        m_FlashlightGuid = 0;
    }
    InitFlashlight(GetPosition());
}

//------------------------------------------------------------------------------
bool player::ReloadWeapon(const new_weapon::ammo_priority& Priority, bool bCheckAmmo)
{
    int         ammoCount = 0;
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (!pWeapon) {
        return false;
    }

    // do NOT reload the mutation weapon
    if (m_CurrentWeaponItem == INVEN_WEAPON_MUTATION) {
        return false;
    }

    // if we don't want to check the ammo (i.e. player pushed "reload button") then ignore count
    if (bCheckAmmo) {
        if (Priority == new_weapon::AMMO_PRIMARY) {
            ammoCount = pWeapon->GetAmmoCount(pWeapon->GetPrimaryAmmoPriority());
        } else {
            ammoCount = pWeapon->GetAmmoCount(pWeapon->GetSecondaryAmmoPriority());
        }
    }

    // If our clip is out of ammo, reload
    // KSS -- always use AMMO_PRIMARY when checking reload?
    // if we don't want to check the ammo (i.e. player pushed "reload button") then ignore count
    if (((ammoCount <= 0) || (!bCheckAmmo)) && pWeapon->CanReload(new_weapon::AMMO_PRIMARY)) {
        // CJ: Reset the zoom state in case of dry fire on sniper rile, etc.
        pWeapon->ClearZoom();

        SetAnimState(ANIM_STATE_RELOAD);

        // succeeded
        return true;
    }

    // didn't reload
    return false;
}

//==============================================================================

bool player::LoadWarnsLowAmmo(void)
{
    // Determine if this level is supposed to warn the player that they are low on clip ammo with a message
    bool bShouldWarn = false;

    /* IJB
    for (int i = 0; s_MapToWeaponTable[i].pLevelName; i++) {
        if (x_stristr(g_ActiveConfig.GetLevelPath(), s_MapToWeaponTable[i].pLevelName)) {
            bShouldWarn = s_MapToWeaponTable[i].bLoadWarnsLowAmmo;
            break;
        }
    }
*/
    return bShouldWarn;
}

//==============================================================================

void player::DebugEnableWeapons(const char* pLevelName)
{
    DebugSetupInventory(pLevelName);

    inven_item StartItem = GetEquipedWeaponForLevel(pLevelName);
    int        Weapons = GetStartWeaponsForLevel(pLevelName);

    // Setup start item
    m_PreMutationWeapon2 = INVEN_NULL;
    m_CurrentWeaponItem = StartItem;
    if (m_CurrentWeaponItem == INVEN_WEAPON_MUTATION) {
        // Force the player to mutate
        m_PreMutationWeapon2 = INVEN_WEAPON_SMP;
        SetNextWeapon2(INVEN_WEAPON_MUTATION, true);
    }

    // Enable mutation abilities
    if (Weapons & WB_MM) {
        m_bMutationMeleeEnabled = true;
    }

    if (Weapons & WB_MP) {
        m_bPrimaryMutationFireEnabled = true;
    }

    if (Weapons & WB_MS) {
        m_bSecondaryMutationFireEnabled = true;
    }

    // initialize flashlight
    Vector3 rInitPos = m_AnimPlayer.GetPosition();
    InitFlashlight(rInitPos);

    // Load tweaks
    LoadAimAssistTweakHandles();
}
