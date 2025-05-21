#include "Player.h"

#include "../tweakManager/TweakMgr.h"
#include "../xfiles/xfs.h"
#include "../xfiles/x_plus.h"
#include "../objectManager/ObjectPtr.h"

// #include "InputMgr\GamePad.hpp"
// #include "objects\ParticleEmiter.hpp"
// #include "objects\Render\PostEffectMgr.hpp"
// #include "objects\SpawnPoint.hpp"
#include "Event.h"
// #include "Sound\EventSoundEmitter.hpp"
// #include "..\support\templatemgr\TemplateMgr.hpp"
// #include "characters\Character.hpp"
// #include "Characters\Conversation_Packet.hpp"
// #include "GameLib\StatsMgr.hpp"
// #include "GameLib\RenderContext.hpp"
// #include "Dictionary\global_dictionary.hpp"
// #include "objects\WeaponSniper.hpp"
#include "ThirdPersonCamera.h"
// #include "objects\WeaponSMP.hpp"
#include "Corpse.h"
#include "CorpsePain.h"
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
// #include "Objects\Camera.hpp"
// #include "Objects\LevelSettings.hpp"
// #include "GameTextMgr\GameTextMgr.hpp"
// #include "StringMgr\StringMgr.hpp"
// #include "e_Audio.hpp"

class player_desc : public object_desc
{
public:
    player_desc()
        : object_desc(
              Object::TYPE_PLAYER,
              "Player",
              "ACTOR",
              Object::ATTR_PLAYER |
                  Object::ATTR_NEEDS_LOGIC_TIME |
                  Object::ATTR_COLLIDABLE |
                  Object::ATTR_BLOCKS_ALL_PROJECTILES |
                  Object::ATTR_BLOCKS_RAGDOLL |
                  Object::ATTR_BLOCKS_CHARACTER_LOS |
                  Object::ATTR_BLOCKS_PLAYER_LOS |
                  Object::ATTR_BLOCKS_SMALL_DEBRIS |
                  Object::ATTR_RENDERABLE |
                  Object::ATTR_CAST_SHADOWS |
                  Object::ATTR_SPACIAL_ENTRY |
                  Object::ATTR_DAMAGEABLE |
                  Object::ATTR_TRANSPARENT |
                  Object::ATTR_LIVING,

              FLAGS_GENERIC_EDITOR_CREATE |
                  FLAGS_IS_DYNAMIC)
    {
    }

    Object* Create(ObjectManager* om, collision_mgr* cm) { return new player(om, cm); }
};

static player_desc s_player_desc;

const object_desc& player::GetObjectType()
{
    return s_player_desc;
}

const object_desc& player::GetTypeDesc() const
{
    return s_player_desc;
}

bool    player::s_bPlayerDied( false );

static const float k_PainParticleDisplace = 20.0f;
static const float s_NotifyTime = 1.f;
static const float k_Modfactor = 2.5f;
static const float DeathCamStartBackDist = 800.0f;
static const float DeathCamEndDist = 500.0f;
static const float ZERO = 0.00001f;

float g_EnemyShootBBoxPct = 1.20f; // percent of screen bbox width to inflate for an aiming HIT bbox

extern bool  g_MirrorWeapon;
extern mtwt  s_MapToWeaponTable[];
extern float g_SpawnFadeTime;

view player::m_Views[MAX_LOCAL_PLAYERS];

static const float s_run_state_transition_speed = 150.f;
static const float DEATH_VIEW_YAW_DELAY = 0.5f; // seconds before camera starts rotating
float              TESTTIME = 0.f;
float              TESTTIMETWO = 0.f;

bool s_WeaponInventoryStrip = false;

float s_MaxPainTime = 2.0f;

// flashlight/batter stuff
float BATTERY_BURN_SECONDS = 2.0f;
float BATTERY_BURN_AMOUNT = 3.0f; // amount of burn per BATTERY_BURN_SECONDS

float BATTERY_GAIN_SECONDS = 2.0f; // gain per x seconds
float BATTERY_GAIN_AMOUNT = 4.0f;  // amount of gain per BATTERY_GAIN_SECONDS

extern tweak_handle Lore_Max_Detect_DistanceTweak; // what is the max distance at which our Geiger will pick up a lore object

// while in mutant form
tweak_handle MutagenChangeMutant_AtWill_Tweak("Mutagen_Change_Mutant_At_Will");    // percent of mutagen change per second in At Will MP mode.
tweak_handle MutagenChangeMutant_Forced_Tweak("Mutagen_Change_Mutant_Forced");     // percent of mutagen change per second in Force MP mode.
tweak_handle MutagenChangeMutant_Campaign_Tweak("Mutagen_Change_Mutant_Campaign"); // percent of mutagen change per second in campaign mode.

// While in human form
tweak_handle MutagenChangeHuman_AtWill_Tweak("Mutagen_Change_Human_At_Will");    // percent of mutagen change per second in At Will MP mode.
tweak_handle MutagenChangeHuman_Forced_Tweak("Mutagen_Change_Human_Forced");     // percent of mutagen change per second in Force MP mode.
tweak_handle MutagenChangeHuman_Campaign_Tweak("Mutagen_Change_Human_Campaign"); // percent of mutagen change per second in campaign mode.

// message timing tweaks
tweak_handle Item_Full_Msg_FadeTimeTweak("Item_Full_Msg_FadeTime");
tweak_handle Item_Full_Msg_DelayTimeTweak("Item_Full_Msg_DelayTime");

tweak_handle Item_Acquired_Msg_FadeTimeTweak("Item_Acquired_Msg_FadeTime");
tweak_handle Item_Acquired_Msg_DelayTimeTweak("Item_Acquired_Msg_DelayTime");

static const float kForwardDelay = 250.0f;
static const float kBackwardDelay = 175.0f;
static const float kFalloff = 0.85f;
static const float kMaxForwardVel = 6.0f;
static const float kFeetsPerInitStep = 3.0f;
static const float kFeetsSpeedMod = 4.0f;
static const float kMaxWalkVolume = 0.5f;
static const float kMaxRunVolume = 1.0f;
static const float kMinRunVolume = 0.65f;
static const float kMaxWalkVel = 2.5f;
static const float kMaxStrafeDelay = 500.0f;
static const float kMinStrafeDelay = 250.0f;
static const float kStrafeInit = 0.5f;
static const float kVertStrafeCutOff = 0.3f;
static const float kLowestVolume = 0.1f;

float g_GrenadeThrowForce = 2000.0f;

bool g_ShowPlayerPos = false;

bool g_RenderTendrilCollision = false;

extern bool g_game_running;

struct convulsion_tweaks
{
    float m_MutagenConvulsionMultiplierPeriod;
    float m_MinConvulsionPeriod;
    float m_ConvulsionDuration;
} g_ConvulsionTweaks =
    {
        0.2f,
        2.0f,
        2.0f};

AimAssistData::AimAssistData()
{
    BulletAssistDir = Vector3(0.0f, 0.0f, 0.0f);
    bReticleOn = false;
    BulletAssistBestDist = FLT_MAX;
    TurnDampeningT = 0.0f;
    TargetGuid = 0;
    LOFCollisionDist = 2500.0f;
    LOFSpineDist = 0.0f;
    SpinePt = Vector3(0.0f, 0.0f, 0.0f);
    LOFPt = Vector3(0.0f, 0.0f, 0.0f);
    LOFPtT = 0.0f;
    SpinePtT = 0.0f;
    LOFPtDist = 1.0f;

    ReticleRadius = 0.0f;
    BulletInnerRadius = 0.0f;
    BulletOuterRadius = 0.0f;
    TurnInnerRadius = 0.0f;
    TurnOuterRadius = 0.0f;

    // online stuff
    OnlineFriendlyTargetGuid = 0;
    AimDelta = Vector3(0.0f, 0.0f, 0.0f);
}

// tweak values
float AimAssist_LOF_Dist;

float AimAssist_Reticle_Near_Dist;
float AimAssist_Reticle_Far_Dist;
float AimAssist_Reticle_Near_Radius;
float AimAssist_Reticle_Far_Radius;

float AimAssist_Bullet_Inner_Near_Dist;
float AimAssist_Bullet_Inner_Far_Dist;
float AimAssist_Bullet_Inner_Near_Radius;
float AimAssist_Bullet_Inner_Far_Radius;

float AimAssist_Bullet_Outer_Near_Dist;
float AimAssist_Bullet_Outer_Far_Dist;
float AimAssist_Bullet_Outer_Near_Radius;
float AimAssist_Bullet_Outer_Far_Radius;

float AimAssist_Bullet_Angle;

float AimAssist_Turn_Inner_Near_Dist;
float AimAssist_Turn_Inner_Far_Dist;
float AimAssist_Turn_Inner_Near_Radius;
float AimAssist_Turn_Inner_Far_Radius;

float AimAssist_Turn_Outer_Near_Dist;
float AimAssist_Turn_Outer_Far_Dist;
float AimAssist_Turn_Outer_Near_Radius;
float AimAssist_Turn_Outer_Far_Radius;

float AimAssist_Turn_Damp_Near_Dist;
float AimAssist_Turn_Damp_Far_Dist;

// Tweak handles
tweak_handle AimAssist_LOF_Dist_Tweak;

tweak_handle AimAssist_Reticle_Near_Dist_Tweak;
tweak_handle AimAssist_Reticle_Far_Dist_Tweak;
tweak_handle AimAssist_Reticle_Near_Radius_Tweak;
tweak_handle AimAssist_Reticle_Far_Radius_Tweak;

tweak_handle AimAssist_Bullet_Inner_Near_Dist_Tweak;
tweak_handle AimAssist_Bullet_Inner_Far_Dist_Tweak;
tweak_handle AimAssist_Bullet_Inner_Near_Radius_Tweak;
tweak_handle AimAssist_Bullet_Inner_Far_Radius_Tweak;

tweak_handle AimAssist_Bullet_Outer_Near_Dist_Tweak;
tweak_handle AimAssist_Bullet_Outer_Far_Dist_Tweak;
tweak_handle AimAssist_Bullet_Outer_Near_Radius_Tweak;
tweak_handle AimAssist_Bullet_Outer_Far_Radius_Tweak;

tweak_handle AimAssist_Bullet_Angle_Tweak;

tweak_handle AimAssist_Turn_Inner_Near_Dist_Tweak;
tweak_handle AimAssist_Turn_Inner_Far_Dist_Tweak;
tweak_handle AimAssist_Turn_Inner_Near_Radius_Tweak;
tweak_handle AimAssist_Turn_Inner_Far_Radius_Tweak;

tweak_handle AimAssist_Turn_Outer_Near_Dist_Tweak;
tweak_handle AimAssist_Turn_Outer_Far_Dist_Tweak;
tweak_handle AimAssist_Turn_Outer_Near_Radius_Tweak;
tweak_handle AimAssist_Turn_Outer_Far_Radius_Tweak;

tweak_handle AimAssist_Turn_Damp_Near_Dist_Tweak;
tweak_handle AimAssist_Turn_Damp_Far_Dist_Tweak;

// flashlight tweaks
tweak_handle FlashlightAutoOffSecondsTweak("FlashlightAutoOffSeconds");       // how long before we turn off flashlight
tweak_handle FlashlightAutoOffBrightnessTweak("FlashlightAutoOffBrightness"); // what is the brightness level we shut the flashlight off at

tweak_handle Mutagen_Convulsion_Color_R("MUTATION_ConvulsionColorR");
tweak_handle Mutagen_Convulsion_Color_G("MUTATION_ConvulsionColorG");
tweak_handle Mutagen_Convulsion_Color_B("MUTATION_ConvulsionColorB");

void player::LoadAimAssistTweakHandles(void)
{
    return;
    // TODO
    /*
    // get weapon pointer
    new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid( m_WeaponGuids[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)] );
    if( !pWeapon )
    {
        return;
    }

    m_bTweakHandlesLoaded = true;

    // load up strings
    std::string StringPrefix = (const char*)xfs( "%s_%d", pWeapon->GetLogicalName(), pWeapon->GetZoomStep() );
    const char* pString = StringPrefix.c_str();

    AimAssist_LOF_Dist_Tweak.SetName                ( xfs("%s_LOF_Dist", pString) );

    AimAssist_Reticle_Near_Dist_Tweak.SetName       ( xfs("%s_Reticle_Near_Dist", pString) );
    AimAssist_Reticle_Far_Dist_Tweak.SetName        ( xfs("%s_Reticle_Far_Dist", pString) );
    AimAssist_Reticle_Near_Radius_Tweak.SetName     ( xfs("%s_Reticle_Near_Rad", pString) );
    AimAssist_Reticle_Far_Radius_Tweak.SetName      ( xfs("%s_Reticle_Far_Rad", pString) );

    AimAssist_Bullet_Inner_Near_Dist_Tweak.SetName  ( xfs("%s_Blt_In_Near_Dist", pString) );
    AimAssist_Bullet_Inner_Far_Dist_Tweak.SetName   ( xfs("%s_Blt_In_Far_Dist", pString) );
    AimAssist_Bullet_Inner_Near_Radius_Tweak.SetName( xfs("%s_Blt_In_Near_Rad", pString) );
    AimAssist_Bullet_Inner_Far_Radius_Tweak.SetName ( xfs("%s_Blt_In_Far_Rad", pString) );

    AimAssist_Bullet_Outer_Near_Dist_Tweak.SetName  ( xfs("%s_Blt_Out_Near_Dist", pString) );
    AimAssist_Bullet_Outer_Far_Dist_Tweak.SetName   ( xfs("%s_Blt_Out_Far_Dist", pString) );
    AimAssist_Bullet_Outer_Near_Radius_Tweak.SetName( xfs("%s_Blt_Out_Near_Rad", pString) );
    AimAssist_Bullet_Outer_Far_Radius_Tweak.SetName ( xfs("%s_Blt_Out_Far_Rad", pString) );

    AimAssist_Bullet_Angle_Tweak.SetName            ( xfs("%s_Blt_Angle", pString) );

    AimAssist_Turn_Inner_Near_Dist_Tweak.SetName    ( xfs("%s_Turn_In_Near_Dist", pString) );
    AimAssist_Turn_Inner_Far_Dist_Tweak.SetName     ( xfs("%s_Turn_In_Far_Dist", pString) );
    AimAssist_Turn_Inner_Near_Radius_Tweak.SetName  ( xfs("%s_Turn_In_Near_Rad", pString) );
    AimAssist_Turn_Inner_Far_Radius_Tweak.SetName   ( xfs("%s_Turn_In_Far_Rad", pString) );

    AimAssist_Turn_Outer_Near_Dist_Tweak.SetName    ( xfs("%s_Turn_Out_Near_Dist", pString) );
    AimAssist_Turn_Outer_Far_Dist_Tweak.SetName     ( xfs("%s_Turn_Out_Far_Dist", pString) );
    AimAssist_Turn_Outer_Near_Radius_Tweak.SetName  ( xfs("%s_Turn_Out_Near_Rad", pString) );
    AimAssist_Turn_Outer_Far_Radius_Tweak.SetName   ( xfs("%s_Turn_Out_Far_Rad", pString) );

    // how much emphasis on turn dampening at close ranges?
    AimAssist_Turn_Damp_Near_Dist_Tweak.SetName  ( xfs("%s_Turn_Damp_Near_Dist", pString) );
    AimAssist_Turn_Damp_Far_Dist_Tweak.SetName  ( xfs("%s_Turn_Damp_Far_Dist", pString) );
*/
}

void player::LoadAimAssistTweaks()
{
    return;
    // TODO
    /*
    // make sure weapon has been loaded
    new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid( m_WeaponGuids[inventory2::ItemToWeaponIndex(m_CurrentWeaponItem)] );
    if( !pWeapon || !m_bTweakHandlesLoaded )
    {
        return;
    }

    AimAssist_LOF_Dist                  = AimAssist_LOF_Dist_Tweak.GetF32();

    AimAssist_Reticle_Near_Dist         = AimAssist_Reticle_Near_Dist_Tweak.GetF32();
    AimAssist_Reticle_Far_Dist          = AimAssist_Reticle_Far_Dist_Tweak.GetF32();
    AimAssist_Reticle_Near_Radius       = AimAssist_Reticle_Near_Radius_Tweak.GetF32();
    AimAssist_Reticle_Far_Radius        = AimAssist_Reticle_Far_Radius_Tweak.GetF32();

    AimAssist_Bullet_Inner_Near_Dist    = AimAssist_Bullet_Inner_Near_Dist_Tweak.GetF32();
    AimAssist_Bullet_Inner_Far_Dist     = AimAssist_Bullet_Inner_Far_Dist_Tweak.GetF32();
    AimAssist_Bullet_Inner_Near_Radius  = AimAssist_Bullet_Inner_Near_Radius_Tweak.GetF32();
    AimAssist_Bullet_Inner_Far_Radius   = AimAssist_Bullet_Inner_Far_Radius_Tweak.GetF32();

    AimAssist_Bullet_Outer_Near_Dist    = AimAssist_Bullet_Outer_Near_Dist_Tweak.GetF32();
    AimAssist_Bullet_Outer_Far_Dist     = AimAssist_Bullet_Outer_Far_Dist_Tweak.GetF32();
    AimAssist_Bullet_Outer_Near_Radius  = AimAssist_Bullet_Outer_Near_Radius_Tweak.GetF32();
    AimAssist_Bullet_Outer_Far_Radius   = AimAssist_Bullet_Outer_Far_Radius_Tweak.GetF32();

    AimAssist_Bullet_Angle              = AimAssist_Bullet_Angle_Tweak.GetF32();

    AimAssist_Turn_Inner_Near_Dist      = AimAssist_Turn_Inner_Near_Dist_Tweak.GetF32();
    AimAssist_Turn_Inner_Far_Dist       = AimAssist_Turn_Inner_Far_Dist_Tweak.GetF32();
    AimAssist_Turn_Inner_Near_Radius    = AimAssist_Turn_Inner_Near_Radius_Tweak.GetF32();
    AimAssist_Turn_Inner_Far_Radius     = AimAssist_Turn_Inner_Far_Radius_Tweak.GetF32();

    AimAssist_Turn_Outer_Near_Dist      = AimAssist_Turn_Outer_Near_Dist_Tweak.GetF32();
    AimAssist_Turn_Outer_Far_Dist       = AimAssist_Turn_Outer_Far_Dist_Tweak.GetF32();
    AimAssist_Turn_Outer_Near_Radius    = AimAssist_Turn_Outer_Near_Radius_Tweak.GetF32();
    AimAssist_Turn_Outer_Far_Radius     = AimAssist_Turn_Outer_Far_Radius_Tweak.GetF32();

    AimAssist_Turn_Damp_Near_Dist       = AimAssist_Turn_Damp_Near_Dist_Tweak.GetF32();
    AimAssist_Turn_Damp_Far_Dist        = AimAssist_Turn_Damp_Far_Dist_Tweak.GetF32();
*/
}

player::player(ObjectManager* pObjectManager, collision_mgr* pCollisionMgr)
    : actor(pObjectManager)
    , m_Loco(pObjectManager, pCollisionMgr)
    , m_Physics(pObjectManager, pCollisionMgr)
    , m_RespawnPosition(0, 0, 0)
    , m_RespawnZone(0)
    , m_ThirdPersonCameraGuid(0)
    , m_PitchAccelTime(3.f)
    , m_YawAccelTime(3.f)
    , m_PitchRate(0.0f)
    , m_YawRate(0.0f)
    , m_PitchAccelFactor(0.0f)
    , m_YawAccelFactor(0.0f)
    , m_PitchMax(R_87)
    , m_PitchMin(-R_87)
    , m_DesiredPitchMax(R_87)
    , m_DesiredPitchMin(-R_87)
    , m_fYawStickSensitivity(1.05f)
    , m_fPitchStickSensitivity(1.0f)
    , m_fOriginalYawStickSensitivity(1.0f)
    , m_fOriginalPitchStickSensitivity(1.0f)
    , m_EyesOffset(0.0f, -25.0f, -5.0f)
    , m_ShakeTime(0.0f)
    , m_ShakeAngle(0.0f)
    , m_ShakeAmount(0)
    , m_ShakeSpeed(0)
    , m_ActivePlayerPad(-1)
    , m_LocalSlot(-1)
    , m_MaxFowardVelocity(100.0f)
    , m_JumpVelocity(200.0f)
    , m_ForwardVelocity(0.0f, 0.0f, 0.0f)
    , m_StrafeVelocity(0.0f, 0.0f, 0.0f)
    , m_fForwardAccel(1000.0f)
    , m_fStrafeAccel(1000.0f)
    , m_fForwardSpeed(0.0f)
    , m_fCurrentYawOffset(0.0f)
    , m_fCurrentPitchOffset(0.0f)
    , m_fPitchChangeSpeed(PI * .45f)
    , m_fPrevForwardSpeed(0.0f)
    , m_fStrafeSpeed(0.0f)
    , m_fDecelerationFactor(2.5f)
    , m_SoftLeanAmount(0.0f)
    , m_LeanWeaponOffset(0.0f, 0.0f, 0.0f)
    , m_PitchArmsScalerPositive(1.0f)
    , m_PitchArmsScalerNegative(1.0f)
    , m_fCurrentCrouchFactor(0.0f)
    , m_fCrouchChangeRate(10.0f)
    , m_bInTurret(false)
    , m_pPlayerTitle(nullptr)
    , m_fMinWalkSpeed(0.0f)
    , m_fMinRunSpeed(0.0f)
    , m_NonExclusiveStateBitFlag(NE_STATE_NULL)
    , m_PreStunPitch(0)
    , m_PreStunYaw(0)
    , m_PreStunRoll(0)
    , m_fStunnedTime(0)
    , m_fShakeAmpScalar(1.f)
    , m_fShakeFreqScalar(1.f)
    , m_fShakeMaxPitch(5.f)
    , m_fShakeMaxYaw(2.f)
    , m_bAllLoreObjectsCollected(false)
    , m_ReticleMovementDegrade(0.5f)
    , m_InvalidSoundTimer(0.0f)
    , m_fRigMoveOffsetVelocity(10.0f)
    , m_fRigStrafeOffsetVelocity(20.0f)
    , m_fCurrentMoveRigOffset(0.0f)
    , m_fCurrentStrafeRigOffset(0.0f)
    , m_RigLookOffset(Radian3(0.0f, 0.0f, 0.0f))
    , m_RigLookMaxVertOffset(R_2)
    , m_RigLookMaxHorozOffset(R_3)
    , m_RigLookVertVelocity(R_10)
    , m_RigLookHorozVelocity(R_20)
    , m_CurrentVertRigOffset(R_0)
    , m_CurrentHorozRigOffset(R_0)
    , m_fCurrentPitchAimModifier(1.f)
    , m_fCurrentYawAimModifier(1.f)
    , m_fFineTuneThreshold(0.4f)
    , m_fYawValueAtFineTuneThreshold(0.15f)
    , m_fPitchValueAtFineTuneThreshold(0.15f)
    , m_fMidRangeThreshold(0.99f)
    , m_fYawValueAtMidrangeThreshold(0.6f)
    , m_fPitchValueAtMidrangeThreshold(0.6f)
    , m_YawAimOffset(R_0)
    , m_TimeSinceLastZonePain(0.0f)
    , m_fMoveValue(0.0f)
    , m_fStrafeValue(0.0f)
    , m_fYawValue(0.0f)
    , m_fPitchValue(0.0f)
    , m_fRawControllerYaw(0)
    , m_fRawControllerPitch(0)
    , m_bVoteButtonPressed(false)
    , m_bRespawnButtonPressed(false)
    , m_fPreviousYawValue(0)
    , m_fPreviousPitchValue(0)
    , m_LastTimeSeenByEnemy(0.0f)
    , m_LastTimeTookDamage(0.0f)
    , m_PositionOfLastSafeSpot(0, 0, 0)
    , m_ZoneIDOfLastSafeSpot(0)
    , m_NextPositionOfLastSafeSpot(0, 0, 0)
    , m_NextZoneIDOfLastSafeSpot(0)
    , m_AimDegradation(0.0f)
    , m_AimRecoverSpeed(0.0f)
    , m_YawMod(0.0f)
    , m_PitchMod(0.0f)
    , m_RollMod(0.0f)
    , m_ShakePitch(0.0f)
    , m_ShakeYaw(0.0f)
    ,
    //m_NearbyObjectCounter(0),
    //m_GameSpeakCounter(0),
    m_SpeakToGuid(0)
    , m_GameSpeakEmitterGuid(0)
    , m_ProximityAlertRadius(300.0f)
    , m_TimeStartTick(0)
    , m_ViewCinematicPlaying(false)
    , m_CurrentViewNode(0)
    , m_StartView(0, 0, 0, 1)
    , m_DesiredView(0, 0, 0, 1)
    , m_CScale(1.0f)
    , m_CTimeSum(0.0f)
    , m_bSpeaking(false)
    , m_LastLadderGuid(0)
    , m_JumpedOffLadderGuid(0)
    , m_WeaponState(WEAPON_STATE_NONE)
    , m_ReticleRadius(0.0f)
    , m_ReticleGrowSpeed(0.0f)
    , m_ArmsOffset(0.0f, 0.0f, 0.0f)
    , m_ArmsVelocity(0.0f, 0.0f, 0.0f)
    , m_WeaponCollisionOffset(0.0f, 0.0f, 0.0f)
    , m_LastWeaponCollisionOffsetScalar(0.0f)
    , m_WeaponCollisionOffsetScalar(0.0f)
    , m_FlashlightGuid(0)
    , m_BatteryChangeTime(0.0f)
    , m_Battery(100.0f)
    , m_MaxBattery(100.0f)
    , m_FlashlightTimeout(0.0f)
    , m_fLastItemFullTime(0.0f)
    , m_fLastItemAcquiredTime(0.0f)
    , m_bStrainInitialized(false)
    , m_CurrentAnimState(ANIM_STATE_UNDEFINED)
    , m_PreviousAnimState(ANIM_STATE_UNDEFINED)
    , m_NextAnimState(ANIM_STATE_UNDEFINED)
    , m_AnimStage(ANIM_STATE_UNDEFINED)
    , m_MeleeAnimStateIndex(-1)
    , m_CurrentAnimIndex(-1)
    , m_PreviousAnimIndex(-1)
    , m_CurrentAnimStateIndex(-1)
    , m_PreviousAnimStateIndex(-1)
    , m_fAnimationTime(0.0f)
    , m_fMaxAnimTime(0.0f)
    , m_WpnHoldTime(0.0f)
    , m_LastTimeWeaponFired(0.0f)
    , m_bOnLadder(false)
    , m_LadderOutDir(0, 0, 0)
    , m_MaxAnimWeaponHoldTime(20.0f)
    , m_nLoreDiscoveries(0)
    , m_DebounceTime(0.0f)
    , m_bWasMutated(false)
    , m_bIsMutantVisionOn(false)
    , m_PreMutationWeapon2(INVEN_NULL)
    , m_bMutationMeleeEnabled(false)
    , m_bPrimaryMutationFireEnabled(false)
    , m_bSecondaryMutationFireEnabled(false)
    , m_bMeleeLunging(false)
    , m_bHolsterWeapon(false)
    , m_MeleeDamage(120.0f)
    , m_MeleeForce(30.0f)
    , m_bInMutationTutorial(false)
    , m_ConvulsionFeedbackDuration(0.5f)
    , m_ConvulsionFeedbackIntensity(1.0f)
    , m_bHitCombo(false)
    , m_bCanRequestCombo(false)
    , m_bLastMeleeHit(false)
    , m_ComboCount(0)
    ,

    m_bTweakHandlesLoaded(false)
    ,

    // From ghost.cpp
    m_Mutagen(100.0f)
    , m_MaxMutagen(100.0f)
    , m_EyesPosition(0.0f, 0.0f, 0.0f)
    , m_EyesPitch(0.0f)
    , m_EyesYaw(0.0f)
    ,
    //m_SuckingMutagenLoopID              ( 0 ),

    m_PrevWeaponItem(INVEN_NULL)
    , m_NextWeaponItem(INVEN_NULL)
    , m_bJustLanded(false)
    , m_DeltaPos(0.0f, 0.0f, 0.0f)
    , m_bCanJump(true)
    , m_DeltaTime(0.0f)
    , m_TimeInState(0.0f)
    , m_MissionFailedTableName(-1)
    , m_MissionFailedReasonName(-1)
    , m_VoteMode(false)
    , m_DelayTillNextStep(0.0f)
    , m_DistanceTraveled(0.0f)
    , m_DelayCountDown(0.0f)
    , m_HeelID(0)
    , m_SlideID(0)
    , m_ToeID(0)
    , m_TrailStep(0)
    , m_MutationChangeTime(0.0f)
    , m_UseTime(0.0f)
{
    int i;

    // SB: Initialize all cinema vars to zero
    memset(&m_Cinema, 0, sizeof(m_Cinema));

    SetIsActive(true);

    InitializeMeleeAnimStateList();

    m_MaxStrafeVelocity = .75f * m_MaxFowardVelocity;

    m_vRigOffset.set(0.0f, 0.0f, 0.0f);

    // The title for this player
    m_pPlayerTitle = "Unknown Mutation";

    m_PeakLandVelocity = -1.0f;
    m_PeakJumpVelocity = -1.0f;

    // Get the players ear id.
    //m_AudioEarID = g_AudioMgr.CreateEar();

    m_RespawnZone = 0;

    m_SpeakToGuid = 0;
    m_CurrentTargetingModifation.Zero();
    m_OffsetToTarget.Zero();

    m_bActivePlayer = true;

    m_fRigMaxMoveOffset = 2.0f;
    m_fRigMaxStrafeOffset = 3.0f;

    m_bHidePlayerArms = false;
    m_bArmsWereHidden = false;
    m_bPlaySwitchTo = true;

    // cinema stuff
    m_Cinema.m_bCinemaOn = false;
    m_Cinema.m_bPlayerZoneInitialized = false;
    m_Cinema.m_LookAtTargetGuid = 0;
    m_Cinema.m_BlendInTime = 3.0f;
    m_Cinema.m_CurrentBlendInTime = 0.0f;

    m_iCameraBone = -1;
    m_iCameraTargetBone = -1;

    // Initialize our arrays.
    memset(m_fAnimPriorityPercentage, 0, sizeof(float) * MAX_ANIM_PER_STATE);

    m_StrainFriendFlags = 0;

    m_CurrentGrenadeType2 = INVEN_GRENADE_FRAG;
    m_PlayMeleeSound = true;
    m_IsRunning = false;
    m_MutationAudioLoopSfx = 0;
    m_NeedRelaodIn = true;
    m_LastFireAnimStateIndex = 0;
    m_bUsingFlashlight = false;
    m_bUsingFlashlightBeforeCinema = false;

    // Reset all weapon anim tables
    for (i = 0; i < INVEN_NUM_WEAPONS; i++) {
        inven_item WeaponItem = inventory2::WeaponIndexToItem(i);
        // TODO: ResetWeaponAnimTable2( WeaponItem );
    }

    // Clear the last pain event.
    m_LastPainEvent.clear();

    m_CurrentAnimState = ANIM_STATE_UNDEFINED;

    // on initialize, the JBG is not in expert mode
    m_bJBGLoreAcquired = false;

    // initialize tap fire time
    m_TapRefireTime = 0.0f;

    // the first time, don't let it "double fire"
    m_bCanTapFire = false;

    // Initalize the FlyBys!
    for (i = 0; i < MAX_FLY_BYS; i++) {
        memset((void*)&m_BulletFlyBy[i], 0, sizeof(m_BulletFlyBy[i]));
    }

    //LOG_MESSAGE( "player::player", "Addr:%08X", this );

    //==========================================================================
    // Begin code from ghost.cpp
    //==========================================================================

    m_Faction = FACTION_PLAYER_NORMAL;
    m_FriendFlags = FACTION_WORKERS;

    // Start out DEAD.  Then spawn.
    m_Health.Dead();
    m_bDead = true;
    m_bWantToSpawn = true;

    // Setup pointer to loco for base class to use.
    m_pLoco = &m_Loco;
    m_pLoco->SetGhostMode(true); // Player controls movement, not animations

    //==========================================================================
    // End code from ghost.cpp
    //==========================================================================

    m_Turret.TurretGuid = 0;
    m_Turret.Turret2Guid = 0;
    m_Turret.Turret3Guid = 0;
    m_Turret.PreviousWeapon = INVEN_WEAPON_SMP;
    m_Turret.AnchorL2W.Identity();

    for (i = 0; i < MAX_LORE_ITEMS; i++) {
        m_LoreObjectGuids[i] = 0;
    }

    // KSS -- new cinema code
    {
        m_Cinema.m_ViewCorrectionDelta.Zero();
        m_Cinema.m_CinemaCameraGuid = 0;
        m_Cinema.m_bUseViewCorrection = false;
    }

    //-- Mission Failer code
    {
        // m_MissionFailedBmp.SetName( PRELOAD_FILE("UI_Mission_failed.xbmp") );
    }
}

player::strain_control_modifiers::strain_control_modifiers()
    : m_StrainProximityAlertRadius(300.0f)
    , m_StrainMaxFowardVelocity(600.f)
    , m_StrainMaxStrafeVelocity(450.f)
    , m_StrainJumpVelocity(500.f)
    , m_StrainMaxHealth(100.f)
    ,
    //  m_StrainStickSensitivity( 1.0f ),
    m_StrainMinWalkSpeed(200.0f)
    , m_StrainMinRunSpeed(400.0f)
    , m_StrainDecelerationFactor(2.5f)
    , m_StrainCrouchChangeRate(10.0f)
    , m_StrainReticleMovementDegrade(0.5f)
    , m_fStrainForwardAccel(3000.0f)
    , m_fStrainStrafeAccel(2000.0f)
    , m_fStrainYawSensitivity(5.5f)
    , m_fStrainPitchSensitivity(1.5f)
    , m_StrainYawAccelTime(1.5f)
    , m_StrainPitchAccelTime(0.2f)
{
    m_StrainEyesOffset.set(0, -25, -20);
}

player::~player()
{
    SetIsActive(false);

    // Destroy flashlight
    Object* pFlashlight = m_FlashlightGuid != 0 ? objectManager->GetObjectByGuid(m_FlashlightGuid) : nullptr;

    if (pFlashlight != nullptr) {
        objectManager->DestroyObjectEx(m_FlashlightGuid, true);
    }

    m_FlashlightGuid = 0;

    // Remove the player's ear from the audio manager.
    //g_AudioMgr.DestroyEar( m_AudioEarID );
}

void player::OnInit()
{
    if (UsingLoco()) {
        InitLoco();
    }

    m_Physics.Init(GetGuid());
    m_Physics.SetSolveActorCollisions(true);

    // Call base
    actor::OnInit();

    //
    // Initialize EVERYTHING
    //

    // character physics init
    m_Physics.SetColHeight(180.0f);
    m_Physics.SetColRadius(30.0f);
    m_Physics.SetColCrouchOffset(70.0f);
    m_Physics.SetHandlePermeable(true);

    if (UsingLoco()) {
        // loco MOVE_STYLE_WALK
        loco::move_style_info_default Defaults;
        Defaults.m_IdleBlendTime = 0.2f;
        Defaults.m_MoveBlendTime = 0.2f;
        Defaults.m_FromPlayAnimBlendTime = 0.2f;
        Defaults.m_MoveTurnRate = R_180;
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_WALK, Defaults);

        // loco MOVE_STYLE_RUN, MOVE_STYLE_RUN_AIM, MOVE_STYLE_CHARGE
        Defaults.m_IdleBlendTime = 0.125f;
        Defaults.m_MoveBlendTime = 0.125f;
        Defaults.m_FromPlayAnimBlendTime = 0.125f;
        Defaults.m_MoveTurnRate = R_360;
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_RUN, Defaults);
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_RUNAIM, Defaults);
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_CHARGE, Defaults);

        // loco MOVE_STYLE_PROWL, MOVE_STYLE_CROUCH, MOVE_STYLE_CROUCHAIM
        Defaults.m_IdleBlendTime = 0.2f;
        Defaults.m_MoveBlendTime = 0.2f;
        Defaults.m_FromPlayAnimBlendTime = 0.2f;
        Defaults.m_MoveTurnRate = R_90;
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_PROWL, Defaults);
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_CROUCH, Defaults);
        m_Loco.SetMoveStyleDefaults(loco::MOVE_STYLE_CROUCHAIM, Defaults);
    }

    // Fall damage
    tweak_handle SafeFallTweak(xfs("%s_MinFallDistToTakeDamage", GetLogicalName()));
    tweak_handle DeathFallTweak(xfs("%s_MaxFallDistToTakeDamage", GetLogicalName()));
    m_SafeFallAltitude = SafeFallTweak.GetF32();
    m_DeathFallAltitude = DeathFallTweak.GetF32();

    // Blood Decals
    m_hBloodDecalPackage.setName("Blood.decalpkg");
    m_BloodDecalGroup = 0;

    // Mutant vision
    m_bAllowedToGlow = true;
    m_FriendlyGlowColor.set(50, 255, 0, 255);
    m_EnemyGlowColor.set(255, 50, 0, 255);

    // player stuff
    m_ViewInfo.XFOV = R_60;
    m_OriginalViewInfo.XFOV = m_ViewInfo.XFOV;
    m_nLoreDiscoveries = 0;
    m_PitchArmsScalerPositive = 1.0f;
    m_PitchArmsScalerNegative = 1.0f;
    m_bCanDie = true;

    // Avatar (only if we're not in split screen)
    if (UsingLoco()) {
        PrepPlayerAvatar();
    }

    // Arms
    {
        m_Skin.SetUpSkinGeom("FP_PLR_Human_BIND.skingeom");
        m_Skin.SetVMeshMask(0);
        m_Skin.SetVMeshBit("MESH_Arms_Hazmat", true);
        m_Skin.SetVMeshBit("MESH_Hands_Hazmat", true);

        // WARNING:
        // It may be some problem here. The resource handles can't start counting references
        // untill a name has been assign to them. Not only that but when a new name is set it
        // must make sure that the old name is decremented reference wise.
        m_AnimGroup.setName("FP_PlayerArms.anim");

        // Make sure that this are clear not matter what
        m_iCameraBone = -1;
        m_iCameraTargetBone = -1;

        // If we can load this anim group then we need to extract some info
        if (m_AnimGroup.getPointer()) {
            OnAnimationInit();
        }
    }

    m_MaxStunPitchOffset = R_1;
    m_MaxStunYawOffset = R_8;
    m_MaxStunRollOffset = R_3;
    m_fStunYawChangeSpeed = 0.5f;
    m_fStunPitchChangeSpeed = 0.1f;
    m_fStunRollChangeSpeed = 0.2f;
    m_fMaxStunTime = 5.0f;

    m_StrainFriendFlags = 0;
    m_StrainFriendFlags |= FACTION_PLAYER_NORMAL;
    m_StrainFriendFlags |= FACTION_NEUTRAL;
    m_StrainFriendFlags |= FACTION_MILITARY;
    m_StrainFriendFlags |= FACTION_WORKERS;

    m_bMutationMeleeEnabled = false;
    m_bPrimaryMutationFireEnabled = false;
    m_bSecondaryMutationFireEnabled = false;

    m_bInTurret = false;

    // load all the Lore Object guids for us
    LoadAllLoreObjects();
}

float player::GetMovementNoiseLevel()
{
    float MaxVelocity = GetMaxVelocity();

    if (MaxVelocity == 0.0f) {
        return MaxVelocity;
    } else {
        return GetCurrentVelocity() / MaxVelocity;
    }
}

//=========================================================================

void player::ResetView()
{
    m_ViewInfo.XFOV = m_OriginalViewInfo.XFOV;
}

//=========================================================================

void player::UpdateZoneTrack()
{
    // Must use view position for horizontally flat portals.
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, GetEyesPosition());
}

//=========================================================================

zone_mgr::zone_id player::GetPlayerObjectZone() const
{
    // Use zone tracker zone
    return m_ZoneTracker.GetMainZone();
}

//=========================================================================

zone_mgr::zone_id player::GetPlayerViewZone() const
{
    // SB: 2/1/2005

    // NOTE: For the next project, the cinema process REALLY needs to be fixed.
    // The player should not be dragged around to the camera (unless the cinema
    // requires this), and this function SHOULD return the camera's zone for ALL
    // cinemas.

    // Also, the fading, letter box etc should be automatic and come from the logic
    // in the cinema object. Right now it's done with triggers and the property system
    // (very ugly!).

    // DS: 2/2/2005
    // NOTE: The third-person death camera is similar to a cinema and so
    // we should be using the camera's zone and NOT the player's zone for
    // rendering. BLECH...our zone and portal system is much too error-prone
    // as seen from this example, Steve's note above, and the numerous data
    // problems coming from art and design.
    third_person_camera* pThirdPersonCam = GetThirdPersonCamera();
    if (pThirdPersonCam) {
        return (zone_mgr::zone_id)pThirdPersonCam->GetZone1();
    }

    // Use object zone
    return GetPlayerObjectZone();
}

//=========================================================================

void player::UpdateSafeSpot(float DeltaTime)
{
    const float  timeBetweenSafeSpotChecks = 10.0f;
    static float timeSinceSafeSpotCheck = 0.0f;

    timeSinceSafeSpotCheck += DeltaTime;
    if (timeSinceSafeSpotCheck > timeBetweenSafeSpotChecks) {
        timeSinceSafeSpotCheck = 0.0f;

        //  If we currently feel safe
        if (GetIsSafeSpot()) {
            SetCurrentSpotAsSafeSpot();
        }
    }
}

//=========================================================================

void player::OnReset()
{
    m_Health.Reset();

    OnMove(m_RespawnPosition);

    // Zone change, must update the tracker zone.
    UpdateZone(m_RespawnZone);
}

//==============================================================================
HudObject* player::GetHud()
{
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_HUD_OBJECT);

    if (SlotID != SLOT_NULL) {
        Object* pObj = objectManager->GetObjectBySlot(SlotID);

        // for some reason the object isn't valid
        if (pObj) {
            // get the HUD from the object
            return &HudObject::GetSafeType(*pObj);
        }
    }

    // failed
    return nullptr;
}

void player::ShakeView(float Time, float Amount, float Speed)
{
    m_ShakeTime = Time;
    m_ShakeAngle = 0.0f;
    m_ShakeAmount = Amount;
    m_ShakeSpeed = Speed;
}

//==============================================================================

bool player::InvalidSound()
{
    if (m_InvalidSoundTimer <= 0.0f) {
        // IJB g_AudioMgr.Play("Klaxon_01_Shot", true);
        m_InvalidSoundTimer = 3.0f;
        return true;
    }

    return false;
}

//==============================================================================
void player::UpdateCameraShake(float DeltaTime)
{
    // Calculate shake using square fall off - the shake is bigger and faster at
    // the start, and gets slowers as it dampens to zero
    float Freq = x_sqr(DEG_TO_RAD((std::min(1.0f, m_ShakeTime) * m_fShakeFreqScalar * 360.0f) * m_ShakeSpeed));

    // Apply shake
    m_ShakeAngle += Freq * DeltaTime;

    m_ShakeAngle = fmod(m_ShakeAngle, 360.0f);

    // Update shake time
    m_ShakeTime = std::max(0.0f, m_ShakeTime - DeltaTime);

    //apply view modification from pain force
    if (m_PitchMod > 0) {
        m_PitchMod = std::max(0.0f, std::min(m_PitchMod - (R_40 * DeltaTime), R_20));
    } else if (m_PitchMod < 0) {
        m_PitchMod = std::min(0.0f, std::max(m_PitchMod + (R_40 * DeltaTime), -R_20));
    }

    if (m_YawMod > 0) {
        m_YawMod = std::max(0.0f, std::min(m_YawMod - (R_40 * DeltaTime), R_20));
    } else if (m_YawMod < 0) {
        m_YawMod = std::min(0.0f, std::max(m_YawMod + (R_40 * DeltaTime), -R_20));
    }

    if (m_RollMod > 0) {
        m_RollMod = std::max(0.0f, std::min(m_RollMod - (R_40 * DeltaTime), R_20));
    } else if (m_RollMod < 0) {
        m_RollMod = std::min(0.0f, std::max(m_RollMod + (R_40 * DeltaTime), -R_20));
    }
}

void player::ClearPainEvent()
{
    m_LastPainEvent.clear();
}

//static float PLAYER_FORCE_VEL = 600.0f;
static float PLAYER_FORCE_RUMBLE_DURATION = 0.25f;
static float PLAYER_FORCE_RUMBLE_INTENSITY = 1.0f;
static float PLAYER_FORCE_SHAKE = 0.5f;
static float PLAYER_FORCE_BLUR = 1.0f;
static float PLAYER_FORCE_ROTATE = 0.05f;

//==============================================================================

void player::DoBasicPainFeedback(float Force)
{
    // Shake the camera
    ShakeView(PLAYER_FORCE_SHAKE * Force);

    //force feedback
    DoFeedback(PLAYER_FORCE_RUMBLE_DURATION * Force, PLAYER_FORCE_RUMBLE_INTENSITY * Force);

    // Start up the shaky-blur pain post-effect
    float EffectForce = PLAYER_FORCE_BLUR * Force;
    /* IJB
    g_PostEffectMgr.StartPainBlur(GetLocalSlot(),
                                  std::min(EffectForce * 20.0f, 20.0f),
                                  Colour((uint8_t)std::min(255.0f, 200 + (EffectForce * 55.0f)), 128, 128, (uint8_t)std::min(180.0f, 100 + (EffectForce * 80.0f))));
*/
}

//==============================================================================

void player::RespondToPain(const pain& Pain)
{

    // Get force and damage from pain
    float Force = Pain.GetForce();

    //
    // Do additional effects
    //
    DoBasicPainFeedback(Force);

    //
    // Rotate player based on pain direction
    //
    if (1) {
        //mess with pitch and yaw
        Radian rPainAngleYaw = v3_AngleBetween(Vector3(0, m_EyesYaw), Pain.GetDirection());
        Radian rPainAnglePitch = v3_AngleBetween(Vector3(m_EyesPitch, 0), Pain.GetDirection());

        Vector3 PlayerFaceDir(0, 0, 1);
        PlayerFaceDir.RotateY(m_Yaw);
        plane Plane(Vector3(0, 0, 0), PlayerFaceDir, Vector3(0, 1, 0));

        if (Plane.InFront(Pain.GetDirection())) {
            rPainAngleYaw = rPainAngleYaw;
        } else {
            rPainAngleYaw = -rPainAngleYaw;
        }

        float RotateForce = Force * PLAYER_FORCE_ROTATE;

        m_YawMod -= rPainAngleYaw * RotateForce;
        m_PitchMod -= rPainAnglePitch * RotateForce;
        m_RollMod -= rPainAngleYaw * RotateForce;
    }
}

//==============================================================================

actor::eHitType player::OverrideFlinchType(actor::eHitType hitType)
{
    // Players 3rd person avatar can only play light (additive) hits
    switch (hitType) {
    case HITTYPE_HARD:
    case HITTYPE_LIGHT:
    case HITTYPE_IDLE:
    case HITTYPE_PLAYER_MELEE_1:
        return HITTYPE_LIGHT;
        assert(false); //( 0, "Need to add new hit type here..." );
    default:
        return hitType;
    }
}

//==============================================================================

void player::OnPain(const pain& Pain)
{
    // Player cannot take damage while viewing a cinematic..
    if (m_ViewCinematicPlaying) {
        return;
    }

    // I have no idea what the line above does.  Is that code even used?
    // Let's check the real cinema system...
    if (m_Cinema.m_bCinemaOn) {
        return;
    }

    // If you are already dead, no pain!
    if (m_bDead) {
        return;
    }

    // If the same pain event as the last one, ignore it.
    if ((Pain.GetAnimEventID() != -1) &&
        (Pain.GetAnimEventID() == m_LastAnimPainID)) {
        return;
    }

    // if we are neutral, we ignore pain
    if (m_SpawnNeutralTime > 0.0f) {
        return;
    }

    // If we didn't create the pain, then early-out if it's friendly
    if ((Pain.GetOriginGuid() != GetGuid()) && IsFriendlyFaction(GetFactionForGuid(Pain.GetOriginGuid()))) {
        return;
    }

    std::string StringPrefix = (const char*)xfs("%s", GetLogicalName());

    // Modify damage on mutated players.
    if (m_bIsMutated) {
        StringPrefix += "_MUTANT";
    }

    // Decide which health id to use
    StringPrefix += "_B";

    // turn into string pointer
    const char* pString = StringPrefix.c_str();

    // Decide which health id to use
    health_handle HealthHandle(pString);

    // Resolve Pain
    if (!Pain.ComputeDamageAndForce(HealthHandle, GetGuid(), GetBBox().GetCenter())) {
        return;
    }

    //
    // Apply the damage
    //
    TakeDamage(Pain);

    // If this is not the active player, then it needs to become it
    // Is this needed anymore !?!
    /*
    if (!m_bActivePlayer) {
        assert(false);
        player* pPlayer = SMP_UTIL_GetActivePlayer();
        if (pPlayer) {
            objectManager->DestroyObject(pPlayer->GetGuid());
        }
        SetAsActivePlayer(true);
    }
*/
    // Record the last pain event and time.
    m_LastPainEvent.push_back(Pain);

    // Do shakes and pushes and rumbles
    RespondToPain(Pain);
}

//=============================================================================

void player::BackUpCurrentState()
{
    OnCopy(m_SaveSpotProperties);
}
//=============================================================================

void player::RestoreState()
{
    if (m_SaveSpotProperties.size() > 0) {
        OnPaste(m_SaveSpotProperties);
    }
}

//----------------------------------------------------------------------------------------------------------------
//
void player::ParseOnPainForEffects(const pain& Pain)
{
    // Skip?
    if (!IsBloodEnabled()) {
        return;
    }

    // Create blood impact if blood decals are assigned
    const decal_package* pBloodDecalPackage = m_hBloodDecalPackage.getPointer();
    if (pBloodDecalPackage) {
        // Create blood based on pain type and use color of assigned blood decal group
        /* IJB
        particle_emitter::CreateOnPainEffect(Pain,
                                             k_PainParticleDisplace,
                                             particle_emitter::UNINITIALIZED_PARTICLE,
                                             pBloodDecalPackage->GetGroupColor(m_BloodDecalGroup));
                                            */
    }
}

//===========================================================================

bool player::GetIsSafeSpot()
{
    const float k_MIN_TIME_NOT_SPOTTED_TO_CONSIDER_SAFE = 10.0f;
    const float k_MIN_TIME_NOT_INJURED_TO_CONSIDER_SAFE = 10.0f;
    float       currentTime = (float)x_GetTimeSec();

    if ((currentTime - m_LastTimeSeenByEnemy) > k_MIN_TIME_NOT_SPOTTED_TO_CONSIDER_SAFE &&
        (currentTime - m_LastTimeTookDamage) > k_MIN_TIME_NOT_INJURED_TO_CONSIDER_SAFE) {
        return true;
    } else {
        return false;
    }
}

//===========================================================================

void player::SetCurrentSpotAsSafeSpot()
{
    m_PositionOfLastSafeSpot = m_NextPositionOfLastSafeSpot;
    m_ZoneIDOfLastSafeSpot = m_NextZoneIDOfLastSafeSpot;
    m_NextPositionOfLastSafeSpot = GetPosition();
    m_NextZoneIDOfLastSafeSpot = GetPlayerObjectZone();
}

//===========================================================================

void player::ResetToLastSafeSpot()
{
    if (m_PositionOfLastSafeSpot.LengthSquared() != 0.0f) {
        OnMove(m_PositionOfLastSafeSpot);
        UpdateZone(m_ZoneIDOfLastSafeSpot);
    } else {
        OnMove(m_RespawnPosition);
        UpdateZone(m_RespawnZone);
    }
}

//===========================================================================
/* IJB
void player::PushViewCinematic(lock_view_node* pLockViewBuffer)
{
    for (int i = 0; i < lock_player_view::MAX_TABLE_SIZE; i++) {
        m_LockViewTable[i] = pLockViewBuffer[i];
    }

    m_ViewCinematicPlaying = true;
    m_CurrentViewNode = 0;
    m_TimeStartTick = objectManager->GetGameTime();

    m_StartView.identity();
    m_DesiredView.identity();

    m_StartView.Setup(Radian3(m_Pitch, m_Yaw, 0.0f));

    lock_view_node* rNode = &m_LockViewTable[0];

    //by convention -1 is an end node..
    if (rNode->m_TimeTo <= 0.0f) {
        m_ViewCinematicPlaying = false;
        return;
    }

    Vector3 LookAtVector = rNode->m_LookAt - GetEyesPosition();

    m_DesiredView.Setup(Radian3(LookAtVector.GetPitch(), LookAtVector.GetYaw(), 0.0f));

    m_fYawValue = 0.0f;
    m_fPitchValue = 0.0f;

    m_CScale = 1.0f / (rNode->m_TimeTo * k_Modfactor);
    m_CTimeSum = 0.0f;

    m_ForwardVelocity.Zero();
    m_StrafeVelocity.Zero();
}
*/

void player::UpdateViewCinematic(const float& rDeltaTime)
{
    //play through the cinematic

    assert(m_CurrentViewNode < lock_player_view::MAX_TABLE_SIZE && m_CurrentViewNode >= 0);

    lock_view_node* rNode = &m_LockViewTable[m_CurrentViewNode];

    float TimeTotal = objectManager->GetGameDeltaTime(m_TimeStartTick);

    if (rNode->m_TimeTo + rNode->m_Linger < TimeTotal) {
        m_CurrentViewNode++;

        if (m_CurrentViewNode >= lock_player_view::MAX_TABLE_SIZE) {
            m_ViewCinematicPlaying = false;
            //            g_Hud.PlayCinematic( false );
            return;
        } else {
            rNode = &m_LockViewTable[m_CurrentViewNode];
        }

        if (rNode->m_TimeTo <= 0.0f) {
            m_ViewCinematicPlaying = false;
            //            g_Hud.PlayCinematic( false );
            return;
        }

        m_CScale = 1.0f / (rNode->m_TimeTo * k_Modfactor);
        m_CTimeSum = 0.0f;

        m_StartView = m_DesiredView;

        m_TimeStartTick = objectManager->GetGameTime();

        TimeTotal = objectManager->GetGameDeltaTime(m_TimeStartTick);

        Vector3 LookAtVector = rNode->m_LookAt - GetEyesPosition();

        m_DesiredView.Setup(Radian3(LookAtVector.GetPitch(), LookAtVector.GetYaw(), 0.0f));
    }

    assert(rNode->m_TimeTo > 0.0f);

    if (rNode->m_TimeTo > TimeTotal) {
        float T = (TimeTotal / rNode->m_TimeTo);

        float Tquad = -(T * T - T) * m_CScale;

        m_CTimeSum += Tquad;

        if (m_CTimeSum > 0.0f && m_CTimeSum < 1.0f) {
            Quaternion BlendView = Blend(m_StartView, m_DesiredView, m_CTimeSum);
            Radian3    BlendViewDir = BlendView.GetRotation();

            m_Pitch = BlendViewDir.pitch;
            m_Yaw = BlendViewDir.yaw;
        } else if (m_CTimeSum > 1.0f) {
            Radian3 ViewDir = m_DesiredView.GetRotation();

            m_Pitch = ViewDir.pitch;
            m_Yaw = ViewDir.yaw;
        }
    } else {
        Radian3 ViewDir = m_DesiredView.GetRotation();

        m_Pitch = ViewDir.pitch;
        m_Yaw = ViewDir.yaw;
    }
}

//----------------------------------------------------------------------------------------------------------------

void player::ResetPlayer(const Vector3& rPos, const Radian3& rViewRot)
{
    // Reset position
    OnMove(rPos);

    // Reset rotation
    m_Pitch = rViewRot.pitch;
    m_Yaw = rViewRot.yaw;

    // Reset zone tracking
    g_ZoneMgr.InitZoneTracking(*this, m_ZoneTracker);
}

//=============================================================================
//=============================================================================
//  Checks for a LOS to an object
//=============================================================================

bool player::CanSeeObject(Object* pObject)
{
    assert(pObject);
    g_CollisionMgr.LineOfSightSetup(GetGuid(),                       // MovingObjGuid,
                                    GetEyesPosition(),               // WorldStart,
                                    pObject->GetBBox().GetCenter()); // WorldEnd,
    g_CollisionMgr.AddToIgnoreList(pObject->GetGuid());
    g_CollisionMgr.IgnoreGlass();

    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PLAYER_LOS, (Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));
    if (g_CollisionMgr.m_nCollisions) {
        return false;
    }

    return true;
}

//===========================================================================

void player::OnMoveFreeCam(view& View)
{
    // View's position
    Vector3 vPos = View.GetPosition();

    // Update zone tracking
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, vPos);

    // Cast a couple rays:
    // 0 = down, 1 = up, 2 = left, 3 = right, 4 = front, 5 = back
    Vector3 vDir[6];

    vDir[0].set(0.0f, -1500.0f, 0.0f);
    vDir[1].set(0.0f, 500.0f, 0.0f);

    for (int i = 2; i < 6; i++) {
        vDir[i].set(0.0f, 0.0f, 500.0f);
        vDir[i].RotateY((i - 2) * R_90);
    }

    for (int i = 5; i >= 0; i--) {
        g_CollisionMgr.RaySetup(GetGuid(), vPos, vDir[i] + vPos);
        g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_COLLIDABLE, (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));

        if (g_CollisionMgr.m_nCollisions > 0) {
            guid    HitGuid = g_CollisionMgr.m_Collisions[0].ObjectHitGuid;
            Object* pObject = objectManager->GetObjectByGuid(HitGuid);
            if (pObject == nullptr) {
                continue;
            }

            // if the object that we hit isn't in the zone that we're in, we need to make the
            // object's zone the active one
            if (pObject->GetZone1() != GetZone1()) {
                SetZone1(pObject->GetZone1());
            }
        }
    }

    // print out the zone info
    int ZoneID = GetZone1();
    //x_printfxy(1, 1, "FlyModeZone=%d", ZoneID);
}

//===========================================================================

extern view g_View;

void player::OnExitFreeCam(Vector3& NewPos)
{
    // CJ: This just ensures the position is where it wants to go and avoids any collisions
    // or other logic on the way by calling the object move function before calling the
    // regular move function
    Object::OnMove(NewPos);
    OnMove(NewPos);
    m_Physics.SetPosition(NewPos);

    // Grab the current view orientation, clear any death states

    g_View.GetPitchYaw(m_Pitch, m_Yaw);
    SetAnimState(ANIM_STATE_IDLE);
    EndDeath();
    GetView().SetRotation(Radian3(m_Pitch, m_Yaw, R_0));
}

//===========================================================================
void player::DoFeedback(float Duration, float Intensity)
{
    /* IJB
    // don't do feedback if we're dead
    if (m_ActivePlayerPad != -1 && !IsDead()) {
        input_Feedback(Duration, Intensity, g_IngamePad[m_ActivePlayerPad].GetControllerID());
    }
        */
}

void player::OnKill()
{
    actor::OnKill();

    // free our slot
    if (m_LocalSlot != -1) {
        m_LocalSlot = -1;
    }
}

//===========================================================================
static float       s_ArmViewPct = 0.3f;
static const float s_MaxCameraDelta = 28.0f;
bool               g_UseOldCameraDefaultPos = false;
Vector3            player::GetDefaultViewPos()
{
    Vector3 FinalPos(0.0f, 0.0f, 0.0f);
    if (m_iCameraBone > -1) {
        if (g_UseOldCameraDefaultPos) {
            FinalPos = m_vRigOffset + m_AnimPlayer.GetBonePosition(m_iCameraBone) - (m_ArmsOffset * s_ArmViewPct) - m_LeanWeaponOffset;

            FinalPos += m_Cinema.m_ViewCorrectionDelta;
        } else
        // KSS -- new cinema code
        {
            Vector3 AnimBonePos = m_AnimPlayer.GetBonePosition(m_iCameraBone);

            Vector3 Offset = m_vRigOffset - (m_ArmsOffset * s_ArmViewPct);
            FinalPos = AnimBonePos + Offset;
            FinalPos += m_Cinema.m_ViewCorrectionDelta;
        }

        // CJ: This wacky code is here to prevent the maximum XZ plane translation of the camera
        // from the vertical center of the player exceeding the s_MaxCameraDelta variable
        // This stops the camera crashing through surfaces as the players head tilts back, it's
        // not an ideal solution but it was the easiest to implement.

        // Construct the plane to limit the camera motion behind the player
        Vector3 Forward(Radian3(0.0f, m_Yaw, 0.0f));
        Vector3 Pos(GetPosition() - s_MaxCameraDelta * Forward);
        plane   p;
        p.Setup(Pos, Forward);

        // Is camera behind the plane, if so move it to the plane
        if (p.InBack(FinalPos)) {
            float d = p.Distance(FinalPos);
            FinalPos -= p.Normal * d;
        }
    } else {
        Vector3 Position(GetPosition());
        Vector3 EyesOffSet(m_EyesOffset);
        EyesOffSet.RotateY(m_Yaw);

        FinalPos.set(Position.x, GetBBox().max.y, Position.z);
        FinalPos += EyesOffSet;
    }

    return FinalPos;
}

//===========================================================================

inline void player::ComputeStunnedPitchYawOffset(Radian PitchOffset, Radian YawOffset)
{
    float YawRotFactor = m_fStunnedTime * m_fStunYawChangeSpeed;
    float PitchRotFactor = m_fStunnedTime * m_fStunPitchChangeSpeed;

    YawOffset = sin(YawRotFactor);
    PitchOffset = sin(PitchRotFactor);

    YawOffset *= m_MaxStunPitchOffset;
    PitchOffset *= m_MaxStunYawOffset;
}

//===========================================================================
static float s_ViewRollTune = 3987.63f;

void player::ComputeView(view& View, view_flags Flags)
{
    third_person_camera* pThirdPersonCamera = GetThirdPersonCamera();
    if (pThirdPersonCamera) {
        pThirdPersonCamera->ComputeView(View);
    } else {
        (void)Flags;
        View.SetXFOV(m_ViewInfo.XFOV);

        // SB: Using player letter box cinema?
        if (m_Cinema.m_bCinemaOn == true) {
            // Using a camera object?
            Object* pCamera = objectManager->GetObjectByGuid(m_Cinema.m_CinemaCameraGuid);
            if (pCamera) {
                // Use exact cinema camera info
                View.SetV2W(m_Cinema.m_CameraV2W);
                View.SetXFOV(m_Cinema.m_CameraXFOV);

                // Update the eyes so that zone tracking works
                m_EyesPosition = View.GetPosition();

                // Get out of here so position, rotation and field-of-view are WYSIWYG!
                return;
            }
        }

        Radian3 Rot(0.0f, 0.0f, 0.0f);
        Vector3 Pos(GetDefaultViewPos());

        // Use camera bone?
        if ((m_CurrentWeaponItem != INVEN_NULL) && (m_iCameraBone != -1)) {
            // Get camera rotation in engine world space
            Rot = m_AnimPlayer.GetBoneL2W(m_iCameraBone).GetRotation();

            Rot.yaw += R_180;
            Rot.pitch = -Rot.pitch;
            Rot.roll = -Rot.roll;

            // Counter act the rig offset that has already been applied to the anim player L2W
            // (which is a parent of the camera bone) otherwise it won't come through since the camera
            // is also moved by the rig offset along with the arms.
            Rot.yaw -= m_CurrentHorozRigOffset;
            Rot.pitch -= m_CurrentVertRigOffset;

        } else {
            // Use current pitch and yaw
            Rot.Set(m_Pitch, m_Yaw, -DEG_TO_RAD(GetTweakF32("LeanMaxDegrees") * m_SoftLeanAmount));
        }

        //
        // Ask the current weapon how far zoomed we are
        //
        if ((m_CurrentAnimState != ANIM_STATE_FALLING_TO_DEATH) && (IsAlive())) {
            /* IJB
            new_weapon* pWeapon = GetCurrentWeaponPtr();

            if (pWeapon) {
                if (pWeapon->IsZoomEnabled()) {
                    View.SetXFOV(pWeapon->GetXFOV());
                    m_ViewInfo.XFOV = View.GetXFOV();
                } else {
                    m_ViewInfo.XFOV = m_OriginalViewInfo.XFOV;
                }
            }
            */
        }

        //
        // If we're not dead, respond to pain and stuns
        //
        if ((m_CurrentAnimState != ANIM_STATE_DEATH) &&
            (m_CurrentAnimState != ANIM_STATE_FALLING_TO_DEATH)) {
            if (m_NonExclusiveStateBitFlag & NE_STATE_STUNNED) {
                Radian PitchOffset = 0.0f;
                Radian YawOffset = 0.0f;
                ComputeStunnedPitchYawOffset(PitchOffset, YawOffset);
                Rot.Set(m_PreStunPitch + PitchOffset, m_PreStunYaw + YawOffset, 0.0f);
            }

            // Apply camera shake
            float Amp = x_sqr(std::min(1.0f, m_ShakeTime * m_fShakeAmpScalar));
            m_ShakePitch = (DEG_TO_RAD(m_fShakeMaxPitch) * Amp * sin(m_ShakeAngle * 0.981f)) * m_ShakeAmount;
            m_ShakeYaw = (DEG_TO_RAD(m_fShakeMaxYaw) * Amp * cos(m_ShakeAngle * 1.375f)) * m_ShakeAmount;
            Rot.pitch -= m_ShakePitch;
            Rot.yaw -= m_ShakeYaw;

            // Apply pain force
            Rot.pitch += m_PitchMod;
            Rot.yaw += m_YawMod;

            // Apply movement roll
            if (m_DeltaTime > 0.0f) {
                Radian  RollAmount = ((m_StrafeVelocity.Length() / m_DeltaTime) / s_ViewRollTune) * R_1;
                Vector3 Forward;
                Forward.set(Radian3(0.0f, Rot.yaw, 0.0f));
                if (m_StrafeVelocity.Cross(Forward).y < 0) {
                    RollAmount = -RollAmount;
                }

                Rot.roll += RollAmount;
            }
        }

        View.SetPosition(Pos);
        View.SetRotation(Rot);

        m_EyesPosition = View.GetPosition();
        View.GetPitchYaw(m_EyesPitch, m_EyesYaw);

        UpdateZoneTrack();
    }

    // Now make sure the player's near and far planes match the level
    // settings near and far planes
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_LEVEL_SETTINGS);
    if (SlotID != SLOT_NULL) {
        Object* pObj = objectManager->GetObjectBySlot(SlotID);
        /* IJB
        level_settings& LevelSettings = level_settings::GetSafeType(*pObj);
        View.SetZLimits(10.0f, LevelSettings.GetFarPlane());
        */
    } else {
        View.SetZLimits(10.0f, 8000.0f);
    }
}

//===========================================================================

void player::InitializeMeleeAnimStateList(void)
{
    // set index to invalid so GetNextMeleeState() will be setup correctly
    m_MeleeAnimStateIndex = -1;

    // initialize list
    for (int i = 0; i < MAX_MELEE_STATES; i++) {
        m_MeleeAnimStates[i] = animation_state(ANIM_STATE_MELEE + 1 + i);
    }

    // randomize it initially
    RandomizeMeleeAnimStateList();
}

//===========================================================================

void player::RandomizeMeleeAnimStateList()
{
    int             j = 0;
    animation_state temp_AnimState;
    int             maxStates = MAX_MELEE_STATES - 1;

    // save off last anim state in list so we can make sure it's not the top one in the new list
    animation_state LastAnimState = m_MeleeAnimStates[maxStates];
    for (int i = 0; i < MAX_MELEE_STATES; i++) {
        j = x_irand(0, maxStates);
        temp_AnimState = m_MeleeAnimStates[i];
        m_MeleeAnimStates[i] = m_MeleeAnimStates[j];
        m_MeleeAnimStates[j] = temp_AnimState;
    }

    // oops the last anim in the old list is at the top of the new list, switch.
    if (LastAnimState == m_MeleeAnimStates[0]) {
        // put it at the end again (doing this because we at least know the list is as big as 0 -> maxStates).
        temp_AnimState = m_MeleeAnimStates[0];
        m_MeleeAnimStates[0] = m_MeleeAnimStates[maxStates];
        m_MeleeAnimStates[maxStates] = temp_AnimState;
    }
}

//=============================================================================
player::animation_state player::GetNextMeleeState()
{
    m_MeleeAnimStateIndex++;
    if (m_MeleeAnimStateIndex >= MAX_MELEE_STATES) {
        RandomizeMeleeAnimStateList();
        m_MeleeAnimStateIndex = 0;
    }

    assert(m_MeleeAnimStateIndex < MAX_MELEE_STATES && m_MeleeAnimStateIndex >= 0);

    return m_MeleeAnimStates[m_MeleeAnimStateIndex];
}

//=============================================================================
third_person_camera* player::GetThirdPersonCamera() const
{
    return ((third_person_camera*)objectManager->GetObjectByGuid(m_ThirdPersonCameraGuid));
}

//=============================================================================
void player::SetupThirdPersonCamera()
{
    // Do not create a 3rd person camera for network ghosts.
    if (m_LocalSlot == -1) {
        return;
    }

    if (!GetThirdPersonCamera()) {
        guid Guid = objectManager->CreateObject("Third Person Camera");
        assert(Guid);

        m_ThirdPersonCameraGuid = Guid;
        assert(GetThirdPersonCamera());
    }

    view&   View = GetView();
    Radian  Pitch;
    Radian  Yaw;
    Vector3 CamTarget = View.GetPosition();

    View.GetPitchYaw(Pitch, Yaw);

    //
    //  If we can, give the 3rd person cam a hint
    //  indicating what direction the killing pain
    //  came from.
    //
    const corpse_pain& ThePain = GetCorpseDeathPain();

    Vector3 DirTowardSource(0, 0, 1);

    if (ThePain.GetOriginGuid()) {
        // If we know the object, let's aim at it
        Object* pObj = objectManager->GetObjectByGuid(ThePain.GetOriginGuid());
        if (pObj) {
            Vector3 Pos = pObj->GetPosition();
            DirTowardSource = Pos - GetPosition();
        }
    } else {
        if (ThePain.IsDirectHit()) {
            // Otherwise, try to aim back along the pain dir
            DirTowardSource = -ThePain.GetDirection();
        }
    }

    DirTowardSource.Normalize();

    third_person_camera* pCam = GetThirdPersonCamera();

    pCam->SetOrbitPoint(CamTarget);
    pCam->Setup(CamTarget, DirTowardSource, DeathCamStartBackDist, DeathCamEndDist, this);
}

//=============================================================================

void player::UpdateThirdPersonCamera()
{
    if (GetThirdPersonCamera()) {
        object_ptr<corpse> pCorpse(m_CorpseGuid, objectManager);
        if (pCorpse) {
            /* IJB
            physics_inst& PhysicsInst = pCorpse->GetPhysicsInst();
            if (PhysicsInst.GetNRigidBodies() > 0) {
                const rigid_body& RigidBody = PhysicsInst.GetRigidBody(0);
                const Vector3     Pos(RigidBody.GetWorldBBox().GetCenter());
                GetThirdPersonCamera()->SetOrbitPoint(Pos);
            } else {
                GetThirdPersonCamera()->SetOrbitPoint(GetBBox().GetCenter());
            }
                */
        } else {
            // There should never be anything interesting at the players bbox location.
            // The corpse is where the fun is.  The camera is initialized based on
            // the players bbox, and the corpse will conveniently be there also.
            // Once things get rolling, the corpse can end up somewhere entirely
            // different than the player.  Let the camera follow the corpse, and when
            // we no longer have one, leave the camera alone.
            //GetThirdPersonCamera()->SetOrbitPoint( GetBBox().GetCenter() );
        }
    }
}

//=============================================================================

void player::SetLocalPlayer(int LocalIndex)
{
    assert(IN_RANGE(0, LocalIndex, MAX_LOCAL_PLAYERS - 1));
    assert(m_LocalSlot <= 0);

    m_LocalSlot = LocalIndex;

    m_ActivePlayerPad = LocalIndex;

    // always controller 0.
    /* IJB
    g_IngamePad[LocalIndex].SetControllerID(0);

    assert(g_IngamePad[LocalIndex].GetControllerID() != -1);

    g_IngamePad[LocalIndex].EnableContext(INGAME_CONTEXT);
    */
}

//=============================================================================

void player::Push(const Vector3& PushVector)
{
    if (!m_bInTurret) {
        m_Physics.SetPosition(GetPosition());
        m_Physics.Push(PushVector);
        OnMove(m_Physics.GetPosition());
    }
}

void player::UpdateWeaponPullback()
{
    if (m_CurrentAnimState == ANIM_STATE_DEATH) {
        m_WeaponCollisionOffset.Zero();
    } else {
        //
        // Check to see if the gun will collide with something. If so, move
        // it back using m_WeaponCollisionOffset.
        //
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon && (pWeapon->GetType() != TYPE_WEAPON_MUTATION)) {
            Vector3       FirePos;
            Vector3       SingleCollisionOffset(0.0f, 0.0f, 0.0f);
            Vector3       LeftCollisionOffset(0.0f, 0.0f, 0.0f);
            Vector3       RightCollisionOffset(0.0f, 0.0f, 0.0f);
            float         SingleCollisionScalar = 0.0f;
            float         LeftCollisionScalar = 0.0f;
            float         RightCollisionScalar = 0.0f;
            const Vector3 ToWeapon(m_EyesPosition - pWeapon->GetPosition());

            if (pWeapon->GetFiringBonePosition(FirePos, new_weapon::FIRE_POINT_DEFAULT)) {
                FirePos += ToWeapon;
                SingleCollisionOffset = GetWeaponCollisionOffset(pWeapon->GetGuid(), FirePos);
                SingleCollisionScalar = m_WeaponCollisionOffsetScalar;
            }

            if (pWeapon->GetFiringBonePosition(FirePos, new_weapon::FIRE_POINT_LEFT)) {
                FirePos += ToWeapon;
                LeftCollisionOffset = GetWeaponCollisionOffset(pWeapon->GetGuid(), FirePos);
                LeftCollisionScalar = m_WeaponCollisionOffsetScalar;
            }

            if (pWeapon->GetFiringBonePosition(FirePos, new_weapon::FIRE_POINT_RIGHT)) {
                FirePos += ToWeapon;
                RightCollisionOffset = GetWeaponCollisionOffset(pWeapon->GetGuid(), FirePos);
                RightCollisionScalar = m_WeaponCollisionOffsetScalar;
            }

            if (SingleCollisionScalar > LeftCollisionScalar) {
                if (SingleCollisionScalar > RightCollisionScalar) {
                    m_WeaponCollisionOffset = SingleCollisionOffset;
                    m_WeaponCollisionOffsetScalar = SingleCollisionScalar;
                } else {
                    m_WeaponCollisionOffset = RightCollisionOffset;
                    m_WeaponCollisionOffsetScalar = RightCollisionScalar;
                }
            } else {
                if (LeftCollisionScalar > RightCollisionScalar) {
                    m_WeaponCollisionOffset = LeftCollisionOffset;
                    m_WeaponCollisionOffsetScalar = LeftCollisionScalar;
                } else {
                    m_WeaponCollisionOffset = RightCollisionOffset;
                    m_WeaponCollisionOffsetScalar = RightCollisionScalar;
                }
            }
        } else {
            m_WeaponCollisionOffset.Zero();
        }
    }
    m_LastWeaponCollisionOffsetScalar = m_WeaponCollisionOffsetScalar;
}

//=============================================================================

// this adds in the arms offset and keeps the weapon
void player::MoveAnimPlayer(const Vector3& Pos)
{
    m_AnimPlayer.SetPosition(Pos + GetAnimPlayerOffset());
}

//=============================================================================
void player::SetCurrentStrain()
{
    if (m_bStrainInitialized == false) {

        m_Faction = FACTION_PLAYER_NORMAL;
        // Set all movement / control variables to match this strain.
        strain_control_modifiers& StrainControl = m_StrainControls;

        // Move all of the data from the strain control structure to the places where it will get used.
        m_fCrouchChangeRate = StrainControl.m_StrainCrouchChangeRate;
        m_ProximityAlertRadius = StrainControl.m_StrainProximityAlertRadius;
        m_MaxFowardVelocity = StrainControl.m_StrainMaxFowardVelocity;
        m_MaxStrafeVelocity = StrainControl.m_StrainMaxStrafeVelocity;
        m_JumpVelocity = StrainControl.m_StrainJumpVelocity;
        m_MaxHealth = StrainControl.m_StrainMaxHealth;
        m_EyesOffset = StrainControl.m_StrainEyesOffset;
        m_fYawStickSensitivity = StrainControl.m_fStrainYawSensitivity;
        m_fPitchStickSensitivity = StrainControl.m_fStrainPitchSensitivity;
        m_fMinWalkSpeed = StrainControl.m_StrainMinWalkSpeed;
        m_fMinRunSpeed = StrainControl.m_StrainMinRunSpeed;
        m_fDecelerationFactor = StrainControl.m_StrainDecelerationFactor;
        m_ReticleMovementDegrade = StrainControl.m_StrainReticleMovementDegrade;
        m_fForwardAccel = StrainControl.m_fStrainForwardAccel;
        m_fStrafeAccel = StrainControl.m_fStrainStrafeAccel;
        m_YawAccelTime = StrainControl.m_StrainYawAccelTime;
        m_PitchAccelTime = StrainControl.m_StrainPitchAccelTime;

        m_FriendFlags = m_StrainFriendFlags;

        // Finish necessary initializations.
        m_fOriginalPitchStickSensitivity = m_fPitchStickSensitivity;
        m_fOriginalYawStickSensitivity = m_fYawStickSensitivity;
        m_fMinWalkSpeed *= m_fMinWalkSpeed;
        m_fMinRunSpeed *= m_fMinRunSpeed;

        // Setup the physics
        m_Physics.CopyValues(m_Physics);

        //
        // NOTE: Make sure that the CurrentStrain is set before we go set the weapons up  for this strain!!!
        //
        for (int i = 0; i < INVEN_NUM_WEAPONS; i++) {
            new_weapon* pWeapon = GetWeaponPtr(inventory2::WeaponIndexToItem(i));
            if (pWeapon) {
                pWeapon->SetupRenderInformation();
            }
        }
        // Restart the current state so the new animations start.  This will
        // most likely change when we get some anims.
        if (m_bStrainInitialized) {
            ShakeView(1.0f);
        }
    }
}

//===========================================================================
static float s_ArmsVelocityCarryover = 0.5f;
static bool  s_DumpWeapons = false;
float        CINEMA_DELTA_FADE_T = 0.9f;
extern bool  s_bUseTestMap;
extern int   s_ScannerTestMap;

#define MAX_JBG_LORE 2

struct s_JBG_LoreIDs
{
    int MapID;
    int LoreIndex;
};

s_JBG_LoreIDs g_LoreIDs[MAX_JBG_LORE] =
    {
        {1075, 4}, // black 0-2, lore index 4 = IDS_LORE_DESC_45 in C:\GameData\A51\Release\PS2\LoreList.txt
        {1090, 0}  // black 2-0, lore index 0 = IDS_LORE_DESC_51 in C:\GameData\A51\Release\PS2\LoreList.txt
};

void player::OnAdvanceLogic(float DeltaTime)
{

    // set vibration
    /*
        int             iPad = g_IngamePad[m_LocalSlot].GetControllerID();
        player_profile& Profile = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(m_LocalSlot));
        input_EnableFeedback(Profile.m_bVibration, iPad);
    */

    g_ZoneMgr.UpdateEar(m_AudioEarID);

    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    bool bFlung = m_Physics.Flung();

    DeltaTime *= g_PerceptionMgr.GetPlayerTimeDialation();
    m_DeltaTime = DeltaTime;

    if (IsChangingMutation()) {
        m_MutationChangeTime = 0.0f;
    } else {
        m_MutationChangeTime += DeltaTime;
    }

    m_UseTime += DeltaTime;

    if (s_DumpWeapons) {
        s_DumpWeapons = 0;
    }

    // KSS -- new cinema code
    if (m_Cinema.m_bUseViewCorrection) {
        m_Cinema.m_ViewCorrectionDelta *= CINEMA_DELTA_FADE_T;
    }

    // Store a pointer to the current weapon.  This pointer is only valid
    // this frame and can only be used in my methods, not engine overloads.
    // Must be very careful with this.
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // Check to see if our current weapon is still in our inventory
    if (pWeapon && !m_Inventory2.HasItem(m_CurrentWeaponItem)) {
        // We no longer have the weapon, drop it
        pWeapon = nullptr;
    }

    // SB - Added to keep the weapon in sync with the player hands
    //      (player::OnAdvanceLogic now updates the weapon)
    if ((pWeapon) && (pWeapon->GetRenderState() != new_weapon::RENDER_STATE_PLAYER)) {
        pWeapon->OnAdvanceLogic(DeltaTime);
    }

    actor::OnAdvanceLogic(DeltaTime);

    UpdateConvulsion();

    // see if we have the JBG lore item
    if (!GetJBGLoreAcquired()) {
        /* IJB
        player_profile& Profile = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(0));
        int             VaultIndex, i;

        for (i = 0; i < MAX_JBG_LORE; i++) {
            if (s_bUseTestMap) {
                // For testing
                g_LoreList.GetVaultByMapID(s_ScannerTestMap, VaultIndex);
            } else {
                g_LoreList.GetVaultByMapID(g_LoreIDs[i].MapID, VaultIndex);
            }

            if (Profile.GetLoreAcquired(VaultIndex, g_LoreIDs[i].LoreIndex)) {
                // found a lore object that unlocks expert mode.
                m_bJBGLoreAcquired = true;

                // Get out.
                break;
            }
        }
            */
    }

    // If we have turned on cinema mode and aren't in the cinema state then switch states
    if ((m_Cinema.m_bCinemaOn == true) && (m_CurrentAnimState != ANIM_STATE_CINEMA)) {
        if (m_CurrentAnimState != ANIM_STATE_MISSION_FAILED) {
            if (!IsChangingMutation()) {
                SetAnimState(ANIM_STATE_CINEMA);
            }
        } else {
            BeginCinema();                           // Get the hud and stuff set up
            SetAnimState(ANIM_STATE_MISSION_FAILED); // Don't lose our state
            m_Cinema.m_bCinemaOn = false;            // Avoid doing this next time
        }
    }

    if (m_bHidePlayerArms) {
        // arms weren't hidden but now they are, make sure we clear zoom
        if (!m_bArmsWereHidden) {
            // we have a weapon and our arms
            if (GetCurrentWeaponPtr()) {
                GetCurrentWeaponPtr()->ClearZoom();

                // make sure we give it a proper state
                SetAnimState(ANIM_STATE_IDLE);
                m_NextAnimState = ANIM_STATE_UNDEFINED;
            }
        }

        // be sure we set this so we know the previous state of the arms being hidden
        m_bArmsWereHidden = true;
    } else {
        // the arms WERE hidden and now they're not, play switch to animation.
        if (m_bArmsWereHidden && m_bPlaySwitchTo) {
            SetAnimState(ANIM_STATE_SWITCH_TO);
            m_bArmsWereHidden = false;
        }
    }

    pWeapon = nullptr;

    // We need to update the airborn state even if we don't have an avatar.
    SetIsAirborn(m_Physics.IsAirborn());

    if (m_Physics.GetFallMode()) {
        m_bFalling = true; // used for pain when we land
    } else if (m_bFalling) {
        // We've just landed
        m_bJustLanded = true;
        // Hurt?
        if (!bFlung) {
            TakeFallPain();
        }

        m_bFalling = false;
    }

    //make sure the jumpguid get's cleared if we aren't falling.
    if (!m_bFalling) {
        m_JumpedOffLadderGuid = 0;
    }

    // Handled by player
    WakeUpDoors();
    m_InvalidSoundTimer = std::max(0.0f, m_InvalidSoundTimer - DeltaTime);

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================

    bool bJumpingBeanFirst = false;

    // LevelID 1030 = Blue 1-2.  While in this map the JBG takes priority even though it's non-expert mode
    // IJB bJumpingBeanFirst = (g_ActiveConfig.GetLevelID() == 1030);

    // we have expert mode, JBG goes first in order
    if (GetJBGLoreAcquired()) {
        bJumpingBeanFirst = true;
    }

    bool bOutOfFrag = (m_Inventory2.GetAmount(INVEN_GRENADE_FRAG) <= 0);

    // make sure we use the "jumping beans" first
    if ((m_Inventory2.GetAmount(INVEN_GRENADE_JBEAN) > 0) && (bJumpingBeanFirst || bOutOfFrag)) {
        m_CurrentGrenadeType2 = INVEN_GRENADE_JBEAN;
    } else {
        m_CurrentGrenadeType2 = INVEN_GRENADE_FRAG;
    }

    UpdateSafeSpot(DeltaTime);
    UpdateUserInput(DeltaTime);
    UpdateBulletSounds(DeltaTime);

    // recharge/burn our mutagen if we have the mutation ability
    if (m_Inventory2.HasItem(INVEN_WEAPON_MUTATION)) {
        UpdateMutagen(DeltaTime);
    }

    // drain/charge flashlight battery
    UpdateFlashlightBattery(DeltaTime);

    pWeapon = GetCurrentWeaponPtr();
    if (m_bIsMutated && !m_bWasMutated) {
        SetMutated(true);
    }

    m_bWasMutated = m_bIsMutated;

    UpdateState(DeltaTime);

    m_DebounceTime += DeltaTime;

    if (s_WeaponInventoryStrip) {
        RemoveAllWeaponInventory();
        s_WeaponInventoryStrip = false;
    }

    // First advance initializes the strain.
    if (!m_bStrainInitialized) {
        SetCurrentStrain();
        m_bStrainInitialized = true;
    }

    UpdateAudio(DeltaTime);
    GatherGameSpeakGuid();

    UpdateActiveNonExclusiveStates(DeltaTime);

    // SB: Update ghost loco for split-screen / network games
    //     NOTE: The location of this is crucial (after player position and weapon switching
    //           have been processed, before weapon is positioned) to keep the avatars weapons
    //           and animation in sync. If you need to move it, please come talk to me first!
    //           Thanks.
    if (!m_bDead && UsingLoco()) {
        //
        // Update loco (NOTE: Loco position will get updated from actor::OnMove)
        //
        m_pLoco->SetGhostIsMoving(m_Physics.GetVelocity().LengthSquared() > x_sqr(0.01f));
        m_pLoco->SetPitch(m_Pitch);
        m_pLoco->SetYaw(m_Yaw);
        OnAdvanceGhostLogic(DeltaTime);
    }

    // Advance the animation for the arms.
    if (m_AnimGroup.getPointer()) {
        Vector3 Pos;
        m_AnimPlayer.Advance(DeltaTime, Pos);

        // Store a pointer to the current weapon.  This pointer is only valid
        // this frame and can only be used in my methods, not engine overloads.
        // Must be very careful with this.
        if (pWeapon) {
            // Update weapon here so muzzle fx is in sync!
            AttachWeapon();

            // run logic
            pWeapon->OnAdvanceLogic(DeltaTime);
        }
    }

    //handle the animation events
    OnAnimEvents();

    if (!m_Physics.GetFallMode() && m_bJustLanded) {
        // We've just landed
        // Keep the arms going
        const float DistanceFell = m_FellFromAltitude - GetPosition().y; // we need this before TakeFallPain()

        // SB: In the editor, this can be negative when you first run the game?!
        if (DistanceFell > 0.001f) {
            static const float Gravity = 980.0f;
            const float        TimeFalling = sqrt(DistanceFell / Gravity);
            const float        FallSpeed = Gravity * TimeFalling * s_ArmsVelocityCarryover;
            m_ArmsVelocity += Vector3(0.0f, -FallSpeed, 0.0f);
        }

        m_bJustLanded = false;
    }

    pWeapon = nullptr;

    // Update any effects
    if (m_pEffects) {
        m_pEffects->Update(this, DeltaTime);
    }
}

//==============================================================================
bool player::UseFocusObject()
{
    //
    // Look for all focus objects in the world, and try to press every single
    // one of them until we find one that works.
    //
    /* IJB
    bool UsePressed = (bool)g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_USE).WasValue;
#if defined(TARGET_PC) && !defined(X_EDITOR)
    UsePressed |= input_WasPressed(INPUT_MOUSE_BTN_C);
#endif

    if (UsePressed && (m_CurrentAnimState != ANIM_STATE_THROW) && !IsChangingMutation()) {
        slot_id SlotID = objectManager->GetFirst(Object::TYPE_FOCUS_OBJECT);
        while (SlotID != SLOT_NULL) {
            Object* pObject = objectManager->GetObjectBySlot(SlotID);
            SlotID = objectManager->GetNext(SlotID);

            if (pObject && pObject->IsKindOf(focus_object::GetRTTI())) {
                focus_object* pFocusObject = (focus_object*)pObject;
                if (pFocusObject->TestPress()) {
                    // If we successfully used the FO, then we don't want to
                    // do anything else with this button press.
                    m_UseTime = 0.0f;
                    return true;
                }
            }
        }
    }
        */
    return false;
}

//==============================================================================

bool player::NearMutagenReservoir()
{
    bool RetVal = false;

    objectManager->SelectBBox(ATTR_ALL, GetBBox(), TYPE_MUTAGEN_RESERVOIR);
    slot_id ObjectSlot = SLOT_NULL;
    Object* pObject = nullptr;
    for (ObjectSlot = objectManager->StartLoop();
         ObjectSlot != SLOT_NULL;
         ObjectSlot = objectManager->GetNextResult(ObjectSlot)) {
        pObject = objectManager->GetObjectBySlot(ObjectSlot);
        if (pObject && GetBBox().Intersect(pObject->GetBBox())) {
            RetVal = true;
            break;
        }
    }

    objectManager->EndLoop();

    return RetVal;
}

//==============================================================================
void player::AcquireAllLoreObjects()
{
    // already done
    if (m_bAllLoreObjectsCollected) {
        return;
    }

    // make sure flag is set
    m_bAllLoreObjectsCollected = true;

    // acquire all lore objects
    int i = 0;
    for (i = 0; i < MAX_LORE_ITEMS; i++) {
        lore_object* pLoreObject = (lore_object*)objectManager->GetObjectByGuid(m_LoreObjectGuids[i]);

        // if valid and is still active, get it.
        if (pLoreObject && pLoreObject->IsActivated()) {
            pLoreObject->OnAcquire();
        }
    }
}

//==============================================================================
void player::LoadAllLoreObjects()
{
    int count = 0;

    // load up all the lore objects for collecting and for updating the Geiger counter
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_LORE_OBJECT);
    while (SlotID != SLOT_NULL && count < MAX_LORE_ITEMS) {
        // Lookup lore object
        Object*      pObj = objectManager->GetObjectBySlot(SlotID);
        lore_object* pLoreObject = &lore_object::GetSafeType(*pObj);

        if (!pLoreObject) {
            assert(0);
            continue;
        }

        // true lore object
        if (pLoreObject->IsTrueLoreObject()) {
            int VaultIndex = -1;
            m_LoreObjectGuids[count] = pLoreObject->GetGuid();
            /* IJB
                        if (s_bUseTestMap) {
                            // For testing
                            g_LoreList.GetVaultByMapID(s_ScannerTestMap, VaultIndex);
                        } else {
                            int MapID = g_ActiveConfig.GetLevelID();
                            g_LoreList.GetVaultByMapID(MapID, VaultIndex);
                        }

                        player_profile& Profile = g_StateMgr.GetActiveProfile(g_StateMgr.GetProfileListIndex(0));

                        if (VaultIndex != -1) {
                            // we have this one
                            if (Profile.GetLoreAcquired(VaultIndex, pLoreObject->GetLoreID())) {
                                // silently acquire the lore object.
                                pLoreObject->OnAcquire(true);
                            }
                        }
            */
            count++;
        }

        // Get next lore object
        SlotID = objectManager->GetNext(SlotID);
    }

    m_bAllLoreObjectsCollected = false;
}

//==============================================================================

bool player::GetClosestLoreObjectDist(float& ClosestDist)
{
    if (m_bAllLoreObjectsCollected) {
        return false;
    }

    ClosestDist = 100.0f; // IJB Lore_Max_Detect_DistanceTweak.GetF32() + 100.0f; // 31 meters; 1 meter greater than max distance
    int  i;
    bool bAllCollected = true;
    bool bFound = false;

    for (i = 0; i < MAX_LORE_ITEMS; i++) {
        lore_object* pLoreObject = (lore_object*)objectManager->GetObjectByGuid(m_LoreObjectGuids[i]);
        if (pLoreObject && pLoreObject->IsActivated() && (pLoreObject->GetLoreID() != -1)) {
            // we found an active one
            bAllCollected = false;

            float dist = (GetPosition() - pLoreObject->GetPosition()).Length();

            // is this one closer?
            if (dist < ClosestDist) {
                bFound = true;
                ClosestDist = dist;
            }
        }
    }

    // all are activated
    if (bAllCollected) {
        m_bAllLoreObjectsCollected = true;
    }

    return bFound;
}

//==============================================================================
void player::InitFlashlight(const Vector3& rInitPos)
{
    // create the flashlight (todo--make this part of the weapon blueprint so artists/designers can play.)
    if (m_FlashlightGuid != 0) {
        // already created
        return;
    } else {
        m_FlashlightGuid = objectManager->CreateObject(projector_obj::GetObjectType());
    }

    object_ptr<projector_obj> ProjObj(m_FlashlightGuid, objectManager);

    if (ProjObj.IsValid()) {
        texture::handle Texture;

        // set visuals
        Texture.setName("Flashlight.xbmp");
        ProjObj.m_pObject->SetShadow(false);
        ProjObj.m_pObject->SetActive(false);
        ProjObj.m_pObject->SetIsFlashlight(true);
        ProjObj.m_pObject->SetFOV(R_60);
        ProjObj.m_pObject->SetLength(2000.0f);
        ProjObj.m_pObject->SetTextureHandle(Texture);
        ProjObj.m_pObject->OnMove(rInitPos);
        ProjObj.m_pObject->SetZone1(GetZone1());
        ProjObj.m_pObject->SetZone2(GetZone2());
    } else {
        assert(false); //ASSERTS(0, "Invalid Flashlight");
    }
}

//==============================================================================

bool player::IsFlashlightActive()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // validate weapon
    if (pWeapon) {
        object_ptr<projector_obj> ProjObj(m_FlashlightGuid, objectManager);

        // is the flashlight object valid and is the bone valid?
        if (ProjObj.IsValid() && pWeapon->CheckFlashlightPoint()) {
            return ProjObj.m_pObject->IsActive();
        }
    }

    return false;
}

//==============================================================================

void player::SetFlashlightActive(bool bOn)
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // validate weapon
    if (pWeapon) {
        object_ptr<projector_obj> ProjObj(m_FlashlightGuid, objectManager);

        // is the flashlight object valid and is the bone valid?
        if (ProjObj.IsValid() && pWeapon->CheckFlashlightPoint()) {
            ProjObj.m_pObject->SetActive(bOn);
        }

        MoveFlashlight();

        const bool bWasUsingFlashlight = m_bUsingFlashlight;
        m_bUsingFlashlight = bOn;

        if (bWasUsingFlashlight != m_bUsingFlashlight) {
            actor_effects* pActorEffects = GetActorEffects(true);

            if (m_bUsingFlashlight) {
                if (pActorEffects) {
                    pActorEffects->InitEffect(actor_effects::FX_FLASHLIGHT, this);
                }
            } else {
                if (pActorEffects) {
                    pActorEffects->KillEffect(actor_effects::FX_FLASHLIGHT);
                }
            }
        }

    } else {
        // set using flashlight to false because weapon is invalid
        m_bUsingFlashlight = false;
    }

    // flashlight is off, reset flashlight timeout
    if (m_bUsingFlashlight == false) {
        m_FlashlightTimeout = FlashlightAutoOffSecondsTweak.GetF32();
    }
}

//==============================================================================
void player::UpdateFlashlightBattery(float nDeltaTime)
{
    // KSS -- people hate the flashlight battery, so removing it.
    // battery "conservation" :)
    if (IsFlashlightActive()) {
        // if we are playing a cinematic, don't burn battery
        if (IsCinemaRunning()) {
            if (m_bUsingFlashlight) {
                m_bUsingFlashlightBeforeCinema = true;
                // if the flashlight is active and a cinema is running, turn it off and get out
                SetFlashlightActive(false);
            }

            return;
        }

        uint8_t Brightness = (uint8_t)GetFloorIntensity();

        // if light levels are high, turn off flashlight after FLASHLIGHT_AUTOOFF_TIME seconds
        if (Brightness > FlashlightAutoOffBrightnessTweak.GetF32()) {
            m_FlashlightTimeout -= nDeltaTime;

            if (m_FlashlightTimeout <= FLT_MIN) {
                // this will reset timer as well
                SetFlashlightActive(false);
            }
        } else {
            m_FlashlightTimeout = FlashlightAutoOffSecondsTweak.GetF32();
        }
    }
}
//==============================================================================

bool player::AddBattery(const float& nDeltaBattery)
{
    // do not allow Battery to go above max.
    if (m_Battery == m_MaxBattery && nDeltaBattery > 0.0f) {
        return false;
    } else if ((m_Battery + nDeltaBattery) < 0.0f) // does what we are using take us below 0?
    {
        // don't have enough
        return false;
    } else {
        // add/subtract Battery
        m_Battery = std::min(m_Battery + nDeltaBattery, m_MaxBattery);
        m_Battery = std::max(m_Battery, 0.0f);

        return true;
    }

    return false;
}

//==============================================================================

void player::MoveFlashlight()
{
    if (IsFlashlightActive()) {
        new_weapon* pWeapon = GetCurrentWeaponPtr();

        // validate weapon
        if (pWeapon) {
            object_ptr<projector_obj> ProjObj(m_FlashlightGuid, objectManager);

            // validate flashlight object and flashlight bone
            if (ProjObj.IsValid() && pWeapon->CheckFlashlightPoint()) {
                Matrix4 L2W;
                Vector3 Vect;

                // transform if we are in the proper state
                if (pWeapon->GetFlashlightTransformInfo(L2W, Vect)) {
                    L2W.PreTranslate(Vect);
                    L2W.PreRotateY(R_180);
                    L2W.PreTranslate(Vector3(0.0f, 0.0f, -100.0f));
                    ProjObj.m_pObject->OnTransform(L2W);

                    // set flashlight zones to the player's zones
                    ProjObj.m_pObject->SetZones(GetZones());
                }
            } else {
                // kill flashlight if we don't have a weapon
                ProjObj.m_pObject->SetActive(false);
            }
        }
    }
}

//==============================================================================

void player::OnDeath()
{
#ifndef X_EDITOR
    //  LOG_MESSAGE( "player::OnDeath", "Slot:%d", m_NetSlot );
#endif

    new_weapon* pWeapon = GetCurrentWeaponPtr();

    // get rid of our weapon (mostly for multiplayer)
    if (pWeapon) {
        // Make sure to stop any looping weapon sfx
        pWeapon->ReleaseAudio();

        // end zoom
        pWeapon->ClearZoom();

        // shut off flashlight
        SetFlashlightActive(false);

        // no moving
        ClearStickInput();

        // clear weapon
        SetAnimState(ANIM_STATE_IDLE);
        m_NextAnimState = ANIM_STATE_UNDEFINED;
    }

    actor::OnDeath();

    // Set this so the state mgr knows what is going on.
    s_bPlayerDied = true;

    SetAnimState(ANIM_STATE_DEATH);

    // tell perception manager we died
    if (IsMutated()) {
        g_PerceptionMgr.EndMutate();
    }

    ClearAllNonExclusiveStates();

    if (m_bInTurret) {
        // make sure we don't go flying
        GetLocoPointer()->m_Physics.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
    }

    m_bInTurret = false;

    // reset lore flag.  It will get set to true again if all are actually collected (self-fixing).
    m_bAllLoreObjectsCollected = false;

    m_LeanState = LEAN_NONE;
    m_SoftLeanAmount = 0.0f;
    m_LeanWeaponOffset.Zero();
}

//==============================================================================

void player::OnMissionFailed(int TableName, int ReasonName)
{
    m_MissionFailedTableName = TableName;
    m_MissionFailedReasonName = ReasonName;

    // We need to die, then go into ANIM_STATE_MISSION_FAILED
    OnDeath();
    SetAnimState(ANIM_STATE_MISSION_FAILED);
}

//==============================================================================

void player::OnSpawn()
{
    actor::OnSpawn();

    // initialize the zone tracker to the player position
    g_ZoneMgr.InitZoneTracking(*this, m_ZoneTracker);

    // Activate the sound emitters.
    /* IJB
    extern void ActivateSoundEmitters( const Vector3& Position );
    ActivateSoundEmitters( GetPosition() );
*/
    // Reset the state.
    SetAnimState(ANIM_STATE_IDLE);

    // refill mutagen
    AddMutagen(GetMaxMutagen());
}

//=============================================================================
extern bool  g_ShowLoreObjectCollision;
extern float g_LO_SphereSize;
extern float g_LO_RenderDist;
extern float g_Dist;

void player::OnRenderTransparent(void)
{
    actor::OnRenderTransparent();
    /* IJB
    #ifndef X_EDITOR
        if (m_CurrentAnimState == ANIM_STATE_MISSION_FAILED) {

            if (x_GetLocale() == XL_LANG_ENGLISH) {
                xbitmap* pBitmap = m_MissionFailedBmp.GetPointer();
                if (pBitmap) {
                    draw_Begin(DRAW_SPRITES, DRAW_TEXTURED | DRAW_USE_ALPHA | DRAW_2D | DRAW_NO_ZBUFFER);
                    draw_SetTexture(*pBitmap);
                    draw_DisableBilinear();
                    float X = float(256 - (m_MissionFailedBmp.GetPointer()->GetWidth() / 2));
                    float Y = 100.0f;
                    draw_Sprite(
                        Vector3(X, Y, 0.0f),
                        vector2((float)m_MissionFailedBmp.GetPointer()->GetWidth(), (float)m_MissionFailedBmp.GetPointer()->GetHeight()),
                        g_HudColor);
                    draw_End();
                }
            } else {
                // display a localized text message instead of the bitmap
                irect Rect(0, 100, 512, 130);
                RenderLine((xwchar*)g_StringTableMgr("ui", "IDS_MISSION_FAILED"), Rect, 255, g_HudColor, 0, ui_font::h_center | ui_font::v_top);
            }

            const int mfx = 0;
            const int mfy = 180;
            Colour    Color(XCOLOR_RED);
            irect     Rect;
            Rect.Set(mfx, mfy + 50, 512, mfy + 51);
            Color.Set(XCOLOR_YELLOW);
            RenderLine((xwchar*)g_StringTableMgr(g_StringMgr.GetString(m_MissionFailedTableName), g_StringMgr.GetString(m_MissionFailedReasonName)), Rect, 255, Color, 0, ui_font::h_center | ui_font::v_top);
        }
    #endif
    */
    //render weapon
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        // set the proper render state so we can refrain from drawing the 1st person muzzle fx
        if (IsAvatar() && pWeapon->IsUsingSplitScreen()) {
            pWeapon->SetRenderState(new_weapon::RENDER_STATE_NPC);
        }

        pWeapon->OnRenderTransparent();

        // put renderstate back like it was
        pWeapon->SetRenderState(new_weapon::RENDER_STATE_PLAYER);
    }
}

//===========================================================================

void player::OnRender()
{

    UpdateWeaponPullback();

    if (GetThirdPersonCamera() && IsDead()) {
        return;
    }

    if (!m_bIsMutated) {
        if (m_Inventory2.GetAmount(INVEN_GLOVES) > 0.0f) {
            // set virtual mesh for gloves
            m_Skin.SetVMeshBit("MESH_Arms_Hazmat", true);
            m_Skin.SetVMeshBit("MESH_Hands_Bare", false);
            m_Skin.SetVMeshBit("MESH_Hands_Hazmat", true);

        } else {
            // set virtual mesh for no gloves
            m_Skin.SetVMeshBit("MESH_Arms_Hazmat", true);
            m_Skin.SetVMeshBit("MESH_Hands_Bare", true);
            m_Skin.SetVMeshBit("MESH_Hands_Hazmat", false);
        }
    }

#if defined(X_EDITOR)
    if (g_ShowPlayerPos) {
        Vector3 Pos = GetPosition();
        x_printfxy(1, 2, "Player( %7.1f, %7.1f, %7.1f )", Pos.GetX(), Pos.y, Pos.GetZ());
    }
#endif

    // We need to render debug stuff at least, if sniper zoom is enabled
    RenderAimAssistDebugInfo();

    if (RenderSniperZoom() ||               // if we're in sniper mode, don't render player arms and such
        m_Cinema.m_bCinemaOn ||             // if we are playing a cinematic, don't draw arms
        (m_bHidePlayerArms && !IsAvatar())) // Has a trigger or something turned off our arms?
    {
        // KSS -- FIXME -- HACK -- This will cause sniper zoom on moving platforms to now work.
        // PREVIOUSLY, you would get locked in and were not able to YAW at all.
        const Matrix4& mat = GetL2W();
        (void)mat;

        return;
    }

    if (!IsAvatar()) {
        // gather flags and ambient color
        Colour   Ambient;
        uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;
        if (true /* IJB g_RenderContext.m_bIsMutated && !g_RenderContext.m_bIsPipRender && m_bAllowedToGlow */) {
            Flags |= render::GLOWING;

            // TODO: Fill in the logic for determining if this is friend or foe.

            // TODO: This color should come from the blueprint properties (m_EnemyGlowColor)
            Ambient = Colour(255, 200, 200, 255);
        } else {
            // IJB Ambient = GetFloorColor();
        }

        if (!m_bActivePlayer) {
            return;
        }

        if (m_LocalSlot == -1) {
            return;
        }

        void* pPtr1 = m_AnimGroup.getPointer();
        void* pPtr2 = m_Skin.GetSkinGeom();

        // Don't render the player arms if he doesn't have a weapon
        // GaryW -> Commented out GetCurrentWeaponPtr() because it was
        // causing a bug where the player was unable to turn while standing
        // on an Anim Surface.  Bug was added to the BugBase to have the
        // appropriate person find a resultion to this problem.
        if (pPtr1 && pPtr2 && (GetCurrentWeaponPtr() || (m_CurrentAnimState == ANIM_STATE_DEATH)) && !(m_bIsMutated && (m_CurrentAnimState == ANIM_STATE_DEATH))) {
            int            nBones = m_AnimPlayer.GetNBones();
            Matrix4*       pBone = (Matrix4*)malloc(nBones * sizeof(Matrix4));
            const Matrix4* pAnimBone = m_AnimPlayer.GetBoneL2Ws();

            for (int i = 0; i < nBones; i++) {
                pBone[i] = pAnimBone[i];
                pBone[i].Translate(m_WeaponCollisionOffset);
            }

            // Handle fade-in on spawn
            if (m_SpawnFadeTime > 0.0f) {
                Flags |= render::FADING_ALPHA;
                float Alpha = 1.0f - (m_SpawnFadeTime / g_SpawnFadeTime);
                Alpha = std::min(Alpha, 1.0f);
                Alpha = std::max(Alpha, 0.0f);
                Ambient.a = (uint8_t)(Alpha * 255.0f);
            }

            skin_inst& SkinInst = m_Skin;
            SkinInst.Render(&GetL2W(),
                            pBone,
                            nBones,
                            Flags | render::CLIPPED | render::DISABLE_SPOTLIGHT,
                            SkinInst.GetLODMask(GetL2W()),
                            Ambient);
            free(pBone);
        } else if (!GetCurrentWeaponPtr()) {
            // KSS -- FIXME -- HACK -- This will cause no weapon on moving platforms to now work.
            // PREVIOUSLY, you would get locked in and were not able to YAW at all.
            const Matrix4& mat = GetL2W();
            (void)mat;
        }

        if (m_CurrentAnimState == ANIM_STATE_CHANGE_MUTATION && (m_AnimStage > 1) && (m_AnimStage < 3)) //stage 1 is the switch from
        {
            //special case
            return;
        }

        //render weapon
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon) {
            pWeapon->SetRenderState(new_weapon::RENDER_STATE_PLAYER);
            //AttachWeapon();
            pWeapon->RenderWeapon(true, Ambient, false);
        }
    } else {
        actor::OnRender();
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon) {
            pWeapon->SetRenderState(new_weapon::RENDER_STATE_PLAYER);
        }
    }
}

//===========================================================================

void player::OnRenderShadowCast(uint64_t ProjMask)
{
    if (IsAvatar()) {
        actor::OnRenderShadowCast(ProjMask);
    }
}

void player::OnAliveLogic(float DeltaTime)
{
    // Recover aim (double the recovery just to make it faster).
    m_AimDegradation = std::max(0.0f, m_AimDegradation - (m_AimRecoverSpeed * DeltaTime * 2.0f));

    float AbsForwardSpeed = abs(m_fForwardSpeed);
    float AbsStrafeSpeed = abs(m_fStrafeSpeed);

    float ScalerForward = (AbsForwardSpeed / m_MaxFowardVelocity) * m_ReticleMovementDegrade;
    float ScalerStrafe = (AbsStrafeSpeed / m_MaxStrafeVelocity) * m_ReticleMovementDegrade;

    float ShootDegrade = 1.0f - m_ReticleMovementDegrade;
    float AimDegrade = std::min(1.0f, (m_AimDegradation * ShootDegrade) + std::max(ScalerStrafe, ScalerForward));

    if (AimDegrade < 0.0f) {
        AimDegrade = 1.0f;
    }

    float AlteredDeltaTime = DeltaTime;

    if ((m_NonExclusiveStateBitFlag & NE_STATE_STUNNED) != 0) {
        AlteredDeltaTime *= 0.3f;
    }

    /* IJB
        if (GameMgr.IsZoneLocked(GetZone1())) {
            m_TimeSinceLastZonePain += DeltaTime;

            if (m_TimeSinceLastZonePain > 0.5f) {
                //Do Damage
                pain Pain;

                pain_handle PainHandle("ZONE_PAIN");
                Pain.Setup(PainHandle, 0, GetBBox().GetCenter());

                Pain.SetCustomScalar(m_TimeSinceLastZonePain);

                Pain.ApplyToObject(this);

                m_TimeSinceLastZonePain = 0.0f;
            }
        }
    */

    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    // Let physics keep track of riding on platform
    m_Physics.CatchUpWithRidingPlatform(DeltaTime);
    m_Physics.WatchForRidingPlatform();

    UpdateMovement(DeltaTime);

    // Call base class
    actor::OnAliveLogic(DeltaTime);

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================

    // Handles rotation.
    UpdateRotation(AlteredDeltaTime);
    UpdateCharacterRotation(AlteredDeltaTime);
    UpdateCrouchHeight(AlteredDeltaTime);
}

//===========================================================================

void player::OnDeathLogic(float DeltaTime)
{
    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    // Keep physics going for death while falling.
    m_Physics.Advance(m_Physics.GetPosition(), DeltaTime, true);

    OnMove(m_Physics.GetPosition());

    if (UsingLoco()) {
        if (m_pLoco && m_pLoco->IsPlayAnimComplete()) {
            CreateCorpse();
        }
    }

    // Call base class
    actor::OnDeathLogic(DeltaTime);

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================
}

//===========================================================================
void player::OnMove(const Vector3& rNewPos)
{
    OnMoveViewPosition(rNewPos);

    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    if (GetAttrBits() & Object::ATTR_DESTROY) {
        return;
    }

    m_DeltaPos = rNewPos - GetPosition();

    // HACKOMOTRON -
    //
    // Problem: When the ghost spawns or is created, he is created essentially
    // at the origin and then moved to his starting point.  The above
    // computation for m_DeltaPos results in a large vector which is, in turn,
    // used by the physics the first time the ghost runs his logic.  The poor
    // ghost then proceeds to collide with several walls.
    //
    // Proper solution:
    // (1) The ghost should not run physics or collision.
    // (2) Upon creation or spawning, the ghost should not attempt to travel
    //     long distances in the first frame following.
    //
    // HACK: If m_DeltaPos is too large, set it to 0.

    if (m_DeltaPos.LengthSquared() > (500 * 500)) { // 5 meters squared
        m_DeltaPos.Zero();
    }

    actor::OnMove(rNewPos);

    m_Physics.SetPosition(rNewPos);

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================
}

//===========================================================================
void player::OnMoveViewPosition(const Vector3& rNewPos)
{
    (void)rNewPos;
    Vector3 EyesOffSet = m_EyesOffset;

    EyesOffSet.RotateY(m_Yaw);

    Vector3 vViewPosition;
    float   Height = GetBBox().max.y;

    vViewPosition.set(rNewPos.GetX(), Height, rNewPos.GetZ());
    vViewPosition += EyesOffSet;

    MoveAnimPlayer(vViewPosition);
}

//=========================================================================

void player::OnTransform(const Matrix4& L2W)
{
    //==========================================================================================
    // Begin code from ghost.cpp
    //==========================================================================================

    actor::OnTransform(L2W);

    //update physics
    m_Physics.SetPosition(L2W.GetTranslation());

    //==========================================================================================
    // End code from ghost.cpp
    //==========================================================================================

    m_Yaw = L2W.GetRotation().yaw;
    m_AnimPlayer.SetYaw(m_fCurrentYawOffset + m_Yaw);

    OnMoveViewPosition(L2W.GetTranslation());
    MoveAnimPlayer(m_EyesPosition);

    //set tracker info for portal/zone
    UpdateZoneTrack();
}

//=========================================================================
static float s_ArmsDampen = 0.2f;
static float s_ArmReturnForceMultiplier = 75.0f;

void player::UpdateMovement(float DeltaTime)
{
    // Skip if in a cinema
    if (m_ViewCinematicPlaying) {
        return;
    }

    // Try ladder movement first
    if (UpdateLadderMovement(DeltaTime)) {
        return;
    }

    Vector3 ViewX(1, 0, 0);
    Vector3 ViewZ(0, 0, 1);

    ViewX.RotateY(m_Yaw);
    ViewZ.RotateY(m_Yaw);

    CalculateRigOffset(DeltaTime);

    // Compute normal movement
    CalculateStrafeVelocity(ViewX, DeltaTime);
    CalculateForwardVelocity(ViewZ, DeltaTime);

    Vector3 FinalVel = m_StrafeVelocity + m_ForwardVelocity;

    //if we're crouching, move a little slower
    if (m_bIsCrouching) {
        FinalVel *= .45f;
    }

    m_DeltaPos = FinalVel;

    if (!m_ViewCinematicPlaying && !m_bInTurret) {
        //==========================================================================================
        // Begin code from ghost.cpp
        //==========================================================================================

        m_Physics.Advance(
            m_Physics.GetPosition() + m_DeltaPos,
            DeltaTime);

        //==========================================================================================
        // End code from ghost.cpp
        //==========================================================================================
    }

    //
    // Update the arms velocity and position
    //

    assert(m_ArmsOffset.IsValid());
    assert(m_ArmsVelocity.IsValid());

    // return force...
    Vector3 ReturnForce(-m_ArmsOffset);
    ReturnForce *= s_ArmReturnForceMultiplier;
    m_ArmsVelocity += ReturnForce * DeltaTime;

    assert(m_ArmsOffset.IsValid());
    assert(m_ArmsVelocity.IsValid());

    // dampen with drag
    m_ArmsVelocity -= m_ArmsVelocity * s_ArmsDampen;
    m_ArmsOffset += m_ArmsVelocity * DeltaTime;

    assert(m_ArmsOffset.IsValid());
    assert(m_ArmsVelocity.IsValid());

    UpdateArmsOffsetForLean();

    OnMove(m_Physics.GetPosition());
}

//===========================================================================

void player::UpdateAudio(float DeltaTime)
{
    /* IJB
    if (GetLocalSlot() == -1) {
        return;
    }

    // Update the ear.
    view& View = GetView();
    ComputeView(View);
    g_AudioMgr.SetEar(m_AudioEarID, View.GetW2V(), GetPosition(), GetZone1(), 1.0f);

    if (DoFootfallCollisions()) {
        PlayFootfall(DeltaTime);
    }
    ProcessSfxEvents();
    */
}

//==============================================================================

void player::UpdateCharacterRotation(const float& DeltaTime)
{
    CalculateLookHorozOffset(DeltaTime);
    CalculateLookVertOffset(DeltaTime);

    //set the pitch and yaw of the rig.
    m_AnimPlayer.SetYaw(m_Yaw + m_CurrentHorozRigOffset - m_ShakeYaw);
    m_AnimPlayer.SetPitch(-m_Pitch - m_CurrentVertRigOffset - m_ShakePitch);
    m_AnimPlayer.SetRoll(m_SoftLeanAmount * DEG_TO_RAD(GetTweakF32("LeanMaxDegrees")));
}

//==============================================================================

void player::UpdateCrouchHeight(const float& rDeltaTime)
{
    if (m_bIsCrouching) {
        //trying to crouch
        float NewCrouchFactor = std::min(1.f, m_fCurrentCrouchFactor + m_fCrouchChangeRate * rDeltaTime);
        if (m_Physics.SetCrouchParametric(NewCrouchFactor)) {
            m_fCurrentCrouchFactor = NewCrouchFactor;
        }
    } else if (m_fCurrentCrouchFactor > 0.f) {
        float NewCrouchFactor = std::max(0.f, m_fCurrentCrouchFactor - m_fCrouchChangeRate * rDeltaTime);
        if (m_Physics.SetCrouchParametric(NewCrouchFactor)) {
            m_fCurrentCrouchFactor = NewCrouchFactor;
        }
    }
}

//===========================================================================

void player::OnEnumProp(prop_enum& rList)
{
    actor::OnEnumProp(rList);

    //
    // Player info
    //
    rList.PropEnumHeader("Player", "Player/Mutation information", PROP_TYPE_HEADER);

    rList.PropEnumAngle("Player\\CamFOV", "This is the Field of View in degrees.", PROP_TYPE_EXPOSE);
    rList.PropEnumRotation("Player\\View Rotation", "This rotation sets up the player view on startup. Use the cyan cone as your pointer", 0);

    rList.PropEnumInt("Player\\LoreDiscoveries", "", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SHOW);
    rList.PropEnumBool("Player\\RenderSkeleton", "Renders the skeleton of the player. This is use for debugging.", PROP_TYPE_DONT_SAVE_MEMCARD);
    rList.PropEnumBool("Player\\RenderBoneNames", "When the Skeleton is render whether you want to render the name of the bones as well", PROP_TYPE_DONT_SAVE_MEMCARD);
    rList.PropEnumBool("Player\\RenderBBox", "This allows to turn off and on the BBox of the player.", PROP_TYPE_DONT_SAVE_MEMCARD);

    rList.PropEnumFloat("Player\\ArmPitchModifier+1", "This is a scaler value use to multiply the camera pitch (when>0)so that the arms don't fallow exactly the camera.", PROP_TYPE_DONT_SAVE_MEMCARD);
    rList.PropEnumFloat("Player\\ArmPitchModifier-1", "This is a scaler value use to multiply the camera pitch (when<0)so that the arms don't fallow exactly the camera.", PROP_TYPE_DONT_SAVE_MEMCARD);

    rList.PropEnumBool("Player\\Can Die", "Determines if the player can die.", PROP_TYPE_DONT_SAVE_MEMCARD | PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Can Jump", "Determines if the player can jump.", PROP_TYPE_DONT_SAVE_MEMCARD | PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Hide Player Arms", "Do we need to hide the player arms for a special event?.", PROP_TYPE_DONT_SAVE_MEMCARD | PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Play SwitchTo", "If this is true, this will play the SwitchTo animation after arms re-appear from Hide Player Arms(default is true).", PROP_TYPE_DONT_SAVE_MEMCARD | PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Using Flashlight", "Indicates if the player is using the flashlight", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SHOW | PROP_TYPE_DONT_SAVE | PROP_TYPE_READ_ONLY);
    rList.PropEnumFloat("Player\\Melee Damage", "This is the damage dished out in one direct melee hit -- not mutation melee", 0);
    rList.PropEnumFloat("Player\\Melee Force", "This is the force dished out in one direct melee hit -- not mutation melee", 0);
    rList.PropEnumFloat("Player\\Health", "Player's Health (1-100, 100 = Full Health)", PROP_TYPE_EXPOSE);
    rList.PropEnumFloat("Player\\Mutagen", "Player's Mutagen level (0-100, 100 = Full Mutagen)", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\In Mutation Tutorial", "true if we are the mutation tutorial is running, changing mutagen behavior", PROP_TYPE_EXPOSE);
    // Third Person Geometry
    m_SkinInst.OnEnumProp(rList);
    rList.PropEnumExternal("RenderInst\\Anim", "Resource\0anim\0", "Resource File", PROP_TYPE_MUST_ENUM | PROP_TYPE_DONT_SAVE_MEMCARD);

    // Skins and animations
    // Enumerate the different strains.
    rList.PropEnumHeader("Player\\Human Strain", "Properties of the human strain", 0);
    rList.PropEnumHeader("Player\\Strain One", "Properties of strain one.", 0);
    rList.PropEnumHeader("Player\\Strain Two", "Properties of strain two.", 0);
    rList.PropEnumHeader("Player\\Strain Three", "Properties of strain three.", 0);

    //    rList.PropEnumEnum (  "Player\\Current Strain", GetStrainEnum(), "Current strain.", PROP_TYPE_EXPOSE  );

    rList.PropEnumBool("Player\\Holster Weapon", "When true, player weapon is hidden, and the user can't cycle or fire", PROP_TYPE_EXPOSE);

    // Enumerate the human specific properties
    int PathIndex = rList.PushPath("Player\\Human Strain\\");
    m_Physics.OnEnumProp(rList);
    m_Skin.OnEnumProp(rList);
    rList.PropEnumExternal("RenderInst\\AnimFile", "Resource\0anim", "Resource Animation File", PROP_TYPE_MUST_ENUM | PROP_TYPE_DONT_SAVE_MEMCARD);
    rList.PropEnumExternal("Audio Package", "Resource\0audiopkg", "The audio package associated with human strain.", PROP_TYPE_DONT_SAVE_MEMCARD);
    EnumrateStrainControls(rList);
    rList.PropEnumHeader("Friendly Factions", "Reticle doesn't highlight on friends", 0);
    int ID = rList.PushPath("Friendly Factions\\");
    // IJB factions_manager::OnEnumFriends(rList);
    rList.PopPath(ID);
    rList.PopPath(PathIndex);

    //  Mutation properties
    rList.PropEnumHeader("Player\\Mutation", "Properties for mutation behavior", 0);
    rList.PropEnumBool("Player\\Mutation\\Is Mutated", "READ ONLY: Indicates if the player is mutated or not.", PROP_TYPE_READ_ONLY | PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\Can Melee", "true if the player can use the mutant melee attack.", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\Can Fire Primary Ammo", "true if the player can use primary mutation ammo.", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\Can Fire Secondary Ammo", "true if the player can use secondary mutation ammo.", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\User Can Toggle Mutation", "true if the user can control mutation through the controller.", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\Force To Mutant", "Set to true to force the player to mutate, assuming he has the mutation weapon.", PROP_TYPE_EXPOSE);
    rList.PropEnumBool("Player\\Mutation\\Force To Human", "Set to true to force the player to become human, assuming he has the mutation weapon.", PROP_TYPE_EXPOSE);
    rList.PropEnumFloat("Player\\Mutation\\Convulsion Feedback Duration", "Duration, in seconds, of the controller feedback for mutation convulsions.", PROP_TYPE_EXPOSE);
    rList.PropEnumFloat("Player\\Mutation\\Convulsion Feedback Intensity", "Intensity, from 0.0 to 1.0, of the controller feedback for mutation convulsions. Full normal force is 1.0.", PROP_TYPE_EXPOSE);

    rList.PropEnumHeader("Player\\Cinema", "Properties for cinemas", 0);
    rList.PropEnumBool("Player\\Cinema\\CinemaOn", "Turns cinema mode and off", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    rList.PropEnumGuid("Player\\Cinema\\CinemaCameraGuid", "This points to the camera that should be the player's view", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    rList.PropEnumBool("Player\\Cinema\\UseViewCorrection", "Do we correct the view at the end of cinema. Don't use this if you are popping player to position", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    rList.PropEnumGuid("Player\\Cinema\\LookAtTarget", "Object to focus camera on", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    rList.PropEnumFloat("Player\\Cinema\\BlendInTime", "How long to blend to desired view", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
}

//===========================================================================

void player::EnumrateStrainControls(prop_enum& List)
{
    List.PropEnumFloat("MaxHealth", "Maximum health value", 0);
    List.PropEnumFloat("Proximity Alert Radius", "Radius of Proximity Broadcast which will alert NPC's to player presence.", 0);

    List.PropEnumHeader("Controls", "Variables that effect the player's control", PROP_TYPE_HEADER);
    List.PropEnumFloat("Controls\\MaxFowardVel", "How fast does the player move foward at this mutation level", 0);
    List.PropEnumFloat("Controls\\MaxStrafeVel", "How fast does the player strafe at this mutation level", 0);
    List.PropEnumFloat("Controls\\FowardAccel", "Players foward acceleration at this mutation level", 0);
    List.PropEnumFloat("Controls\\StrafeAccel", "Players strafe acceleration at this mutation level", 0);
    List.PropEnumFloat("Controls\\Pitch Accel Time", "How long it takes to get to full pitch change speed.", 0);
    List.PropEnumFloat("Controls\\Yaw Accel Time", "How long it takes to get to full pitch change speed.", 0);
    List.PropEnumFloat("Controls\\JumpVelocity", "How fast does the player jump. This is the initial velocity for the jump.", 0);
    List.PropEnumVector3("Controls\\EyesOffSet", "This is where the eyes are relative to the top of his head", 0);
    List.PropEnumFloat("Controls\\Pitch Stick Sensitivity", "How sensitive is the right thumb stick pitch.  Expects a number above zero.", 0);
    List.PropEnumFloat("Controls\\Yaw Stick Sensitivity", "How sensitive is the right thumb stick pitch.  Expects a number above zero.", 0);
    List.PropEnumFloat("Controls\\MinWalkSpeed", "Minimum speed at which the player walks.", 0);
    List.PropEnumFloat("Controls\\MinRunSpeed", "Minimum speed at which the player runs.", 0);
    List.PropEnumFloat("Controls\\Deceleration Multiplier", "This is how many times faster the player slows down than speeds up.", 0);
    List.PropEnumFloat("Controls\\Crouch Change Rate", "This is how fast the player crouches.  10 is a good place to start tweaking.  Lower is slower, higher is faster", 0);
    List.PropEnumFloat("Controls\\Movement Aim Degradation", "How much aim you are going to lose by movement 0 -> 1", 0);

    List.PropEnumHeader("Controls\\Stun Properties", "Effects the way this guy is stunned", 0);
    List.PropEnumAngle("Controls\\Stun Properties\\MaxStunPitchOffset", "How far does the pitch go?", 0);
    List.PropEnumAngle("Controls\\Stun Properties\\MaxStunYawOffset", "How far does the yaw go?", 0);
    List.PropEnumAngle("Controls\\Stun Properties\\MaxStunRollOffset", "How far does the roll go?", 0);
    List.PropEnumFloat("Controls\\Stun Properties\\StunYawChangeSpeed", "How fast does the yaw change", 0);
    List.PropEnumFloat("Controls\\Stun Properties\\StunPitchChangeSpeed", "How fast does the yaw change", 0);
    List.PropEnumFloat("Controls\\Stun Properties\\StunRollChangeSpeed", "How fast does the yaw change", 0);
    List.PropEnumFloat("Controls\\Stun Properties\\Stun Time", "How long does he stay stunned?", 0);
}

//===========================================================================

bool player::OnStrainControlProperty(prop_query& I)
{
    if (I.VarFloat("Proximity Alert Radius", m_StrainControls.m_StrainProximityAlertRadius)) {
        return true;
    }

    if (I.VarFloat("Controls\\MaxFowardVel", m_StrainControls.m_StrainMaxFowardVelocity)) {
        return true;
    }

    if (I.VarFloat("Controls\\MaxStrafeVel", m_StrainControls.m_StrainMaxStrafeVelocity)) {
        return true;
    }

    if (I.VarFloat("Controls\\JumpVelocity", m_StrainControls.m_StrainJumpVelocity)) {
        return true;
    }

    if (I.VarVector3("Controls\\EyesOffSet", m_StrainControls.m_StrainEyesOffset)) {
        return true;
    }

    if (I.VarFloat("MaxHealth", m_StrainControls.m_StrainMaxHealth)) {
        return true;
    }

    if (I.VarFloat("Controls\\FowardAccel", m_StrainControls.m_fStrainForwardAccel)) {
        return true;
    }

    if (I.VarFloat("Controls\\StrafeAccel", m_StrainControls.m_fStrainStrafeAccel)) {
        return true;
    }

    if (I.VarFloat("Controls\\Pitch Stick Sensitivity", m_StrainControls.m_fStrainPitchSensitivity)) {
        return true;
    }

    if (I.VarFloat("Controls\\Yaw Stick Sensitivity", m_StrainControls.m_fStrainYawSensitivity)) {
        return true;
    }

    if (I.VarFloat("Controls\\Yaw Accel Time", m_StrainControls.m_StrainYawAccelTime)) {
        return true;
    }

    if (I.VarFloat("Controls\\Pitch Accel Time", m_StrainControls.m_StrainPitchAccelTime)) {
        return true;
    }

    if (I.VarFloat("Controls\\MinWalkSpeed", m_StrainControls.m_StrainMinWalkSpeed)) {

        return true;
    }

    if (I.VarFloat("Controls\\MinRunSpeed", m_StrainControls.m_StrainMinRunSpeed)) {

        return true;
    }

    if (I.VarFloat("Controls\\Deceleration Multiplier", m_StrainControls.m_StrainDecelerationFactor)) {
        return true;
    }

    if (I.VarFloat("Controls\\Crouch Change Rate", m_StrainControls.m_StrainCrouchChangeRate)) {
        return true;
    }

    if (I.VarFloat("Controls\\Movement Aim Degradation", m_StrainControls.m_StrainReticleMovementDegrade, 0.0f, 1.0f)) {
        return true;
    }

    if (I.VarAngle("Controls\\Stun Properties\\MaxStunPitchOffset", m_MaxStunPitchOffset)) {
        return true;
    }

    if (I.VarAngle("Controls\\Stun Properties\\MaxStunYawOffset", m_MaxStunYawOffset)) {
        return true;
    }

    if (I.VarAngle("Controls\\Stun Properties\\MaxStunRollOffset", m_MaxStunRollOffset)) {
        return true;
    }

    if (I.VarFloat("Controls\\Stun Properties\\StunYawChangeSpeed", m_fStunYawChangeSpeed)) {
        return true;
    }

    if (I.VarFloat("Controls\\Stun Properties\\StunPitchChangeSpeed", m_fStunPitchChangeSpeed)) {
        return true;
    }

    if (I.VarFloat("Controls\\Stun Properties\\StunRollChangeSpeed", m_fStunRollChangeSpeed)) {
        return true;
    }

    if (I.VarFloat("Controls\\Stun Properties\\Stun Time", m_fMaxStunTime)) {
        return true;
    }

    return false;
}

//===========================================================================

bool player::OnProperty(prop_query& rPropQuery)
{
    if (rPropQuery.VarBool("Player\\Can Die", m_bCanDie)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Can Jump", m_bCanJump)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Hide Player Arms", m_bHidePlayerArms)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Play SwitchTo", m_bPlaySwitchTo)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Using Flashlight", m_bUsingFlashlight)) {
        return true;
    }

    if (rPropQuery.IsVar("Player\\Cinema\\CinemaOn")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarBool(m_Cinema.m_bCinemaOn);
        } else {
            m_Cinema.m_bCinemaOn = rPropQuery.GetVarBool();

            // Make sure zone flag is cleared ready for cinema
            m_Cinema.m_bPlayerZoneInitialized = false;

            if (m_bIsMutated) {
                // make sure our mutant vision is on, since the cinema will interrupt any
                // animations, and this will prevent the mutant vision event from firing

                m_bIsMutantVisionOn = true;

                if (m_Cinema.m_bCinemaOn) {
                    // we need to turn off the mutant perception stuff.
                    g_PerceptionMgr.EndMutate();
                } else {
                    // we need to turn mutant perception stuff back on.
                    g_PerceptionMgr.BeginMutate();
                    /* IJB
                                        // Force switch to mutation weapon?
                                        new_weapon* pWeapon = GetCurrentWeaponPtr();
                                        if ((!pWeapon) || (!pWeapon->IsKindOf(weapon_mutation::GetRTTI()))) {
                                            m_NextWeaponItem = INVEN_WEAPON_MUTATION;
                                            ForceNextWeapon();
                                        }

                                        // Make sure the weapon mesh is set properly
                                        pWeapon = GetCurrentWeaponPtr();
                                        assert(pWeapon && pWeapon->IsKindOf(weapon_mutation::GetRTTI()));
                                        render_inst* pInst = pWeapon->GetRenderInstPtr();
                                        assert(pInst);
                                        pInst->SetVMeshMask(0xffffffff);
                    */
                }
            } else {
                // make sure our mutant vision is OFF, since the cinema will interrupt any
                // animations, and this will prevent the mutant vision event from firing

                m_bIsMutantVisionOn = false;

                if (!m_Cinema.m_bCinemaOn) {
                    // Make sure the weapon mesh is set properly
                    new_weapon* pWeapon = GetCurrentWeaponPtr();
                    /* IJB
                    if (pWeapon && pWeapon->IsKindOf(weapon_mutation::GetRTTI())) {
                        // Turn off mutant hands
                        render_inst* pInst = pWeapon->GetRenderInstPtr();
                        assert(pInst);
                        pInst->SetVMeshMask(0);

                        // Force switch to previous weapon?
                        m_NextWeaponItem = m_PreMutationWeapon2;
                        ForceNextWeapon();
                    }
                        */
                }
            }

            if (m_bIsCrouching) {
                SetIsCrouching(false);
            }
        }

        if (m_Cinema.m_bCinemaOn) {
            // Stop leaning
            m_LeanAmount = 0.0f;
            m_SoftLeanAmount = 0.0f;
        }
        return true;
    }

    if (rPropQuery.VarGUID("Player\\Cinema\\CinemaCameraGuid", m_Cinema.m_CinemaCameraGuid)) {
        return true;
    }

    if (rPropQuery.VarGUID("Player\\Cinema\\LookAtTarget", m_Cinema.m_LookAtTargetGuid)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Cinema\\UseViewCorrection", m_Cinema.m_bUseViewCorrection)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\Cinema\\BlendInTime", m_Cinema.m_BlendInTime)) {
        return true;
    }

    if (rPropQuery.IsVar("Player\\Health")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarFloat(m_Health.GetHealth());
        } else {
            // Get new health value
            float NewHealth = rPropQuery.GetVarFloat();
            if (NewHealth < 1.0f) {
                NewHealth = 1.0f;
            }

            // Now add the difference between the new health and current health
            float DeltaHealth = NewHealth - m_Health.GetHealth();
            if (DeltaHealth != 0.0f) {
                m_Health.Add(DeltaHealth, false);
            }
        }

        return true;
    }

    if (rPropQuery.VarFloat("Player\\Mutagen", m_Mutagen, 0, 100.0f)) {
        return true;
    }

    const bool WasInTutorial = m_bInMutationTutorial;
    if (rPropQuery.VarBool("Player\\In Mutation Tutorial", m_bInMutationTutorial)) {
        if (WasInTutorial != m_bInMutationTutorial) {
            if (m_bInMutationTutorial) {
                // Start the convulsion system
                m_ConvulsionInfo.m_bConvulsingNow = false;
                m_ConvulsionInfo.m_TimeSinceLastConvulsion = 0.0f;
                m_ConvulsionInfo.m_ConvulseAtTime = m_Mutagen * g_ConvulsionTweaks.m_MutagenConvulsionMultiplierPeriod;
            } else {
                // No more convulsions
                m_ConvulsionInfo.m_bConvulsingNow = false;
            }
        }
        return true;
    }

    if (Object::OnProperty(rPropQuery)) {
        if (rPropQuery.IsVar("Base\\Position")) {
            if (rPropQuery.IsRead()) {
                m_PositionOfLastSafeSpot = GetPosition();
                m_NextPositionOfLastSafeSpot = m_PositionOfLastSafeSpot;
                m_RespawnPosition = GetPosition();
            } else {
                m_PositionOfLastSafeSpot = rPropQuery.GetVarVector3();
                m_NextPositionOfLastSafeSpot = m_PositionOfLastSafeSpot;
                m_RespawnPosition = rPropQuery.GetVarVector3();
            }
        } else if (rPropQuery.IsVar("Base\\ZoneInfo") && !rPropQuery.IsRead()) {
            m_NextZoneIDOfLastSafeSpot = (uint8_t)GetZone1();
            m_ZoneIDOfLastSafeSpot = (uint8_t)GetZone1();
            m_RespawnZone = (uint8_t)GetZone1();
        }
        return true;
    }
    /* IJB
        if (rPropQuery.VarString("Player", (char *)m_pPlayerTitle, 256)) {
            // You can only read this guy
            assert(rPropQuery.IsRead() == true);
            return true;
        }
    */
    if (rPropQuery.VarInt("Player\\LoreDiscoveries", m_nLoreDiscoveries)) {
        return true;
    }

#if !defined(CONFIG_RETAIL)
    if (rPropQuery.VarBool("Player\\RenderSkeleton", m_bRenderSkeleton)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\RenderBoneNames", m_bRenderSkeletonNames)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\RenderBBox", m_bRenderBBox)) {
        return true;
    }
#endif // !defined( CONFIG_RETAIL )

    if (rPropQuery.VarFloat("Player\\ArmPitchModifier+1", m_PitchArmsScalerPositive)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\ArmPitchModifier-1", m_PitchArmsScalerNegative)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\Melee Damage", m_MeleeDamage)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\Melee Force", m_MeleeForce)) {
        return true;
    }

    if (rPropQuery.IsVar("Player\\CamFOV")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarAngle(m_ViewInfo.XFOV);
        } else {
            m_ViewInfo.XFOV = rPropQuery.GetVarAngle();
            m_OriginalViewInfo.XFOV = m_ViewInfo.XFOV;
        }
        return true;
    }

    Radian3 Rot(m_Pitch, m_Yaw, 0.0f);
    if (rPropQuery.VarRotation("Player\\View Rotation", Rot)) {
        m_Pitch = Rot.pitch;
        m_Yaw = Rot.yaw;
        return true;
    }

    bool bIsMutated = m_bIsMutated;
    if (rPropQuery.VarBool("Player\\Mutation\\Is Mutated", bIsMutated)) {
        m_bIsMutated = bIsMutated;
        return true;
    }

    if (rPropQuery.VarBool("Player\\Mutation\\Can Melee", m_bMutationMeleeEnabled)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Mutation\\Can Fire Primary Ammo", m_bPrimaryMutationFireEnabled)) {
        return true;
    }

    if (rPropQuery.VarBool("Player\\Mutation\\Can Fire Secondary Ammo", m_bSecondaryMutationFireEnabled)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\Mutation\\Convulsion Feedback Duration", m_ConvulsionFeedbackDuration)) {
        return true;
    }

    if (rPropQuery.VarFloat("Player\\Mutation\\Convulsion Feedback Intensity", m_ConvulsionFeedbackIntensity)) {
        return true;
    }

    bool Temp = m_bCanToggleMutation;
    if (rPropQuery.VarBool("Player\\Mutation\\User Can Toggle Mutation", Temp)) {
        m_bCanToggleMutation = Temp;
        return true;
    }

    bool TempForce = false;
    if (rPropQuery.VarBool("Player\\Mutation\\Force To Mutant", TempForce)) {
        // if we're not mutated, get there
        if (TempForce && m_Inventory2.HasItem(INVEN_WEAPON_MUTATION) && !IsMutated()) {
            ForceMutationChange(true);
        }

        return true;
    }

    if (rPropQuery.VarBool("Player\\Mutation\\Force To Human", TempForce)) {
        // if we're mutant, go human
        if (TempForce && IsMutated()) {
            SetupMutationChange(false);
        }

        return true;
    }

    if (actor::OnProperty(rPropQuery)) {
        return true;
    }

    // Human
    int PathIndex = rPropQuery.PushPath("Player\\Human Strain\\");
    {
        if (OnStrainProperty(rPropQuery)) {
            return true;
        }
        if (OnStrainControlProperty(rPropQuery)) {
            return true;
        }
    }
    rPropQuery.PopPath(PathIndex);

    return false;
}

//===========================================================================

bool player::OnStrainProperty(prop_query& rPropQuery)
{
    if (m_Skin.OnProperty(rPropQuery)) {
        return true;
    }

    if (m_Physics.OnProperty(rPropQuery)) {
        return true;
    }

    if (rPropQuery.IsVar("RenderInst\\AnimFile")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarExternal(m_AnimGroup.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // WARNING:
            // It may be some problem here. The resurce handles can't start counting references
            // untill a name has been assign to them. Not only that but when a new name is set it
            // must make sure that the old name is decremented reference wise.
            m_AnimGroup.setName(rPropQuery.GetVarExternal());

            // Make sure that this are clear not matter what
            m_iCameraBone = -1;
            m_iCameraTargetBone = -1;

            // If we can load this animgoup then we need to extract some info
            if (m_AnimGroup.getPointer()) {
                OnAnimationInit();
            }

            // Notify the user if we don't have certain key bones
            if (m_iCameraBone == -1) {
                //  x_DebugMsg("WARNING: There is not Camera bone (bone_cam) in the skeleton of the player(%d)\n", m_pPlayerTitle);
            }

            if (m_iCameraTargetBone == -1) {
                //   x_DebugMsg("WARNING: There is not Camera TargetBone bone(bone_cam.Target) in the skeleton of the player(%d)\n", m_pPlayerTitle);
            }
        }
        return true;
    }

    // External
    /* IJB
    if (rPropQuery.IsVar("Audio Package")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarExternal(m_hAudioPackage.getName(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = rPropQuery.GetVarExternal();

            if (pString[0]) {
                if (xstring(pString) == "<null>") {
                    m_hAudioPackage.SetName("");
                } else {
                    m_hAudioPackage.SetName(pString);

                    // Load the audio package.
                    if (m_hAudioPackage.IsLoaded() == false) {
                        m_hAudioPackage.GetPointer();
                    }
                }
            }
        }
        return true;
    }
*/
    int ID = rPropQuery.PushPath("Friendly Factions\\");
    /* IJB
    if (factions_manager::OnPropertyFriends(rPropQuery, m_StrainFriendFlags)) {
        return true;
    }
        */
    rPropQuery.PopPath(ID);

    return false;
}

//===========================================================================

// Animation initialization methods.
void player::OnAnimationInit()
{
    m_AnimPlayer.SetAnimGroup(m_AnimGroup);
    m_AnimPlayer.SetAnim(0, true, true);

    if (m_AnimPlayer.GetBoneIndex("bone_cam") != -1) {
        m_iCameraBone = m_AnimPlayer.GetBoneIndex("bone_cam");
    }

    if (m_AnimPlayer.GetBoneIndex("bone_cam.Target") != -1) {
        m_iCameraTargetBone = m_AnimPlayer.GetBoneIndex("bone_cam.Target");
    }

    // Reset our animations for this strain.
    ResetAnimationTable();

    // Start collecting animations for each state
    animation_state  AnimIndex = ANIM_STATE_UNDEFINED;
    const AnimGroup& animGroup = m_AnimPlayer.GetAnimGroup();

    for (int i = 0; i < animGroup.GetNAnims(); i++) {
        int WeaponIndex = -1;
        AnimIndex = ANIM_STATE_UNDEFINED;

        const AnimInfo& animInfo = animGroup.GetAnimInfo(i);
        const char*     pAnimName = animInfo.GetName();

        // This animation is always there...
        if (strcmp(pAnimName, "BIND_POSE") == 0) {
            continue;
        }

        WeaponIndex = inventory2::ItemToWeaponIndex(GetWeaponFromAnimName(pAnimName));
        AnimIndex = GetAnimStateFromName(pAnimName);

        if (!((WeaponIndex == 0) && (AnimIndex == ANIM_STATE_DEATH)) && ((WeaponIndex <= 0) || (AnimIndex < 0))) {
            continue;
        }

        //We have valid index to the animation table, now set the values
        state_anims& State = m_Anim[WeaponIndex][AnimIndex];
        if (State.nPlayerAnims >= MAX_ANIM_PER_STATE) {
            //x_throw(xfs("Too many animations of this type %s for player", pAnimName));
            return;
        } else {
            if (AnimIndex >= ANIM_STATE_DEATH) {
                int nAnimState = AnimIndex;
                for (int j = 0; j < INVEN_NUM_WEAPONS; j++) {
                    m_Anim[j][nAnimState].PlayerAnim[ANIM_PRIORITY_DEFAULT] = i;
                    m_Anim[j][nAnimState].nPlayerAnims = 1;
                    m_Anim[j][nAnimState].nWeaponAnims++;
                }
            }

            // Set the index of the animation in the table at the appropriate place.
            assert(State.nPlayerAnims < MAX_ANIM_PER_STATE);
            State.PlayerAnim[State.nPlayerAnims] = i;
            State.nPlayerAnims++;
        }
    }
}

//==============================================================================

void player::ResetAnimationTable()
{
    int i, j;

    //clear the animation array.
    for (i = 0; i < INVEN_NUM_WEAPONS; i++) {
        for (j = 0; j < ANIM_STATE_MAX; j++) {
            m_Anim[i][j].nPlayerAnims = 0;
            m_Anim[i][j].nWeaponAnims = 0;
        }
    }
}

//==============================================================================

inven_item player::GetWeaponFromAnimName(const char* pAnimName)
{
    //parse for weapon name.  Documentation on naming conventions used for the player and weapon animations can
    //be found in C:\GameData\A51\Source\Art\Characters\Mut 01_02 - Arms\NOTES_MUT01_01.txt
    inven_item retValue = INVEN_NULL;

    if (x_stristr(pAnimName, "SMP_")) {
        retValue = INVEN_WEAPON_SMP;
    }

    // KSS -- TO ADD NEW WEAPON
    else if (x_stristr(pAnimName, "SHT_")) {
        retValue = INVEN_WEAPON_SHOTGUN;
    } else if (x_stristr(pAnimName, "SCN_")) {
        retValue = INVEN_WEAPON_SCANNER;
    } else if (x_stristr(pAnimName, "SNI_")) {
        retValue = INVEN_WEAPON_SNIPER_RIFLE;
    } else if (x_stristr(pAnimName, "EGL_")) {
        retValue = INVEN_WEAPON_DESERT_EAGLE;
    } else if (x_stristr(pAnimName, "MSN_")) {
        retValue = INVEN_WEAPON_MESON_CANNON;
    } else if (x_stristr(pAnimName, "BBG_")) {
        retValue = INVEN_WEAPON_BBG;
    } else if (x_stristr(pAnimName, "TRA_")) {
        retValue = INVEN_WEAPON_TRA;
    } else if (x_stristr(pAnimName, "2MP_")) {
        retValue = INVEN_WEAPON_DUAL_SMP;
    } else if (x_stristr(pAnimName, "2SH_")) {
        retValue = INVEN_WEAPON_DUAL_SHT;
    } else if (x_stristr(pAnimName, "MUT_")) {
        retValue = INVEN_WEAPON_MUTATION;
    }

    return retValue;
}

//==============================================================================

player::animation_state player::GetAnimStateFromName(const char* pAnimName)
{
    //parse for animation state.  Documentation on naming conventions used for the player and weapon animations can
    //be found in C:\GameData\A51\Source\Art\Characters\Mut 01_02 - Arms\NOTES_MUT01_01.txt
    animation_state retValue = ANIM_STATE_UNDEFINED;

    if (x_stristr(pAnimName, "_Idle")) {
        retValue = ANIM_STATE_IDLE;
    } else if (x_stristr(pAnimName, "_Switch_To")) {
        retValue = ANIM_STATE_SWITCH_TO;
    } else if (x_stristr(pAnimName, "_Switch_From")) {
        retValue = ANIM_STATE_SWITCH_FROM;
    } else if (x_stristr(pAnimName, "_Pickup")) {
        retValue = ANIM_STATE_PICKUP;
    } else if (x_stristr(pAnimName, "_Discard")) {
        retValue = ANIM_STATE_DISCARD;
    }
    //
    // NEEDS TO RESOLVE THIS "_Reload" will return true when looking at ".._Reload_IN"
    // for now we will check the Reload_In and Reload_Out stuff first.
    //
    //
    else if (x_stristr(pAnimName, "_Load_IN")) {
        retValue = ANIM_STATE_RELOAD_IN;
    } else if (x_stristr(pAnimName, "_Load_OUT")) {
        retValue = ANIM_STATE_RELOAD_OUT;
    }

    else if (x_stristr(pAnimName, "_Reload")) {
        retValue = ANIM_STATE_RELOAD;
    } else if (x_stristr(pAnimName, "_Fire")) {
        retValue = ANIM_STATE_FIRE;
    } else if (x_stristr(pAnimName, "_AltFire")) {
        retValue = ANIM_STATE_ALT_FIRE;
    } else if (x_stristr(pAnimName, "_Grenade")) {
        retValue = ANIM_STATE_GRENADE;
    } else if (x_stristr(pAnimName, "_AltGrenade")) {
        retValue = ANIM_STATE_ALT_GRENADE;
    }
    // this is the mutation melee "spear"
    else if (x_stristr(pAnimName, "_Spear")) {
        retValue = ANIM_STATE_MUTATION_SPEAR;
    }
    /////////////////////////////////////////
    // START -- Melee Section
    else if (x_stristr(pAnimName, "_Melee")) {
        retValue = ANIM_STATE_MELEE;
    } else if (x_stristr(pAnimName, "_AttackFromCenter")) {
        retValue = ANIM_STATE_MELEE_FROM_CENTER;
    } else if (x_stristr(pAnimName, "_AttackFromDown")) {
        retValue = ANIM_STATE_MELEE_FROM_DOWN;
    } else if (x_stristr(pAnimName, "_AttackFromLeft")) {
        retValue = ANIM_STATE_MELEE_FROM_LEFT;
    } else if (x_stristr(pAnimName, "_AttackFromRight")) {
        retValue = ANIM_STATE_MELEE_FROM_RIGHT;
    } else if (x_stristr(pAnimName, "_AttackFromUp")) {
        retValue = ANIM_STATE_MELEE_FROM_UP;
    }
    // END -- Melee Section
    /////////////////////////////////////////

    /////////////////////////////////////////
    // START -- Combo Section
    else if (x_stristr(pAnimName, "_ComboBegin")) {
        retValue = ANIM_STATE_COMBO_BEGIN;
    } else if (x_stristr(pAnimName, "_ComboHit")) {
        retValue = ANIM_STATE_COMBO_HIT;
    } else if (x_stristr(pAnimName, "_ComboEnd")) {
        retValue = ANIM_STATE_COMBO_END;
    }
    // END -- Combo Section
    /////////////////////////////////////////

    else if (x_stristr(pAnimName, "_Ramp_Up")) {
        retValue = ANIM_STATE_RAMP_UP;
    } else if (x_stristr(pAnimName, "_Ramp_Down")) {
        retValue = ANIM_STATE_RAMP_DOWN;
    } else if (x_stristr(pAnimName, "_Hold")) {
        retValue = ANIM_STATE_HOLD;
    } else if (x_stristr(pAnimName, "_AltHold")) {
        retValue = ANIM_STATE_ALT_HOLD;
    } else if (x_stristr(pAnimName, "_Run")) {
        retValue = ANIM_STATE_RUN;
    } else if (x_stristr(pAnimName, "_Death01")) {
        retValue = ANIM_STATE_DEATH;
    } else if (x_stristr(pAnimName, "_Mutation_")) {
        retValue = ANIM_STATE_CHANGE_MUTATION;
    } else if (x_stristr(pAnimName, "_AltRamp_Up")) {
        retValue = ANIM_STATE_ALT_RAMP_UP;
    } else if (x_stristr(pAnimName, "_AltRamp_Down")) {
        retValue = ANIM_STATE_ALT_RAMP_DOWN;
    } else if (x_stristr(pAnimName, "_ZoomIn")) {
        retValue = ANIM_STATE_ZOOM_IN;
    } else if (x_stristr(pAnimName, "_ZoomOut")) {
        retValue = ANIM_STATE_ZOOM_OUT;
    } else if (x_stristr(pAnimName, "_ZoomIdle")) {
        retValue = ANIM_STATE_ZOOM_IDLE;
    } else if (x_stristr(pAnimName, "_ZoomRun")) {
        retValue = ANIM_STATE_ZOOM_RUN;
    } else if (x_stristr(pAnimName, "_ZoomFire")) {
        retValue = ANIM_STATE_ZOOM_FIRE;
    } else {
        retValue = ANIM_STATE_UNDEFINED;
    }

    return retValue;
}

//===========================================================================
static float s_TEST_min_fall_vel = 400.f;
static float s_TEST_min_damage_vel = 1100.f;
static float s_TEST_volume_attenuation = 900.f;

void player::ProcessSfxEvents()
{
    // Did we jumped.
    if ((m_Physics.GetFallMode()) && (m_Physics.GetVelocity().y > 0.0f) && (m_PeakJumpVelocity == -1.0f)) {
        m_PeakJumpVelocity = m_Physics.GetVelocity().y;
        // IJB g_AudioMgr.PlayVolumeClipped("HumanMale_JumpGrunt", GetPosition(), GetZone1(), true);
    }

    if ((m_Physics.GetVelocity().y <= 0.0f) && !(m_Physics.GetFallMode())) {
        m_PeakJumpVelocity = -1.0f;
    }

    // Are we going to land.
    if ((m_Physics.GetFallMode()) && (m_Physics.GetVelocity().y < 0.0f)) {
        m_PeakLandVelocity = (m_Physics.GetVelocity().y) * -1.0f;
    } else if ((m_PeakLandVelocity != -1.0f) && !(m_Physics.GetFallMode())) {
        if (m_PeakLandVelocity > s_TEST_min_fall_vel) {

            if (m_PeakLandVelocity > s_TEST_min_damage_vel) {
                // IJB g_AudioMgr.PlayVolumeClipped("HumanMale_LandGrunt", GetPosition(), GetZone1(), true);
            }

            float ImpactVolume = m_PeakLandVelocity / s_TEST_volume_attenuation;

            if (ImpactVolume > 1.0f) {
                ImpactVolume = 1.0f;
            }

            m_PeakLandVelocity = -1.0f;
            /* IJB
                        int VoiceId = g_AudioMgr.PlayVolumeClipped(GetFootfallLandSweetner(GetFloorMaterial()), GetPosition(), GetZone1(), true);
                        g_AudioMgr.SetVolume(VoiceId, ImpactVolume);

                        g_AudioMgr.PlayVolumeClipped(GetFootfallHeel(GetFloorMaterial()), GetPosition(), GetZone1(), true);
            */
        }
    }
}

//===========================================================================
player::animation_state player::GetMotionTransitionAnimState()
{
    animation_state retState = ANIM_STATE_UNDEFINED;

    float VelocitySquared = m_Physics.GetVelocity().LengthSquared();
    bool  bIdle = VelocitySquared < m_fMinRunSpeed;

    if (bIdle) {
        retState = ANIM_STATE_IDLE;
    } else {
        //if we're transitioning from
        retState = ANIM_STATE_RUN;
    }

    return retState;
}

//==============================================================================

void player::SetAnimation(const animation_state& AnimState, const int& nAnimIndex, const float& fBlendTime)
{
    // increment our pain event ID whenever we change animations;
    m_CurrentPainEventID = pain_event::CurrentEventID++;
    if (pain_event::CurrentEventID >= INT_MAX) {
        pain_event::CurrentEventID = 0;
    }

    //Get a reference to the state that we are considering
    int          WeaponIndex = inventory2::ItemToWeaponIndex(m_CurrentWeaponItem);
    state_anims& State = m_Anim[WeaponIndex][AnimState];
    state_anims& WeaponState = m_Anim[WeaponIndex][AnimState];

    bool bResetFrame = (m_CurrentAnimStateIndex == nAnimIndex) ? true : false;

    m_PreviousAnimIndex = m_CurrentAnimIndex;
    m_CurrentAnimIndex = WeaponState.WeaponAnim[nAnimIndex];

    m_PreviousAnimStateIndex = m_CurrentAnimStateIndex;
    m_CurrentAnimStateIndex = nAnimIndex;

    if (State.nPlayerAnims > nAnimIndex && WeaponState.nWeaponAnims > nAnimIndex) {
        //set the animation in the player.
        m_AnimPlayer.SetAnim(State.PlayerAnim[nAnimIndex], true, true, fBlendTime, bResetFrame);

        switch (m_CurrentAnimState) {
        case ANIM_STATE_DEATH:  // Intentional fallthrough
        case ANIM_STATE_CINEMA: // Intentional fallthrough
        case ANIM_STATE_MISSION_FAILED:
            // do nothing
            break;
        default:
        {
            //set the animation for the weapon
            new_weapon* pWeapon = GetCurrentWeaponPtr();
            if (pWeapon) {
                pWeapon->SetAnimation(WeaponState.WeaponAnim[nAnimIndex], fBlendTime, bResetFrame);
            }
        }
        }
    }
}

//==============================================================================

void player::CameraFall(float fPercentHeight)
{
    float fHeight = GetBBox().min.y + ((GetBBox().GetSize().y + m_EyesOffset.y) * fPercentHeight) + 10.0f; //10 cm buffer
    m_PosOverrideCamera = Vector3(GetPosition().GetX(), fHeight, GetPosition().GetZ());
    MoveAnimPlayer(m_PosOverrideCamera);
}

//==============================================================================

void player::OnAnimEvents()
{
    g_EventMgr.HandleSuperEvents(m_AnimPlayer, this);
}

//===========================================================================
float s_FireShakeTime = 0.75f;
float s_FireShakeAmount = 0.9f;
float s_FireShakeSpeed = 1.0f;
float s_FireFeedbackDuration = 0.1f;
float s_FireFeedbackIntensity = 0.75f;

float s_AltFireShakeTime = 0.85f;
float s_AltFireShakeAmount = 1.4f;
float s_AltFireShakeSpeed = 1.0f;
float s_AltFireFeedbackDuration = 0.15f;
float s_AltFireFeedbackIntensity = 0.9f;

// how much (in degrees) do we offset the pitch so the JBG will start off higher.
tweak_handle JBEAN_PitchThrowOffsetTweak("JBEAN_PitchThrowOffset");

// this is the highest angle at which the pitch offset will apply.
tweak_handle JBEAN_PitchThrowAngleTweak("JBEAN_PitchThrowAngle");

void player::OnEvent(const event& Event)
{
    if (m_ActivePlayerPad == -1) {
        return;
    }

    if (Event.Type == event::EVENT_INTENSITY) {
        const intensity_event& IntensityEvent = intensity_event::GetSafeType(Event);

        DoFeedback(IntensityEvent.ControllerDuration, IntensityEvent.ControllerIntensity);
        ShakeView(IntensityEvent.CameraShakeTime, IntensityEvent.CameraShakeAmount, IntensityEvent.CameraShakeSpeed);
    }

    if (Event.Type == event::EVENT_GENERIC) {
        const generic_event& GenericEvent = generic_event::GetSafeType(Event);
        if (strcmp(GenericEvent.GenericType, "FP_Mutation_Switch") == 0) {
            int   WeaponIndex = inventory2::ItemToWeaponIndex(m_CurrentWeaponItem);
            int   nAnimIndex = m_Anim[WeaponIndex][ANIM_STATE_CHANGE_MUTATION].PlayerAnim[0];
            float nFrame = m_AnimPlayer.GetFrame();

            m_AnimPlayer.SetAnim(nAnimIndex, true, true, 0.f);
            m_AnimPlayer.SetFrame(nFrame);

            SetAnimState(ANIM_STATE_CHANGE_MUTATION);
            //          g_PostEffectMgr.AddToHowlBlur( 0.4f, 0.5f, 1.0f, .3f );
        } else if (strcmp(GenericEvent.GenericType, "Player_Death") == 0) {
            if ((m_CurrentAnimState == ANIM_STATE_DEATH) && (m_AnimStage > 1)) {
                const anim_event& Event = m_AnimPlayer.GetEvent(GenericEvent.EventIndex);
                // the timerange here is for falling
                float nTotalFramesForEvent = (float)Event.GetInt(anim_event::INT_IDX_END_FRAME) - Event.GetInt(anim_event::INT_IDX_START_FRAME);
                float nCurrentEventFrame = m_AnimPlayer.GetFrame() - Event.GetInt(anim_event::INT_IDX_START_FRAME);
                float fPercentHeight = ((nTotalFramesForEvent - nCurrentEventFrame) / nTotalFramesForEvent);
                CameraFall(fPercentHeight);
            }
            /* IJB
                    } else if (strcmp(GenericEvent.GenericType, "Spear_Out_Left") == 0) {
                        GetMutationMeleeWeapon()->FireTendril(GetEyesPosition(),
                                                              m_ForwardVelocity + m_StrafeVelocity,
                                                              GetProjectileTrajectory(),
                                                              GetGuid(), true);
                    } else if (strcmp(GenericEvent.GenericType, "Spear_Out_Right") == 0) {
                        GetMutationMeleeWeapon()->FireTendril(GetEyesPosition(),
                                                              m_ForwardVelocity + m_StrafeVelocity,
                                                              GetProjectileTrajectory(),
                                                              GetGuid(), false);
                    } else if (strcmp(GenericEvent.GenericType, "Spear_In_Left") == 0) {
                        GetMutationMeleeWeapon()->RetractTendril(true);
                    } else if (strcmp(GenericEvent.GenericType, "Spear_In_Right") == 0) {
                        GetMutationMeleeWeapon()->RetractTendril(false);
                    } else if (strcmp(GenericEvent.GenericType, "Mutant Vision") == 0) {
                        if (IsMutated()) {
                            m_bIsMutantVisionOn = true;
                        } else {
                            m_bIsMutantVisionOn = false;
                        }
                    }
                    // check for mutant melee stuff here
                    else if (x_stristr(GenericEvent.GenericType, "MeleeFrom")) {
                        GetMutationMeleeWeapon()->DoExtremeMelee();
            */
        } else if (x_stristr(GenericEvent.GenericType, "Combo_Start")) {
            // threshold start
            m_bCanRequestCombo = true;
        } else if (x_stristr(GenericEvent.GenericType, "Combo_End")) {
            // threshold timed out
            m_bCanRequestCombo = false;
        } else if (x_stristr(GenericEvent.GenericType, "CompletedReload")) {
            // reload sequence has gone far enough to count, the rest is fluff
            new_weapon* pWeapon = GetCurrentWeaponPtr();
            if (pWeapon) {
                pWeapon->SetReloadCompleted(true);
            }
        }
    } else if (Event.Type == event::EVENT_WEAPON) {
        const weapon_event& WeaponEvent = weapon_event::GetSafeType(Event);

        switch (WeaponEvent.WeaponState) {
        case new_weapon::EVENT_FIRE:
        case new_weapon::EVENT_FIRE_LEFT:
        case new_weapon::EVENT_FIRE_RIGHT:
        {
            // don't allow player to switch weapons, zoom in, attack, etc.
            if (m_bHidePlayerArms) {
                break;
            }

            int iFirePoint = -1;

            switch (WeaponEvent.WeaponState) {
            case new_weapon::EVENT_FIRE:
                iFirePoint = new_weapon::FIRE_POINT_DEFAULT;
                break;
            case new_weapon::EVENT_FIRE_LEFT:
                iFirePoint = new_weapon::FIRE_POINT_LEFT;
                break;
            case new_weapon::EVENT_FIRE_RIGHT:
                iFirePoint = new_weapon::FIRE_POINT_RIGHT;
                break;
            }

            new_weapon* pWeapon = GetCurrentWeaponPtr();

            if (pWeapon) {
                /* IJB
                new_weapon::reticle_radius_parameters ReticleParams = GetReticleParams();
                m_ReticleShotPenalty += ReticleParams.m_PenaltyForShot;
                const float MaxPenalty = (ReticleParams.m_MaxRadius - ReticleParams.m_MaxMovementPenalty) - ReticleParams.m_MinRadius;
                m_ReticleShotPenalty = std::min(MaxPenalty, m_ReticleShotPenalty);
                pWeapon->SetTarget(GetEnemyOnReticle());
                pWeapon->FireWeapon(GetEyesPosition(), m_ForwardVelocity + m_StrafeVelocity, m_WpnHoldTime, GetProjectileTrajectory(), GetGuid(), iFirePoint);
                */
            }

        } break;
        case new_weapon::EVENT_ALT_FIRE:
        case new_weapon::EVENT_ALT_FIRE_LEFT:
        case new_weapon::EVENT_ALT_FIRE_RIGHT:
        {
            // don't allow player to switch weapons, zoom in, attack, etc.
            if (m_bHidePlayerArms) {
                break;
            }

            int iFirePoint = -1;

            switch (WeaponEvent.WeaponState) {
            case new_weapon::EVENT_ALT_FIRE:
                iFirePoint = new_weapon::FIRE_POINT_DEFAULT;
                break;
            case new_weapon::EVENT_ALT_FIRE_LEFT:
                iFirePoint = new_weapon::FIRE_POINT_LEFT;
                break;
            case new_weapon::EVENT_ALT_FIRE_RIGHT:
                iFirePoint = new_weapon::FIRE_POINT_RIGHT;
                break;
            }

            new_weapon* pWeapon = GetCurrentWeaponPtr();
            /* IJB
            if (pWeapon) {
                new_weapon::reticle_radius_parameters ReticleParams = GetReticleParams();
                m_ReticleShotPenalty += ReticleParams.m_PenaltyForShot;
                const float MaxPenalty = (ReticleParams.m_MaxRadius - ReticleParams.m_MaxMovementPenalty) - ReticleParams.m_MinRadius;
                m_ReticleShotPenalty = std::min(MaxPenalty, m_ReticleShotPenalty);
                pWeapon->SetTarget(GetEnemyOnReticle());
                pWeapon->FireSecondary(GetEyesPosition(), m_ForwardVelocity + m_StrafeVelocity, m_WpnHoldTime, GetProjectileTrajectory(), GetGuid(), iFirePoint);
            }
                */
        } break;
        case new_weapon::EVENT_GRENADE:
        {
            // don't allow player to switch weapons, zoom in, attack, etc.
            if (m_bHidePlayerArms) {
                break;
            }

            tweak_handle SpeedTweak("PLAYER_GrenadeThrowSpeed");
            // Compute velocity
            Vector3 Dir = GetView().GetViewZ();
            // TODO: Tweak throw speed based on pitch of vector, less power when looking down, etc.
            Vector3 Velocity = Dir * SpeedTweak.GetF32();
            Velocity += m_ForwardVelocity + m_StrafeVelocity;

            pain_handle PainHandle(xfs("%s_GRENADE", GetLogicalName()));

            // which grenade do we throw?
            /* IJB
            if (m_CurrentGrenadeType2 == INVEN_GRENADE_FRAG) {
                // Create the Grenade projectile.
                guid                GrenadeID = objectManager->CreateObject(grenade_projectile::GetObjectType());
                grenade_projectile* pFragGrenade = (grenade_projectile*)objectManager->GetObjectByGuid(GrenadeID);

                // make sure the grenade was created.
                assert(pFragGrenade);

                // New Position
                Vector3 NewEventPos = SetupGrenadeThrow(WeaponEvent.Pos);

                pFragGrenade->Setup(GetGuid(),
                                    net_GetSlot(),
                                    NewEventPos,
                                    Radian3(0.0f, 0.0f, 0.0f),
                                    Velocity,
                                    GetZone1(),
                                    GetZone2(),
                                    PainHandle);

                m_Inventory2.RemoveAmount(m_CurrentGrenadeType2, 1.0f);
            }
                */
        } break;
        case new_weapon::EVENT_ALT_GRENADE:
        {
            // don't allow player to switch weapons, zoom in, attack, etc.
            if (m_bHidePlayerArms) {
                break;
            }

            pain_handle PainHandle(xfs("%s_JBEAN", GetLogicalName()));

            /* IJB
            // which grenade do we throw?
            if (m_CurrentGrenadeType2 == INVEN_GRENADE_JBEAN) {
                // Create the Jumping Bean Grenade projectile.
                guid                     GrenadeID = objectManager->CreateObject(jumping_bean_projectile::GetObjectType();
                jumping_bean_projectile* pJBeanGrenade = (jumping_bean_projectile*)objectManager->GetObjectByGuid(GrenadeID);

                // make sure the grenade was created.
                assert(pJBeanGrenade);

                if (!pJBeanGrenade) {
                    return;
                }

                float Speed = 0.0f;
                bool  bExpert = (pJBeanGrenade->GetGrenadeMode() == GM_EXPERT);

                if (bExpert) {
                    tweak_handle SpeedTweak("JBEAN_ThrowSpeed");
                    Speed = SpeedTweak.GetF32();
                } else {
                    tweak_handle SpeedTweak("JBEAN_ThrowSpeed_Normal");
                    Speed = SpeedTweak.GetF32();
                }

                // Compute velocity
                Vector3 Dir = GetView().GetViewZ();
                // TODO: Tweak throw speed based on pitch of vector, less power when looking down, etc.
                Vector3 Velocity = Dir * Speed;
                Velocity += m_ForwardVelocity + m_StrafeVelocity;

                if (bExpert) {
                    Radian Pitch, Yaw;
                    Velocity.GetPitchYaw(Pitch, Yaw);

                    Radian JBG_PitchAngle = JBEAN_PitchThrowAngleTweak.GetRadian();

                    // up to a point, rotate the way for a faked "lob" of the grenade
                    if (Pitch > JBG_PitchAngle) {
                        float T = 1.0f;

                        // if we're looking upwards, scale pitch offset
                        if (Pitch < R_0) {
                            T = Pitch / JBG_PitchAngle;

                            // since the values have to be backwards to work, must flip T
                            T = 1.0f - T;
                        }

                        Radian JBG_PitchOffset = JBEAN_PitchThrowOffsetTweak.GetRadian();
                        Pitch -= JBG_PitchOffset * T;
                        float Scalar = Velocity.Length();
                        Velocity.set(Pitch, Yaw);
                        Velocity.Scale(Scalar);
                    }
                }

                Vector3 NewEventPos = SetupGrenadeThrow(WeaponEvent.Pos);

                pJBeanGrenade->Setup(GetGuid(),
                                     net_GetSlot(),
                                     NewEventPos,
                                     Radian3(0.0f, 0.0f, 0.0f),
                                     Velocity,
                                     GetZone1(),
                                     GetZone2(),
                                     PainHandle);


                m_Inventory2.RemoveAmount(m_CurrentGrenadeType2, 1.0f);
            }
                */
        } break;

        default:
            break;
        }
    } else if (Event.Type == event::EVENT_PAIN) {
        const pain_event& PainSuperEvent = pain_event::GetSafeType(Event);

        // check if this is a melee pain and kick it off
        if ((PainSuperEvent.PainType == pain_event::EVENT_PAIN_MELEE)) {
            EmitMeleePain();
        }
    }
}

//=============================================================================
Vector3 player::SetupGrenadeThrow(const Vector3& EventPos)
{
    Vector3 NewEventPos = EventPos;

    // do some LOS checks here so we don't throw the grenade through the wall.
    {
        /*
        3
        /|
        / |
        /  |
        /   |
        /    |
        /     |
        1------2
        (arm)
        1 = eyes position
        2 = point perpendicular to eyes position
        3 = event pos

        To make sure we can throw the grenade at the event pos, we need to:
        ~ make sure that we can see point 2 from point 1
        AND, If you can see it then,
        ~ make sure that we can see point 3 from point 2
        if not, it failed right off

        This will ensure all bases are covered in events such as:

        =====================================================================
        (FAIL)
                    3           - Actual Event Position (3)
        --------------------    - Wall
                    N           - New event position (N)
             1------|2          - Player Eye Position (1) and point perpendicular to eyes position (2)
        (arm)

        * You can't see point 2 from put 1 and you can't see point 3 from point 2

        =====================================================================
        (FAIL)
                      (wall)
                         /
            3      _2  /N           - Actual Event Position (3) then New Event Position (N)
                    |/
                   /|               - Point perpendicular to eyes position (2)
                 /  | (arm)
               /    1               - Player Eye Position (1)

        * You can't see point 2 from point 1 even though you can see point 3 from point 2

        =====================================================================
        (PASS)

                3            - Actual Event Position (3)
        -------              - Wall
         1------|2           - Player Eye Position (1) and point perpendicular to eyes position (2)
        (arm)

        * You can see point 2 from point 1 and you can see point 3 from point 2
        =====================================================================
        */

        // get player position
        Vector3 Point1 = Vector3(0.0f, 0.0f, 0.0f);
        // not set yet
        Vector3 Point2 = Vector3(0.0f, 0.0f, 0.0f);
        // get event position
        Vector3 Point3 = NewEventPos;

        // load up our points for calculations
        GetThrowPoints(Point1, Point2, Point3);

        // ---------------------------------------------------------------------------------
        // Check point 1 to point 2 (player 'eye' position to 'shoulder')
        g_CollisionMgr.RaySetup(GetGuid(), // MovingObjGuid,
                                Point1,    // WorldStart,
                                Point2);   // WorldEnd,

        g_CollisionMgr.IgnoreGlass();

        g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES,
                                       Object::ATTR_BLOCKS_PLAYER_LOS,
                                       (Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));

        // we have collisions, this failed
        if (g_CollisionMgr.m_nCollisions) {
            // back off the wall a bit
            Vector3 theNormal = g_CollisionMgr.m_Collisions[0].Plane.Normal;
            NewEventPos = g_CollisionMgr.m_Collisions[0].Point + (theNormal * 3.5f);
        } else // clear from eyes to shoulder
        {
            // ---------------------------------------------------------------------------------
            // Check point 2 to point 3 ('shoulder' position to event position)
            g_CollisionMgr.RaySetup(GetGuid(), // MovingObjGuid,
                                    Point2,    // WorldStart,
                                    Point3);   // WorldEnd,

            g_CollisionMgr.IgnoreGlass();

            g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES,
                                           Object::ATTR_BLOCKS_LARGE_PROJECTILES,
                                           (Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));

            // if we have collisions, pick new point
            if (g_CollisionMgr.m_nCollisions) {
                // back off the wall a bit
                Vector3 theNormal = g_CollisionMgr.m_Collisions[0].Plane.Normal;
                NewEventPos = g_CollisionMgr.m_Collisions[0].Point + (theNormal * 3.5f);
            }
        }
    }

    return NewEventPos;
}

//=============================================================================
void player::GetThrowPoints(Vector3& Point1, Vector3& Point2, Vector3& Point3)
{
    // get the player position and raise the "eyes" up to the event pos.
    Point1 = GetPosition();
    Point1.y = Point3.y;

    // get our view vector (normalized)
    Vector3 ViewZ = GetView().GetViewZ();

    // get the vector closest to the line segment from player position to event position
    Vector3 Closest = ViewZ.Cross(Point3 - Point1);

    // get distance between player position and event position
    float d = Closest.Length();

    // get the lean
    Vector3 lean = (GetView().GetViewX());

    if (m_CurrentGrenadeType2 == INVEN_GRENADE_FRAG) {
        d = -d;
    }

    // this is our "shoulder" position
    Point2 = Point1 + (lean * d); // perpendicular position
}

//=============================================================================
//===========================================================================
// Need to use some anti mu?

bool player::UseAntiMu(collectable_anti_mutagen* pAntiMu)
{
    return false;
}

//===========================================================================

void player::SetNonExclusiveState(non_exclusive_states nStateBit)
{
    // don't set the state if we're already in it
    if (m_NonExclusiveStateBitFlag & nStateBit) {
        return;
    }

    m_NonExclusiveStateBitFlag |= nStateBit;
    BeginNonExclusiveState(nStateBit);
}

//===========================================================================

void player::ClearNonExclusiveState(non_exclusive_states nStateBit)
{
    if (nStateBit & m_NonExclusiveStateBitFlag) {
        EndNonExclusiveState(nStateBit);
        m_NonExclusiveStateBitFlag &= ~nStateBit;
    }
}

//===========================================================================
// NOTE:  When new non exclusive states are added, you must clear them here.
void player::ClearAllNonExclusiveStates()
{
    ClearNonExclusiveState(NE_STATE_STUNNED);
}

//===========================================================================

void player::BeginNonExclusiveState(non_exclusive_states nStateBit)
{
    switch (nStateBit) {
    case NE_STATE_STUNNED:
        BeginStunnedNE();
        break;
    default:
        break;
    }
}

//===========================================================================

void player::UpdateActiveNonExclusiveStates(float DeltaTime)
{
    if (m_NonExclusiveStateBitFlag & NE_STATE_STUNNED) {
        UpdateStunnedNE(DeltaTime);
    }
}

//===========================================================================

void player::EndNonExclusiveState(non_exclusive_states nStateBit)
{
    switch (nStateBit) {
    case NE_STATE_STUNNED:
        EndStunnedNE();
        break;
    default:
        break;
    }
}

//===========================================================================

void player::ProcessStunnedPain(const pain& Pain)
{
    (void)Pain;
    SetNonExclusiveState(NE_STATE_STUNNED);
}

//===========================================================================

void player::BeginStunnedNE()
{
    // Get the current rotation from the view
    m_fStunnedTime = 0.f;

    GetEyesPitchYaw(m_PreStunPitch, m_PreStunYaw);
}

//===========================================================================
void player::UpdateStunnedNE(float DeltaTime)
{
    m_fStunnedTime += DeltaTime;

    float YawRotFactor = m_fStunnedTime * m_fStunYawChangeSpeed;
    float PitchRotFactor = m_fStunnedTime * m_fStunPitchChangeSpeed;

    float YawOffset = sin(YawRotFactor);
    float PitchOffset = sin(PitchRotFactor);
    YawOffset *= m_MaxStunPitchOffset;
    PitchOffset *= m_MaxStunYawOffset;

    m_AnimPlayer.SetPitch(m_PreStunPitch + PitchOffset);
    m_AnimPlayer.SetYaw(m_PreStunYaw + YawOffset);

    if (!m_bInTurret) {
        m_Physics.Advance(m_Physics.GetPosition(), DeltaTime);
        OnMove(m_Physics.GetPosition());
    }

    if (m_fStunnedTime >= m_fMaxStunTime) {
        ClearNonExclusiveState(NE_STATE_STUNNED);
    }
}

//===========================================================================

void player::EndStunnedNE()
{
}

//==============================================================================

void player::PlayFootfall(float DeltaTime)
{
    //not the active player, don't play footfalls
    if (!IsActivePlayer()) {
        return;
    }

    // Use the player velocity.
    float XVel = (m_MaxStrafeVelocity * m_fStrafeValue) / m_MaxStrafeVelocity;
    float YVel = (m_MaxFowardVelocity * m_fMoveValue) / m_MaxFowardVelocity;

    float AbsYVel = abs(YVel);
    float AbsXVel = abs(XVel);
    float ComboVel = sqrt(x_sqr(YVel) + x_sqr(XVel));

    float CurrentVel = kMaxForwardVel * ComboVel;
    float FeetsPerStep = (kFeetsPerInitStep + (ComboVel * kFeetsSpeedMod));
    float MeterPerStep = FeetsPerStep * 0.3048f;

    m_DelayTillNextStep -= (DeltaTime * 1000.0f);
    if (m_DelayTillNextStep < 0.0f) {
        m_DistanceTraveled = m_DistanceTraveled + (CurrentVel * DeltaTime);
    }
    m_IsRunning = false;

    // Don't pitch down the footfalls...
    float Pitch = 1.0f / g_PerceptionMgr.GetAudioTimeDialation();

    // Going forward.
    if (YVel >= 0.0f) {
        if (m_DistanceTraveled > MeterPerStep) {
            // Get the delay till toe hit.
            m_DelayCountDown = kForwardDelay * (((kFalloff - ComboVel) < 0.0f) ? 0.0f : (kFalloff - ComboVel) / kFalloff);
            /* IJB
                        // Play the heel sound and set the volume level depending on the speed.
                        m_HeelID = g_AudioMgr.Play(GetFootfallHeel(GetFloorMaterial()), GetPosition(), GetZone1(), true);
                        g_AudioMgr.SetPitch(m_HeelID, Pitch);
                        g_AudioManager.NewAudioAlert(m_HeelID, audio_manager::FOOT_STEP, GetPosition(), GetZone1(), GetGuid());

                        float Volume = 0.0f;
                        if ((ComboVel * kMaxForwardVel) < kMaxWalkVel || m_bIsCrouching) {
                            Volume = kLowestVolume + (kMaxWalkVolume - kLowestVolume) * std::min(ComboVel, kMaxWalkVolume);
                        } else {
                            m_IsRunning = true;
                            Volume = kMinRunVolume + (kMaxRunVolume - kMinRunVolume) * std::min(ComboVel, kMaxRunVolume);
                        }

                        g_AudioMgr.SetVolume(m_HeelID, Volume);

                        if (AbsXVel > kStrafeInit) {
                            m_SlideID = g_AudioMgr.Play(GetFootfallSlide(GetFloorMaterial()));
                            g_AudioMgr.SetPitch(m_SlideID, Pitch);
                        }

                        float StrafeVolume = std::max(AbsXVel - kStrafeInit, 0.0f) * (1.0f / kStrafeInit);
                        StrafeVolume = StrafeVolume * (AbsYVel > kVertStrafeCutOff) ? 0.0f : ((kVertStrafeCutOff - AbsYVel) / kVertStrafeCutOff);
                        g_AudioMgr.SetVolume(m_SlideID, StrafeVolume);
            */
            m_DistanceTraveled = 0;
        }

        if (m_HeelID && ((AbsYVel < kFalloff) || (AbsXVel > kStrafeInit))) {
            m_DelayCountDown -= (DeltaTime * 1000.0f);

            if (m_DelayCountDown < 0.0f) {
                float Volume = kLowestVolume + (1.0f - kLowestVolume) * (std::min(ComboVel, 1.0f) * (1.0f / kFalloff));
/* IJB
                m_ToeID = g_AudioMgr.PlayVolumeClipped(GetFootfallToe(GetFloorMaterial()), GetPosition(), GetZone1(), true);
                g_AudioMgr.SetPitch(m_ToeID, Pitch);
                g_AudioMgr.SetVolume(m_ToeID, Volume);
*/
                m_HeelID = 0;

                m_TrailStep ^= (1 << 0);
            }
        }

        if ((AbsXVel >= AbsYVel) && m_TrailStep && (m_DelayTillNextStep <= 0.0f)) {
            m_DelayTillNextStep = ((kMaxStrafeDelay - kMinStrafeDelay) * (1.0f - AbsXVel)) + kMinStrafeDelay;
        } else {
            m_DelayTillNextStep = 0.0f;
        }

    }
    // Going backwards.
    else {
        if (m_DistanceTraveled > MeterPerStep) {
            if ((AbsYVel < kFalloff) || (AbsXVel > kStrafeInit)) {
                float Volume = kLowestVolume + (1.0f - kLowestVolume) * (AbsYVel * (1.0f / kFalloff));
                /* IJB
                m_ToeID = g_AudioMgr.PlayVolumeClipped(GetFootfallToe(GetFloorMaterial()), GetPosition(), GetZone1(), true);
                g_AudioMgr.SetPitch(m_ToeID, Pitch);
                g_AudioMgr.SetVolume(m_ToeID, Volume);
                */
            }

            m_TrailStep ^= (1 << 0);

            m_HeelID = 0;
            m_DelayCountDown = kBackwardDelay * (((kFalloff - ComboVel) < 0.0f) ? 0.0f : (kFalloff - ComboVel) / kFalloff);
            m_DistanceTraveled = 0;
        }

        if (!m_HeelID) {
            m_DelayCountDown -= (DeltaTime * 1000.0f);

            if (m_DelayCountDown < 0.0f) {
                float Volume = 0.0f;
                if ((ComboVel * kMaxForwardVel) < kMaxWalkVel) {
                    Volume = kLowestVolume + (kMaxWalkVolume - kLowestVolume) * std::min(ComboVel, kMaxWalkVolume);
                } else {
                    Volume = kMinRunVolume + (kMaxRunVolume - kMinRunVolume) * std::min(ComboVel, kMaxRunVolume);
                }
                /* IJB
                                m_HeelID = g_AudioMgr.Play(GetFootfallHeel(GetFloorMaterial()), GetPosition(), GetZone1(), true);
                                g_AudioManager.NewAudioAlert(m_HeelID, audio_manager::FOOT_STEP, GetPosition(), GetZone1(), GetGuid());
                                g_AudioMgr.SetPitch(m_HeelID, Pitch);
                                g_AudioMgr.SetVolume(m_HeelID, Volume);

                                if (AbsXVel > kStrafeInit) {
                                    m_SlideID = g_AudioMgr.Play(GetFootfallSlide(GetFloorMaterial()));
                                    g_AudioMgr.SetPitch(m_SlideID, Pitch);
                                }

                                float StrafeVolume = std::max(AbsXVel - kStrafeInit, 0.0f) * (1.0f / kStrafeInit);
                                StrafeVolume = StrafeVolume * (AbsYVel > kVertStrafeCutOff) ? 0.0f : ((kVertStrafeCutOff - AbsYVel) / kVertStrafeCutOff);
                                g_AudioMgr.SetVolume(m_SlideID, StrafeVolume);
                                */
            }
        }

        if ((AbsXVel >= AbsYVel) && m_TrailStep && (m_DelayTillNextStep <= 0.0f)) {
            m_DelayTillNextStep = ((kMaxStrafeDelay - kMinStrafeDelay) * (1.0f - AbsXVel)) + kMinStrafeDelay;
        } else {
            m_DelayTillNextStep = 0.0f;
        }
    }
}

const char* player::GetFootfallHeel(int Material)
{
    switch (Material) {
    case MAT_TYPE_NULL:
        return "FF_Boot_Null_Heel";
        break;
    case MAT_TYPE_EARTH:
        return "FF_Boot_Earth_Heel";
        break;
    case MAT_TYPE_ROCK:
        return "FF_Boot_Rock_Heel";
        break;
    case MAT_TYPE_CONCRETE:
        return "FF_Boot_Concrete_Heel";
        break;
    case MAT_TYPE_SOLID_METAL:
        return "FF_Boot_Metal_Heel";
        break;
    case MAT_TYPE_HOLLOW_METAL:
        return "FF_Boot_HollowMetal_Heel";
        break;
    case MAT_TYPE_METAL_GRATE:
        return "FF_Boot_MetalGrate_Heel";
        break;
    case MAT_TYPE_PLASTIC:
        return "FF_Boot_Plastic_Heel";
        break;
    case MAT_TYPE_WATER:
        return "FF_Boot_Water_Heel";
        break;
    case MAT_TYPE_WOOD:
        return "FF_Boot_Wood_Heel";
        break;
    case MAT_TYPE_ENERGY_FIELD:
        return "FF_Boot_EnergyField_Heel";
        break;
    case MAT_TYPE_BULLET_PROOF_GLASS:
        return "FF_Boot_BulletProofGlass_Heel";
        break;
    case MAT_TYPE_ICE:
        return "FF_Boot_Ice_Heel";
        break;

    case MAT_TYPE_LEATHER:
        return "FF_Boot_Leather_Heel";
        break;
    case MAT_TYPE_EXOSKELETON:
        return "FF_Boot_Exoskeleton_Heel";
        break;
    case MAT_TYPE_FLESH:
        return "FF_Boot_Flesh_Heel";
        break;
    case MAT_TYPE_BLOB:
        return "FF_Boot_Blob_Heel";
        break;

    case MAT_TYPE_FIRE:
        return "FF_Boot_Fire_Heel";
        break;
    case MAT_TYPE_GHOST:
        return "FF_Boot_Ghost_Heel";
        break;
    case MAT_TYPE_FABRIC:
        return "FF_Boot_Fabric_Heel";
        break;
    case MAT_TYPE_CERAMIC:
        return "FF_Boot_Ceramic_Heel";
        break;
    case MAT_TYPE_WIRE_FENCE:
        return "FF_Boot_WireFence_Heel";
        break;

    case MAT_TYPE_GLASS:
        return "FF_Boot_Glass_Heel";
        break;
    default:
        return "Null";
        break;
    }
}

//===========================================================================

const char* player::GetFootfallToe(int Material)
{
    switch (Material) {
    case MAT_TYPE_NULL:
        return "FF_Boot_Null_Toe";
        break;
    case MAT_TYPE_EARTH:
        return "FF_Boot_Earth_Toe";
        break;
    case MAT_TYPE_ROCK:
        return "FF_Boot_Rock_Toe";
        break;
    case MAT_TYPE_CONCRETE:
        return "FF_Boot_Concrete_Toe";
        break;
    case MAT_TYPE_SOLID_METAL:
        return "FF_Boot_Metal_Toe";
        break;
    case MAT_TYPE_HOLLOW_METAL:
        return "FF_Boot_HollowMetal_Toe";
        break;
    case MAT_TYPE_METAL_GRATE:
        return "FF_Boot_MetalGrate_Toe";
        break;
    case MAT_TYPE_PLASTIC:
        return "FF_Boot_Plastic_Toe";
        break;
    case MAT_TYPE_WATER:
        return "FF_Boot_Water_Toe";
        break;
    case MAT_TYPE_WOOD:
        return "FF_Boot_Wood_Toe";
        break;
    case MAT_TYPE_ENERGY_FIELD:
        return "FF_Boot_EnergyField_Toe";
        break;
    case MAT_TYPE_BULLET_PROOF_GLASS:
        return "FF_Boot_BulletProofGlass_Toe";
        break;
    case MAT_TYPE_ICE:
        return "FF_Boot_Ice_Toe";
        break;

    case MAT_TYPE_LEATHER:
        return "FF_Boot_Leather_Toe";
        break;
    case MAT_TYPE_EXOSKELETON:
        return "FF_Boot_Exoskeleton_Toe";
        break;
    case MAT_TYPE_FLESH:
        return "FF_Boot_Flesh_Toe";
        break;
    case MAT_TYPE_BLOB:
        return "FF_Boot_Blob_Toe";
        break;

    case MAT_TYPE_FIRE:
        return "FF_Boot_Fire_Toe";
        break;
    case MAT_TYPE_GHOST:
        return "FF_Boot_Ghost_Toe";
        break;
    case MAT_TYPE_FABRIC:
        return "FF_Boot_Fabric_Toe";
        break;
    case MAT_TYPE_CERAMIC:
        return "FF_Boot_Ceramic_Toe";
        break;
    case MAT_TYPE_WIRE_FENCE:
        return "FF_Boot_WireFence_Toe";
        break;

    case MAT_TYPE_GLASS:
        return "FF_Boot_Glass_Toe";
        break;
    default:
        return "Null";
        break;
    }
}

//===========================================================================

const char* player::GetFootfallLandSweetner(int Material)
{
    switch (Material) {
    case MAT_TYPE_NULL:
        return "FF_Boot_Land_Sweetner_Null";
        break;
    case MAT_TYPE_EARTH:
        return "FF_Boot_Land_Sweetner_Earth";
        break;
    case MAT_TYPE_ROCK:
        return "FF_Boot_Land_Sweetner_Rock";
        break;
    case MAT_TYPE_CONCRETE:
        return "FF_Boot_Land_Sweetner_Concrete";
        break;
    case MAT_TYPE_SOLID_METAL:
        return "FF_Boot_Land_Sweetner_Metal";
        break;
    case MAT_TYPE_HOLLOW_METAL:
        return "FF_Boot_Land_Sweetner_HollowMetal";
        break;
    case MAT_TYPE_METAL_GRATE:
        return "FF_Boot_Land_Sweetner_MetalGrate";
        break;
    case MAT_TYPE_PLASTIC:
        return "FF_Boot_Land_Sweetner_Plastic";
        break;
    case MAT_TYPE_WATER:
        return "FF_Boot_Land_Sweetner_Water";
        break;
    case MAT_TYPE_WOOD:
        return "FF_Boot_Land_Sweetner_Wood";
        break;
    case MAT_TYPE_ENERGY_FIELD:
        return "FF_Boot_Land_Sweetner_EnergyField";
        break;
    case MAT_TYPE_BULLET_PROOF_GLASS:
        return "FF_Boot_Land_Sweetner_BulletProofGlass";
        break;
    case MAT_TYPE_ICE:
        return "FF_Boot_Land_Sweetner_Ice";
        break;

    case MAT_TYPE_LEATHER:
        return "FF_Boot_Land_Sweetner_Leather";
        break;
    case MAT_TYPE_EXOSKELETON:
        return "FF_Boot_Land_Sweetner_Exoskeleton";
        break;
    case MAT_TYPE_FLESH:
        return "FF_Boot_Land_Sweetner_Flesh";
        break;
    case MAT_TYPE_BLOB:
        return "FF_Boot_Land_Sweetner_Blob";
        break;

    case MAT_TYPE_FIRE:
        return "FF_Boot_Land_Sweetner_Fire";
        break;
    case MAT_TYPE_GHOST:
        return "FF_Boot_Land_Sweetner_Ghost";
        break;
    case MAT_TYPE_FABRIC:
        return "FF_Boot_Land_Sweetner_Fabric";
        break;
    case MAT_TYPE_CERAMIC:
        return "FF_Boot_Land_Sweetner_Ceramic";
        break;
    case MAT_TYPE_WIRE_FENCE:
        return "FF_Boot_Land_Sweetner_WireFence";
        break;

    case MAT_TYPE_GLASS:
        return "FF_Boot_Land_Sweetner_Glass";
        break;
    default:
        return "FF_Boot_Land_Sweetner_Null";
        break;
    }
}

//===========================================================================

const char* player::GetFootfallSlide(int Material)
{
    switch (Material) {
    case MAT_TYPE_NULL:
        return "FF_Boot_Null_Slide";
        break;
    case MAT_TYPE_EARTH:
        return "FF_Boot_Earth_Slide";
        break;
    case MAT_TYPE_ROCK:
        return "FF_Boot_Rock_Slide";
        break;
    case MAT_TYPE_CONCRETE:
        return "FF_Boot_Concrete_Slide";
        break;
    case MAT_TYPE_SOLID_METAL:
        return "FF_Boot_Metal_Slide";
        break;
    case MAT_TYPE_HOLLOW_METAL:
        return "FF_Boot_HollowMetal_Slide";
        break;
    case MAT_TYPE_METAL_GRATE:
        return "FF_Boot_MetalGrate_Slide";
        break;
    case MAT_TYPE_PLASTIC:
        return "FF_Boot_Plastic_Slide";
        break;
    case MAT_TYPE_WATER:
        return "FF_Boot_Water_Slide";
        break;
    case MAT_TYPE_WOOD:
        return "FF_Boot_Wood_Slide";
        break;
    case MAT_TYPE_ENERGY_FIELD:
        return "FF_Boot_EnergyField_Slide";
        break;
    case MAT_TYPE_BULLET_PROOF_GLASS:
        return "FF_Boot_BulletProofGlass_Slide";
        break;
    case MAT_TYPE_ICE:
        return "FF_Boot_Ice_Slide";
        break;

    case MAT_TYPE_LEATHER:
        return "FF_Boot_Leather_Slide";
        break;
    case MAT_TYPE_EXOSKELETON:
        return "FF_Boot_Exoskeleton_Slide";
        break;
    case MAT_TYPE_FLESH:
        return "FF_Boot_Flesh_Slide";
        break;
    case MAT_TYPE_BLOB:
        return "FF_Boot_Blob_Slide";
        break;

    case MAT_TYPE_FIRE:
        return "FF_Boot_Fire_Slide";
        break;
    case MAT_TYPE_GHOST:
        return "FF_Boot_Ghost_Slide";
        break;
    case MAT_TYPE_FABRIC:
        return "FF_Boot_Fabric_Slide";
        break;
    case MAT_TYPE_CERAMIC:
        return "FF_Boot_Ceramic_Slide";
        break;
    case MAT_TYPE_WIRE_FENCE:
        return "FF_Boot_WireFence_Slide";
        break;

    case MAT_TYPE_GLASS:
        return "FF_Boot_Glass_Slide";
        break;
    default:
        return "Null";
        break;
    }
}

//===========================================================================

bool player::DoFootfallCollisions()
{
    if (m_Physics.GetJumpMode() || m_Physics.GetFallMode()) {
        return false;
    }

    return true;
}

//==============================================================================
bool player::IsAnimStateAvailable2(inven_item WeaponItem, animation_state AnimState)
{
    // Get a reference to the state that we are considering
    int          WeaponIndex = inventory2::ItemToWeaponIndex(WeaponItem);
    state_anims& State = m_Anim[WeaponIndex][AnimState];

    if ((State.nWeaponAnims > 0) && (State.nPlayerAnims > 0)) {
        return true;
    }

    return false;
}

//==============================================================================

bool player::AddHealth(float DeltaHealth)
{
    // Check to see if the player took damage
    if (DeltaHealth < 0.0f) {
        m_LastTimeTookDamage = (float)x_GetTimeSec();
    }

    return actor::AddHealth(DeltaHealth);
}

//==============================================================================
bool player::RenderSniperZoom()
{
    return (m_CurrentWeaponItem == INVEN_WEAPON_SNIPER_RIFLE) && m_bActivePlayer && ((m_CurrentAnimState == ANIM_STATE_ZOOM_IDLE) || (m_CurrentAnimState == ANIM_STATE_ZOOM_RUN) || (m_CurrentAnimState == ANIM_STATE_ZOOM_FIRE));
}

//=============================================================================
bool player::SetMutated(bool bMutate)
{
    if (actor::SetMutated(bMutate)) {
        if (bMutate) {
            if ((GetMutagen() <= FLT_MIN) && !m_bDead) {
                m_bIsMutated = false;

                // this failed
                return false;
            }
        }
        m_bIsMutated = bMutate;
        return true;
    } else {
        return false;
    }
}

//=============================================================================

void player::SetupMutationChange(bool bMutate)
{
    bool bIsMutated = m_bIsMutated;
    if (bIsMutated == bMutate) {
        return;
    }

    //  LOG_MESSAGE( "player::SetupMutationChange", "Mutate:%d", bMutate );

    if (bMutate) {
        // can we mutate?
        if (SetMutated(true)) {
            m_PreMutationWeapon2 = m_CurrentWeaponItem;
            SetNextWeapon2(INVEN_WEAPON_MUTATION);
            //  LOG_MESSAGE( "player::SetupMutationChange", "SetNextWeapon2( %s )", inventory2::ItemToName( INVEN_WEAPON_MUTATION ) );
            // IJB m_MutationAudioLoopSfx = g_AudioMgr.Play("Mutation_Vision_Loop");
            g_PerceptionMgr.BeginMutate();
        }

        // turn off flashlight when mutating
        SetFlashlightActive(false);
    } else {
        g_PerceptionMgr.EndMutate();
        static const float FADE_TIME = 0.25f;
        // IJB g_AudioMgr.Release(m_MutationAudioLoopSfx, FADE_TIME);
        // see if our previous weapon is in inventory
        if (m_Inventory2.HasItem(m_PreMutationWeapon2) &&
            (m_PreMutationWeapon2 != INVEN_WEAPON_MUTATION)) {
            SetMutated(false);
            SetNextWeapon2(m_PreMutationWeapon2);
            //  LOG_MESSAGE( "player::SetupMutationChange", "SetNextWeapon2( %s )", inventory2::ItemToName( m_PreMutationWeapon2 ) );
        } else {
            // see if we have another weapon in inventory
            int i;
            for (i = 0; i < INVEN_NUM_WEAPONS; ++i) {
                if (inventory2::WeaponIndexToItem(i) == INVEN_WEAPON_MUTATION) {
                    continue;
                } else if (m_Inventory2.HasItem(inventory2::WeaponIndexToItem(i))) {
                    SetMutated(false);
                    SetNextWeapon2(inventory2::WeaponIndexToItem(i));
                }
            }
        }
    }
}

//=============================================================================

float MPMutagenBurn = 1.0f;

void player::UpdateMutagen(float DeltaTime)
{
    // if we are playing a cinematic, don't burn mutagen
    if (m_Cinema.m_bCinemaOn) {
        return;
    }

    float AmountToChange = 0.0f; // If this is negative, for instance, when in mutant form in campaign mode, then it acts as a burn.
    float pct = 1.0f;

    switch (GetMutagenBurnMode()) {
    case MBM_AT_WILL:
    {
        pct = IsMutated() ? MutagenChangeMutant_AtWill_Tweak.GetF32() : MutagenChangeHuman_AtWill_Tweak.GetF32();
        pct = pct / 100.0f; // make it a percentage, tweaks are whole numbers.
        AmountToChange = (pct * GetMaxMutagen() * DeltaTime);
        AmountToChange *= MPMutagenBurn;
    } break;

    case MBM_FORCED:
    {
        pct = IsMutated() ? MutagenChangeMutant_Forced_Tweak.GetF32() : MutagenChangeHuman_Forced_Tweak.GetF32();
        pct = pct / 100.0f; // make it a percentage, tweaks are whole numbers.
        AmountToChange = (pct * GetMaxMutagen() * DeltaTime);
    } break;

    default:
    case MBM_NORMAL_CAMPAIGN:
    {
        pct = IsMutated() ? MutagenChangeMutant_Campaign_Tweak.GetF32() : MutagenChangeHuman_Campaign_Tweak.GetF32();
        pct = pct / 100.0f; // make it a percentage, tweaks are whole numbers.
        AmountToChange = (pct * GetMaxMutagen() * DeltaTime);
    } break;
    }

    // this is removing mutagen
    if (AmountToChange < 0.0f) {
        // see if the change takes us below 0 (remember AmountToChange is negative here so we have to add)
        if ((GetMutagen() + AmountToChange) < 0.0f) {
            // only burn as much as we have, otherwise the sanity check in AddMutagen won't let this happen
            AmountToChange = -GetMutagen();
        }
    }
    // this will only remove mutagen if AmountToChange is negative.
    AddMutagen(AmountToChange);

    if (IsMutated()) {
        // We don't burn mutagen if we have unlimited ammo, so, we should never run out.

            // check if we need to de-mutate
            if (GetMutagen() < FLT_MIN) {
                if (m_bInMutationTutorial) {
                    pain_handle PainHandle("GENERIC_LETHAL");
                    pain        Pain(objectManager);
                    Pain.Setup("GENERIC_LETHAL", 0, GetPosition());
                    Pain.SetDirectHitGuid(GetGuid());
                    Pain.ApplyToObject(this);
                } else {
                    SetupMutationChange(false);
                }
            }
        
    }
}

//=============================================================================
// Begin code from ghost.cpp
//==============================================================================

Vector3 player::GetPositionWithOffset(eOffsetPos offset)
{
    switch (offset) {
    case OFFSET_NONE:
        return GetPosition();
        break;
    case OFFSET_CENTER:
        return GetBBox().GetCenter();
        break;
    case OFFSET_AIM_AT:
        return GetBBox().GetCenter() + GetLeanOffset();
        break;
    case OFFSET_EYES:
        return GetEyesPosition();
        break;
    case OFFSET_TOP_OF_BBOX:
        return GetPosition() + Vector3(0.0f, GetBBox().max.y, 0.0f);
        break;
    default:
        return GetPosition();
        break;
    }
}

//==============================================================================

bool player::AddMutagen(const float& nDeltaMutagen)
{
    if (m_bDead) {
        return false;
    }
    // do not allow Mutagen to go above max.
    else if (m_Mutagen == m_MaxMutagen && nDeltaMutagen > 0.0f) {
        return false;
    } else if ((m_Mutagen + nDeltaMutagen) < 0.0f) // does what we are using take us below 0?
    {
        // don't have enough
        return false;
    } else if (m_bInMutationTutorial && ((m_Mutagen + nDeltaMutagen) < 1.0f)) {
        return false;
    } else if (m_Cinema.m_bCinemaOn) {
        return false;
    } else {
        // add/subtract mutagen
        m_Mutagen = std::min(m_Mutagen + nDeltaMutagen, m_MaxMutagen);
        m_Mutagen = std::max(m_Mutagen, 0.0f);

        return true;
    }

    return false;
}

//==============================================================================

const Matrix4& player::GetL2W() const
{
    Matrix4&      L2W = *(Matrix4*)(&actor::GetL2W()); // de-constification
    const Vector3 Pos(GetPosition());
    L2W.Identity();
    L2W.RotateY(m_Yaw);
    L2W.Translate(Pos);
    return L2W;
}

BBox player::GetLocalBBox() const
{
    BBox bb = m_Physics.GetBBox();

    // Take lean into account so leaning ghosts/players can be hit in MP
    float LeanDist = abs(GetLeanAmount() * 100.0f);
    bb.Inflate(LeanDist, 0.0f, LeanDist);

    return bb;
}

//==============================================================================

BBox player::GetColBBox()
{
    // Start with physics bbox
    BBox bb = m_Physics.GetBBox();

    // Take lean into account so leaning ghosts/players can be hit in MP
    float LeanDist = abs(GetLeanAmount() * 100.0f);
    bb.Inflate(LeanDist, 0.0f, LeanDist);

    // Convert into world space
    bb.Transform(GetL2W());
    return bb;
}

//==============================================================================

void player::OnColCheck()
{
    Vector3 Pos = GetPosition();
    Vector3 Offset(GetLeanOffset());
    Offset.y = 0.0f;
    Pos += Offset;

    Vector3 SpherePos[16];
    g_CollisionMgr.StartApply(GetGuid());

    int nSpheres = g_CollisionMgr.GetCylinderSpherePositions(
        Pos,
        Pos + Vector3(0, m_Physics.GetColHeight(), 0),
        m_Physics.GetColRadius(),
        SpherePos,
        Object::MAT_TYPE_FLESH);

    for (int i = 0; i < nSpheres; i++) {
        g_CollisionMgr.ApplySphere(SpherePos[i], m_Physics.GetColRadius(), Object::MAT_TYPE_FLESH);
    }

    g_CollisionMgr.EndApply();
}

void player::TakeFallPain()
{
    const float CurrentAltitude = GetPosition().y;
    const float FallDist = m_FellFromAltitude - CurrentAltitude;

    if (FallDist > m_SafeFallAltitude) {
        // Get parametric fall distance where 0=safe, 1=dead
        float T = x_parametric(FallDist, m_SafeFallAltitude, m_DeathFallAltitude, true);

        // Build a pain event to describe damage and apply to player
        pain Pain(objectManager);
        Pain.Setup(xfs("%s_FALL_DAMAGE", GetLogicalName()), 0, GetPosition());
        Pain.SetCustomScalar(T);
        Pain.SetDirectHitGuid(GetGuid());
        Pain.ApplyToObject(GetGuid());

        // reset fall altitude
        m_FellFromAltitude = GetPosition().y;
    }
}

//==============================================================================

bool player::AddItemToInventory2(inven_item Item)
{
    bool bFirstTimePickedUp = false;

    // if we're dead, don't add this to our inventory.
    if (IsDead()) {
        return false;
    }

    // is this the first time they've picked up this weapon on this level
    if (!m_Inventory2.HasItem(Item)) {
        bFirstTimePickedUp = true;
    }

    bool ItemAdded = actor::AddItemToInventory2(Item);

    if (ItemAdded && inventory2::IsAWeapon(Item)) {
        // Adding a weapon -> Determine if we should switch to the new one
        if (ShouldSwitchToWeapon2(Item, bFirstTimePickedUp)) {
            // force the switch if this is the first time we've picked it up
            SetNextWeapon2(Item, bFirstTimePickedUp);
        }
    }

    return ItemAdded;
}

//==============================================================================

bool player::RemoveItemFromInventory2(inven_item Item, bool bRemoveAll)
{
    if (Item == m_CurrentWeaponItem) {
        // Removing the current weapon -> Switch to having no weapon at all.
        // NOTE:  This logic assumes that when you remove the player's current
        // weapon, they shouldn't have any weapons.
        SwitchWeapon2(INVEN_NULL);
    }

    return actor::RemoveItemFromInventory2(Item, bRemoveAll);
}

//==============================================================================
void player::ItemAcquiredMessage(inven_item Item)
{
    float currentTime = (float)x_GetTimeSec();

    // make sure we don't flood the player with info.
    if ((currentTime - m_fLastItemAcquiredTime) > Item_Acquired_Msg_DelayTimeTweak.GetF32()) {
        // tell the player that they picked up something.
       // IJB MsgMgr.Message(MSG_ACQUIRED_ITEM, net_GetSlot(), Item);

        // reset time
        m_fLastItemAcquiredTime = currentTime;
    }
}

//==============================================================================

void player::ItemFullMessage(inven_item Item)
{
    float currentTime = (float)x_GetTimeSec();

    // make sure we don't flood the player with info.
    if ((currentTime - m_fLastItemFullTime) > Item_Full_Msg_DelayTimeTweak.GetF32()) {
        // tell the player that they can't carry anymore of these.
     // IJB   MsgMgr.Message(MSG_FULL_ITEM, net_GetSlot(), Item);

        // reset time
        m_fLastItemFullTime = currentTime;
    }
}

//==============================================================================

void player::NoWeapon_NoAmmoPickupMessage(inven_item Item)
{
    float currentTime = (float)x_GetTimeSec();

    // make sure we don't flood the player with info.
    if ((currentTime - m_fLastItemFullTime) > Item_Full_Msg_DelayTimeTweak.GetF32()) {
        // tell the player that they can't carry anymore of these.
       // IJB MsgMgr.Message(MSG_NO_ITEM_AMMO_FAIL, net_GetSlot(), Item);

        // reset time
        m_fLastItemFullTime = currentTime;
    }
}

//==============================================================================
bool player::CanTakePickup(pickup& Pickup)
{
    if (m_bDead) {
        //LOG_WARNING("player::CanTakePickup", "Dead player trying to take a pickup!");
        return false;
    }

    if (!Pickup.GetTakeable()) {
        return false;
    }

    inven_item Item = Pickup.GetItem();

    // **HEALTH**
    if (Item == INVEN_HEALTH) {
        return (GetHealth() < 100.0f);
    }

    // **MUTAGEN**
    if (Item == INVEN_MUTAGEN) {
        return (GetMutagen() < 100.0f);
    }

    // **WEAPONS**
    // The only time you CAN'T take a weapon pickup is when you already have
    // the weapon AND you already have the maximum ammo allowed.
    if (IN_RANGE(INVEN_WEAPON_FIRST, Item, INVEN_WEAPON_LAST)) {

        if (m_Inventory2.GetAmount(Item) < m_Inventory2.GetMaxAmount(Item)) {
            return (true);
        }

        new_weapon* pWeapon = GetWeaponPtr(Item);

        if (((Item == INVEN_WEAPON_SMP) && (m_Inventory2.GetAmount(Item) == 1.0f)) ||
            ((Item == INVEN_WEAPON_SHOTGUN) && (m_Inventory2.GetAmount(Item) == 1.0f)) ||
            ((Item == INVEN_WEAPON_DESERT_EAGLE) && (m_Inventory2.GetAmount(Item) == 1.0f))) {
            // get the dual weapon
            inven_item  DualItem = new_weapon::GetDualWeaponID(Item);
            new_weapon* pDualWeapon = GetWeaponPtr(DualItem);

            // make sure the dual weapon is valid and that we already have the dual in our inventory.
            if (pDualWeapon && m_Inventory2.GetAmount(DualItem) == 1.0f) {
                int Current = pDualWeapon->GetTotalPrimaryAmmo();
                int Limit = pDualWeapon->GetMaxPrimaryAmmo();

                // full of ammo, don't get it.
                if (Current >= Limit) {
                    // don't pick it up
                    return false;
                }
            }

            // dual weapon and we're missing some ammo or we haven't gotten a dual yet... get it.
            return true;
        }

        if (pWeapon) {
            int Current = pWeapon->GetTotalPrimaryAmmo();
            int Limit = pWeapon->GetMaxPrimaryAmmo();

            // never pickup a BBG for AMMO
            if (Item == INVEN_WEAPON_BBG) {
                return false;
            } else if ((Current < Limit)) {
                // pick it up
                return true;
            } else {
                // ammo full, don't pick it up and notify weapon for message
                pWeapon->NotifyAmmoFull(this);
                return false;
            }
        } else {
            assert(false); // We should have a weapon.
            return (false);
        }
    }

    // **AMMO**
    // The only time you CAN'T take an ammo pickup is when you already have the
    // maximum ammo allowed.
    if (IN_RANGE(INVEN_AMMO_FIRST, Item, INVEN_AMMO_LAST)) {
        // BBG ammo is just for debugging and such, NEVER pick it up
        if (Item == INVEN_AMMO_BBG) {
            assert(false); //ASSERTS(0, "You should not be placing BBG ammo, it is for debugging only");
            return false;
        }

        inven_item  Weapon = m_Inventory2.AmmoToWeapon(Item);
        new_weapon* pWeapon = GetWeaponPtr(Weapon);
        if (pWeapon) {
            if (m_Inventory2.HasItem(Weapon)) {
                int Current = pWeapon->GetTotalPrimaryAmmo();
                int Limit = pWeapon->GetMaxPrimaryAmmo();

                if (Current < Limit) {
                    // pick it up
                    return true;
                } else {
                    // ammo full, don't pick it up and notify weapon for message
                    pWeapon->NotifyAmmoFull(this);
                    return false;
                }
            } else {
                // weapon hasn't been gotten yet, don't pick up ammo.
                NoWeapon_NoAmmoPickupMessage(Item);

                return (false);
            }
        } else {
            assert(false); // We should have a weapon.
            return (false);
        }
    }

    // **EVERYTHING ELSE**
    // Otherwise, just check the limit within the inventory.
    if (m_Inventory2.CanHoldMore(Item)) {
        return true;
    } else {
        return false;
    }
}

//==============================================================================

void player::TakePickup(pickup& Pickup)
{
    ItemAcquiredMessage(Pickup.GetItem());

    if (m_bDead) {
        //LOG_WARNING("player::TakePickup", "A dead player got a pickup!");
        return;
    }

    inven_item Item = Pickup.GetItem();
    int        Amount = (int)Pickup.GetAmount();

    // **WEAPONS**
    // The only time you CAN'T take a weapon pickup is when you already have
    // the weapon AND you already have the maximum ammo allowed.
    if (IN_RANGE(INVEN_WEAPON_FIRST, Item, INVEN_WEAPON_LAST)) {
        if ((Item == INVEN_WEAPON_SMP) &&
            (m_Inventory2.GetAmount(Item) == 1.0f)) {
            Item = INVEN_WEAPON_DUAL_SMP;
        } else if ((Item == INVEN_WEAPON_SHOTGUN) &&
                   (m_Inventory2.GetAmount(Item) == 1.0f)) {
            Item = INVEN_WEAPON_DUAL_SHT;
        }
        /*** Uncomment this code to support dual desert eagles.
        if( (Item == INVEN_WEAPON_DESERT_EAGLE) &&
            (m_Inventory2.GetAmount( Item ) == 1.0f) )
        {
            Item = INVEN_WEAPON_DUAL_EAGLE;
        }
        ***/

        bool bFirstTimePickedup = false;

        // Take the weapon if there is space.
        if (m_Inventory2.GetAmount(Item) < 1.0f) {
            // don't have this item, it's the first time we've grabbed it.
            bFirstTimePickedup = true;

            m_Inventory2.SetAmount(Item, 1.0f);

            // set flag for dual weapon
            bool bIsDual = (Item == INVEN_WEAPON_DUAL_SMP) || (Item == INVEN_WEAPON_DUAL_SHT); //|| (Item == INVEN_WEAPON_DUAL_EAGLE );

            // we currently don't have one of these in our inventory
            if (ShouldSwitchToWeapon2(Item, true)) {
                // don't do state change if this is a dual weapon, we'll do it below
                SetNextWeapon2(Item, false, !bIsDual);

                // override animation for dual pickup
                if (bIsDual) {
                    SetAnimState(ANIM_STATE_PICKUP);
                }
            } else {
                // be sure we change the pre-mutation weapon if we pick up a dual
                if (IsMutated()) {
                    // set dual as premutation weapon
                    if (bIsDual) {
                        m_PreMutationWeapon2 = Item;
                    }
                }
            }
        }

        new_weapon* pWeapon = GetWeaponPtr(Item);

        // Take the clip of ammo in the weapon if possible.
        {
            // set up dual weapons specially.
            if (pWeapon) {
                inven_item ParentItem = new_weapon::GetParentIDForDualWeapon(Item);

                // make sure the clip is full!
                if (ParentItem != INVEN_NULL) {
                    // dual weapon's clip is full... give ammo to parent weapon
                    if ((pWeapon->GetAmmoCount() >= pWeapon->GetAmmoPerClip()) && !bFirstTimePickedup) {
                        new_weapon* pParentWeapon = GetWeaponPtr(ParentItem);

                        // add the ammo to the weapon
                        int GunAmmo = pParentWeapon->GetAmmoPerClip(new_weapon::AMMO_PRIMARY);

                        // add ammo to parent item's reserves
                        AddAmmo2(ParentItem, GunAmmo);

                        // make counts match up
                        pWeapon->SetupDualAmmo(ParentItem);
                    } else {
                        // we ran over a dual capable weapon, put the ammo in the dual clip.
                        pWeapon->SetupDualAmmo(ParentItem);
                    }
                } else {
                    // add the ammo to the weapon
                    int GunAmmo = pWeapon->GetAmmoPerClip(new_weapon::AMMO_PRIMARY);

                    // we ran over a dual capable weapon, put the ammo in the dual clip.
                    pWeapon->AddAmmoToWeapon(GunAmmo, 0);

                    // if you've died and picked up the weapon again, reload it
                    if (bFirstTimePickedup) {
                        int count = pWeapon->GetAmmoCount();
                        if (count <= 0) {
                            // reload without anim
                            pWeapon->RefillClip(new_weapon::AMMO_PRIMARY);

                            return;
                        }
                    }
                }

                // be sure we play reload anim if gun is completely empty
                int count = pWeapon->GetAmmoCount();
                if (count <= 0) {
                    // reload with anim
                    ReloadWeapon(new_weapon::AMMO_PRIMARY);
                }
            } else {
                assert(false); //ASSERTS(false, xfs("Weapon %s missing from blueprint bag", GetInventory2().ItemToName(Item)));
            }
        }

        return;
    }

    // **EVERYTHING ELSE**
    switch (Item) {
    case INVEN_HEALTH:
        m_Inventory2.SetAmount(Item, 0.0f);
        AddHealth((float)Amount);
        break;

    case INVEN_MUTAGEN:
    case INVEN_MUTAGEN_CORPSE:
        m_Inventory2.SetAmount(Item, 0.0f);
        AddMutagen((float)Amount);
        break;

    case INVEN_AMMO_SMP:
        AddAmmo2(INVEN_WEAPON_SMP, Amount);
        break;
    case INVEN_AMMO_SHOTGUN:
        AddAmmo2(INVEN_WEAPON_SHOTGUN, Amount);
        break;
    case INVEN_AMMO_DESERT_EAGLE:
        AddAmmo2(INVEN_WEAPON_DESERT_EAGLE, Amount);
        break;
    case INVEN_AMMO_SNIPER_RIFLE:
        AddAmmo2(INVEN_WEAPON_SNIPER_RIFLE, Amount);
        break;
    case INVEN_AMMO_MESON:
        AddAmmo2(INVEN_WEAPON_MESON_CANNON, Amount);
        break;

    case INVEN_AMMO_BBG:
        AddAmmo2(INVEN_WEAPON_BBG, Amount);
        break;

        // TODO - Handle ammo boxes (which have multiple ammo types within).

    default:
        // Should this be here?  Should we just assert( false )?
        m_Inventory2.AddAmount(Item, (float)Amount);
        break;
    }
}

//==============================================================================

bool player::OnPickup(pickup& Pickup)
{
    //  LOG_MESSAGE( "player::OnPickup", "" );

    // On the server:
    //  - First see if you want the pickup.
    //  - If so, take it.
    //
    // On a client:
    //  - First see if you want the pickup.
    //  - If so, tell the server "I want this pickup".
    //  - Server: If pickup not already taken, "You can have it".
    //  - Client: Take it.
    //  + This is further complicated by health pickups which must be handled
    //    by the ghost on the server side.

    if (!CanTakePickup(Pickup)) {
        return (false);
    }

    TakePickup(Pickup);
    return (true);
}

//==============================================================================
int GetStartWeaponsForLevel(const char* pLevelName)
{
    // KSS -- TO ADD NEW WEAPON
    // Determine the weapons a player start with and the start item
    int Weapons = WB_SMP | WB_DE | WB_SG | WB_SR | WB_MC | WB_MM | WB_MP | WB_FG | WB_BBG | WB_JBG | WB_SCN | WB_MS;
    for (int i = 0; s_MapToWeaponTable[i].pLevelName; i++) {
        if (x_stristr(pLevelName, s_MapToWeaponTable[i].pLevelName)) {
            Weapons = s_MapToWeaponTable[i].StartWeapons;
            break;
        }
    }

    return Weapons;
}

static int GetAvailableWeaponsForLevel(const char* pLevelName)
{
    // KSS -- TO ADD NEW WEAPON
    // Determine the weapons a player start with and the start item
    int Weapons = WB_SMP | WB_DE | WB_SG | WB_SR | WB_MC | WB_MM | WB_MP | WB_FG | WB_BBG | WB_JBG | WB_SCN;
    for (int i = 0; s_MapToWeaponTable[i].pLevelName; i++) {
        if (stristr(pLevelName, s_MapToWeaponTable[i].pLevelName)) {
            Weapons = s_MapToWeaponTable[i].AvailableWeapons;
            break;
        }
    }

    return Weapons;
}

//==============================================================================

inven_item GetEquipedWeaponForLevel(const char* pLevelName)
{
    // Determine the weapons a player start with and the start item
    inven_item StartItem = INVEN_WEAPON_SMP;
    for (int i = 0; s_MapToWeaponTable[i].pLevelName; i++) {
        if (x_stristr(pLevelName, s_MapToWeaponTable[i].pLevelName)) {
            StartItem = s_MapToWeaponTable[i].StartItem;
            break;
        }
    }

    return StartItem;
}

//==============================================================================

//==============================================================================
void player::DebugSetupInventory(const char* pLevelName)
{
    // KSS -- TO ADD NEW WEAPON
    int Weapons = GetStartWeaponsForLevel(pLevelName);

    // Give the player some weapons
    if (Weapons & WB_SMP) {
        m_Inventory2.SetAmount(INVEN_WEAPON_SMP, 1.0f);
    }
    if (Weapons & WB_SG) {
        m_Inventory2.SetAmount(INVEN_WEAPON_SHOTGUN, 1.0f);
    }
    if (Weapons & WB_DE) {
        m_Inventory2.SetAmount(INVEN_WEAPON_DESERT_EAGLE, 1.0f);
    }
    if (Weapons & WB_SR) {
        m_Inventory2.SetAmount(INVEN_WEAPON_SNIPER_RIFLE, 1.0f);
    }
    if (Weapons & WB_MC) {
        m_Inventory2.SetAmount(INVEN_WEAPON_MESON_CANNON, 1.0f);
    }
    if (Weapons & WB_BBG) {
        m_Inventory2.SetAmount(INVEN_WEAPON_BBG, 1.0f);
    }
    if ((Weapons & WB_MM) || (Weapons & WB_MP)) {
        m_Inventory2.SetAmount(INVEN_WEAPON_MUTATION, 1.0f);
    }
    if (Weapons & WB_FG) {
        m_Inventory2.SetAmount(INVEN_GRENADE_FRAG, 5.0f);
    }
    if (Weapons & WB_JBG) {
        m_Inventory2.SetAmount(INVEN_GRENADE_JBEAN, 5.0f);
    }
    if (Weapons & WB_SCN) {
        m_Inventory2.SetAmount(INVEN_WEAPON_SCANNER, 1.0f);
    }

    // mreed: You'll only have gloves until you mutate. The final solution is to make sure the player
    // has goves in the first level. I'm giving the player gloves all the time here so that we can
    // see the effect of having them, then losing them from mutation.
    if (!(Weapons & WB_MM) && !(Weapons & WB_MP)) {
        m_Inventory2.SetAmount(INVEN_GLOVES, 1.0f);
    }

    m_CurrentWeaponItem = GetEquipedWeaponForLevel(pLevelName);
}

void player::ReInitInventory()
{
    assert(m_WeaponsCreated);
    if (m_WeaponsCreated) {
        for (int i = 0; i < INVEN_NUM_WEAPONS; i++) {
            // Nuke the object, if it exists!
            new_weapon* pWeapon = (new_weapon*)objectManager->GetObjectByGuid(m_WeaponGuids[i]);
            if (pWeapon) {
                objectManager->DestroyObjectEx(m_WeaponGuids[i], true);
            }
            m_WeaponGuids[i] = 0;
        }
        m_WeaponsCreated = false;

        /* IJB
        FXMgr.EndOfFrame(); // Call FXMgr EndOfFrame to flush any deferred deletes - must be after ObjMgr.Clear
        FXMgr.EndOfFrame();
        FXMgr.EndOfFrame();
        FXMgr.EndOfFrame();
        */
    }

    // Now init the inventory.
    InitInventory();
}

//==============================================================================

void player::InitInventory()
{
    if (!m_WeaponsCreated) {
        CreateAllWeaponObjects();
        m_WeaponsCreated = true;
        /* IJB
        if (g_StateMgr.UseDefaultLoadOut()) {
            // Clear inventory
            m_Inventory2.Clear();
            DebugEnableWeapons(g_ActiveConfig.GetLevelPath());
            g_StateMgr.DisableDefaultLoadOut();
        }
            */
    }
}

//==============================================================================
//==============================================================================

view& player::GetView(int Player)
{
    return m_Views[Player];
}

//==============================================================================

view& player::GetView()
{
    return GetView(0);
}

//==============================================================================

bool player::IsAvatar()
{
    bool bRenderAvatar = !m_bActivePlayer || (m_CurrentAnimState == ANIM_STATE_FALLING_TO_DEATH) || (m_LocalSlot == -1);

    return bRenderAvatar;
}

//==============================================================================

bool player::UsingLoco()
{
    return (false);
}

//=========================================================================
void player::UpdateConvulsion()
{
}

//=========================================================================

float ComputeSoftLean(float LeanAmount)
{
    LeanAmount = std::max(-1.0f, LeanAmount);
    LeanAmount = std::min(1.0f, LeanAmount);

    const float Sign = (LeanAmount > 0.0f) ? 1.0f : (LeanAmount < 0.0f) ? -1.0f
                                                                        : 0;
    const float Period = abs(LeanAmount) * PI;
    const float Phase = PI / 2.0f;

    return (Sign * (1.0f - ((sin(Period + Phase) + 1.0f) / 2.0f)));
}

//=========================================================================

float ComputeLean(float SoftLeanAmount)
{
    SoftLeanAmount = std::max(-1.0f, SoftLeanAmount);
    SoftLeanAmount = std::min(1.0f, SoftLeanAmount);

    const float LeanAmount = abs((asin(((abs(SoftLeanAmount) - 1.0f) * -2.0f) - 1.0f) - (PI / 2.0f)) / PI);
    const float Sign = (SoftLeanAmount >= 0.0f) ? 1.0f : -1.0f;

    return Sign * LeanAmount;
}

//=========================================================================

float ComputeLeanValueForPosition(const Vector3& Start, const Vector3& HitPoint, bool bLeaningRight)
{
    Vector3 Lean(HitPoint - Start);
    Lean.y = 0.0f;

    float LeanAmount = (Lean / GetTweakF32("LeanX")).Length();
    if (bLeaningRight) {
        LeanAmount *= -1.0f;
    }

    return LeanAmount;
}

//=========================================================================

void player::UpdateLean(float LeanValue)
{
    const lean_state OldLeanState = m_LeanState;
    const float      OldLeanAmount = m_LeanAmount;
    const float      OldSoftLeanAmount = m_SoftLeanAmount;

    if ((LeanValue > GetTweakF32("LeanThreshold")) && (m_LeanAmount >= 0.0f)) {
        // Leaning left
        const float ElapsedTime = m_LeanAmount * GetTweakF32("LeanTime");
        const float NewElapsedTime = ElapsedTime + m_DeltaTime;
        m_LeanAmount = NewElapsedTime / GetTweakF32("LeanTime");
        m_LeanState = LEAN_LEFT;
    } else if ((LeanValue < -GetTweakF32("LeanThreshold")) && (m_LeanAmount <= 0.0f)) {
        // Leaning right
        const float ElapsedTime = (-m_LeanAmount) * GetTweakF32("LeanTime");
        const float NewElapsedTime = ElapsedTime + m_DeltaTime;
        m_LeanAmount = -(NewElapsedTime / GetTweakF32("LeanTime"));
        m_LeanState = LEAN_RIGHT;
    } else {
        // Returning to upright
        if (m_LeanAmount > 0.0f) {
            // from left
            const float ElapsedTime = m_LeanAmount * GetTweakF32("LeanTime");
            const float NewElapsedTime = ElapsedTime - m_DeltaTime;
            m_LeanAmount = NewElapsedTime / GetTweakF32("LeanTime");
            m_LeanAmount = std::max(0.0f, m_LeanAmount);
            m_LeanState = LEAN_RETURN_FROM_LEFT;
        } else if (m_LeanAmount < 0.0f) {
            // from right
            const float ElapsedTime = (-m_LeanAmount) * GetTweakF32("LeanTime");
            const float NewElapsedTime = ElapsedTime - m_DeltaTime;
            m_LeanAmount = -(NewElapsedTime / GetTweakF32("LeanTime"));
            m_LeanAmount = std::min(0.0f, m_LeanAmount);
            m_LeanState = LEAN_RETURN_FROM_RIGHT;
        } else {
            m_LeanState = LEAN_NONE;
        }
    }
    m_LeanAmount = std::max(-1.0f, m_LeanAmount);
    m_LeanAmount = std::min(1.0f, m_LeanAmount);

    m_SoftLeanAmount = ComputeSoftLean(m_LeanAmount);

    // check to make sure we aren't violating any collision
    if ((m_LeanState == LEAN_RIGHT) || (m_LeanState == LEAN_LEFT)) {
        // New position
        Vector3 StartPos(GetPosition());
        Vector3 Offset(GetAnimPlayerOffset());
        Offset.y = 0.0f;
        Vector3 EndPos(GetPosition() + Offset);

        //
        // We need to elevate our collision check to account for uneven floors
        // that cause us to collide at our feet, when we're mostly just
        // concerned about the player's head and weapon
        //
        static float TweakLeanElevate = 10.0f;
        StartPos.y += TweakLeanElevate;
        EndPos.y += TweakLeanElevate;

        const Vector3 Delta(EndPos - StartPos);

        if (Delta.LengthSquared() > ZERO) {
            m_Physics.SetupPlayerCollisionCheck(StartPos, EndPos);
            g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES,
                                           Object::ATTR_BLOCKS_PLAYER_LOS,
                                           Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING);

            if (g_CollisionMgr.m_nCollisions > 0) {
                Vector3 StopPos = StartPos + (Delta * g_CollisionMgr.m_Collisions[0].T);
                m_SoftLeanAmount = ComputeLeanValueForPosition(StartPos, StopPos, m_LeanState == LEAN_RIGHT);
                m_LeanAmount = ComputeLean(m_SoftLeanAmount);
            }
        }
    }
}

//=========================================================================

Vector3 player::GetLeanOffset()
{
    Vector3 Forward(Radian3(0.0f, m_Yaw, 0.0f));
    Vector3 Lean(0.0f, 0.0f, 0.0f);

    if (m_SoftLeanAmount != 0.0f) {
        Lean = Forward;
        Lean.RotateY(R_90);
        Lean *= (m_SoftLeanAmount * GetTweakF32("LeanX")); // Scale according to our horiz offset

        float Vertical = (PI / 2) * abs(m_SoftLeanAmount) * GetTweakF32("LeanY");

        Lean += Vector3(0.0f, -Vertical, 0.0f);
    }

    return Lean;
}

//=========================================================================

void player::UpdateArmsOffsetForLean()
{
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        //const bbox BBox( pWeapon->GetBBox() );
        const Vector3 CenterPos(pWeapon->GetBBox().GetCenter());
        Radian        Pitch, Yaw;
        GetView().GetPitchYaw(Pitch, Yaw);
        const Vector3 LookDir(Radian3(Pitch, Yaw, 0.0f));
        const Vector3 ViewPos(GetView().GetPosition() - (LookDir * 10000.0f));
        const Vector3 ViewEnd(ViewPos + (LookDir * 10000.0f));
        Vector3       Closest(CenterPos.GetClosestPToLSeg(ViewPos, ViewEnd));
        const float   DistanceThisFrame = GetTweakF32("LeanWeaponOffsetSpeed") * m_DeltaTime;

        switch (m_LeanState) {
        case LEAN_LEFT:
        case LEAN_RIGHT:
        {
            // Move the rig under our look direction
            Vector3     Dir = Closest - CenterPos;
            const float TotalDistanceSquared = Dir.LengthSquared();
            if (x_sqr(DistanceThisFrame) >= TotalDistanceSquared) {
                m_LeanWeaponOffset += Dir; // Just go the remaining distance
            } else {
                Dir.Normalize();
                Dir *= (m_LeanWeaponOffset.Length() + DistanceThisFrame);
                m_LeanWeaponOffset = Dir;
            }
        } break;

        case LEAN_NONE:
        case LEAN_RETURN_FROM_LEFT:
        case LEAN_RETURN_FROM_RIGHT:
        {
            Vector3 Dir = -m_LeanWeaponOffset;
            // Move the rig back
            if (x_sqr(DistanceThisFrame) > Dir.LengthSquared()) {
                m_LeanWeaponOffset.Zero();
            } else {
                Dir.Normalize();
                Dir *= DistanceThisFrame;
                m_LeanWeaponOffset += Dir;
            }
        } break;
        default:
            assert(false); //, xfs("Invalid lean_state: %d", m_LeanState));
        }
    } else {
        m_LeanWeaponOffset.Zero();
    }

    assert(m_LeanWeaponOffset.IsValid());
}

//=========================================================================

Vector3 player::GetAnimPlayerOffset()
{
    return m_ArmsOffset + GetLeanOffset() + m_LeanWeaponOffset;
}

//=========================================================================

// we can't "hide" objects, so we'll just push it down really far out of sight
void HideObject(Object* pObj)
{
    if (pObj) {
        Vector3 Pos(pObj->GetPosition());
        Pos.y -= 10000.0f;
        pObj->OnMove(Pos);
    }
}

void UnhideObject(Object* pObj)
{
    if (pObj) {
        Vector3 Pos(pObj->GetPosition());
        Pos.y += 10000.0f;
        pObj->OnMove(Pos);
    }
}

//=========================================================================

void player::ManTurret(guid TurretGuid,
                       guid Turret2Guid,
                       guid Turret3Guid,
                       guid AnchorGuid,
                       guid LeftBoundaryGuid,
                       guid RightBoundaryGuid,
                       guid UpperBoundaryGuid,
                       guid LowerBoundaryGuid)
{
    if (m_bIsMutated) {
        SetupMutationChange(false);
        m_bIsMutantVisionOn = false;
        ForceNextWeapon();
    }

    m_Turret.PreviousWeapon = m_CurrentWeaponItem;
    SetNextWeapon2(INVEN_WEAPON_TRA);

    m_Turret.TurretGuid = TurretGuid;
    m_Turret.Turret2Guid = Turret2Guid;
    m_Turret.Turret3Guid = Turret3Guid;
    m_Turret.PreviousL2W = GetL2W();
    m_Turret.PreviousWeapon = m_CurrentWeaponItem;
    m_Turret.LeftBoundaryGuid = LeftBoundaryGuid;
    m_Turret.RightBoundaryGuid = RightBoundaryGuid;
    m_Turret.UpperBoundaryGuid = UpperBoundaryGuid;
    m_Turret.LowerBoundaryGuid = LowerBoundaryGuid;

    // Hide the turret and parts
    /* IJB
    turret* pTurret = (turret*)objectManager->GetObjectByGuid(TurretGuid);
    assert(pTurret->IsKindOf(turret::GetRTTI()));
    if (pTurret) {
        pTurret->Hide();
    }
        */
    HideObject(objectManager->GetObjectByGuid(Turret2Guid));
    HideObject(objectManager->GetObjectByGuid(Turret3Guid));

    Object* pAnchor = objectManager->GetObjectByGuid(AnchorGuid);
    if (pAnchor) {
        m_Turret.AnchorL2W = pAnchor->GetL2W();
        const Radian3 Rotation(m_Turret.AnchorL2W.GetRotation());
        Teleport(m_Turret.AnchorL2W.GetTranslation(), Rotation.pitch, Rotation.yaw);
    } else {
        m_Turret.AnchorL2W.Identity();
    }

    SetIsCrouching(false);
    m_bInTurret = true;
}

//=========================================================================

void player::ExitTurret()
{
    if (m_bInTurret) {
        const Radian3 Rotation(m_Turret.PreviousL2W.GetRotation());
        Teleport(m_Turret.PreviousL2W.GetTranslation(), Rotation.pitch, Rotation.yaw);
        loco* pLoco = GetLocoPointer();
        if (pLoco) {
            pLoco->m_Physics.SetVelocity(Vector3(0.0f, 0.0f, 0.0f));
        }
        SwitchWeapon2(m_Turret.PreviousWeapon);
        m_Inventory2.RemoveAmount(INVEN_WEAPON_TRA, 1);
        m_bInTurret = false;

        // unhide the turret and parts
        /* IJB
        turret* pTurret = (turret*)objectManager->GetObjectByGuid(m_Turret.TurretGuid);
        assert(pTurret->IsKindOf(turret::GetRTTI()));
        if (pTurret) {
            pTurret->Unhide();
        }
        */
        UnhideObject(objectManager->GetObjectByGuid(m_Turret.Turret2Guid));
        UnhideObject(objectManager->GetObjectByGuid(m_Turret.Turret3Guid));
    }
}

//=========================================================================

void player::Teleport(const Vector3& Position, bool DoBlend, bool DoEffect)
{
    if (!DoBlend) {
        /* IJB
        extern void ActivateSoundEmitters(const Vector3& Position);
        ActivateSoundEmitters(Position);
        */
    }

    m_Physics.InitialGroundCheck(Position);
    actor::Teleport(Position, DoBlend, DoEffect);

    if (!DoBlend) {
        // Update ears
        Radian  Pitch = GetPitch();
        Radian  Yaw = GetYaw();
        Matrix4 W2V;
        W2V.Identity();
        W2V.RotateX(Pitch);
        W2V.RotateY(Yaw);
        W2V.Translate(Position + Vector3(0, 180, 0));
        /* IJB
        W2V.InvertRT();
        g_AudioMgr.SetEar(m_AudioEarID, W2V, Position, GetZone1(), 1.0f);
        */
    }

    // Make sure 1st person weapon is in sync
    OnMoveWeapon();
}

//=========================================================================

void player::Teleport(const Vector3& Position, Radian Pitch, Radian Yaw, bool DoBlend, bool DoEffect)
{
    // Update velocity direction
    Vector3 Velocity = m_Physics.GetVelocity();
    Radian  DeltaYaw = x_MinAngleDiff(Yaw, GetYaw());
    Velocity.RotateY(DeltaYaw);
    m_Physics.SetVelocity(Velocity);

    if (!DoBlend) {
        /* IJB
        extern void ActivateSoundEmitters(const Vector3& Position);
        ActivateSoundEmitters(Position);
        */
    }

    // When the player (not actor or ghost) teleports and gets a new pitch/yaw,
    // we need to clear his network targeting information just to be safe.
    m_TargetNetSlot = -1;

    m_Physics.InitialGroundCheck(Position);
    actor::Teleport(Position, Pitch, Yaw, DoBlend, DoEffect);

    if (!DoBlend) {
        // Update ears
        Matrix4 W2V;
        W2V.Identity();
        W2V.RotateX(Pitch);
        W2V.RotateY(Yaw);
        W2V.Translate(Position + Vector3(0, 180, 0));
        // IJB W2V.InvertRT();
        // IJB g_AudioMgr.SetEar(m_AudioEarID, W2V, Position, GetZone1(), 1.0f);

        // Make sure 1st person hands are in sync
        m_AnimPlayer.SetYaw(m_Yaw + m_CurrentHorozRigOffset - m_ShakeYaw);
        m_AnimPlayer.SetPitch(-m_Pitch - m_CurrentVertRigOffset - m_ShakePitch);
    }

    // Make sure 1st person weapon is in sync
    AttachWeapon();
}

void player::ForceNextWeapon()
{
    // Set previous weapon and current weapon, clear next weapon
    m_PrevWeaponItem = m_CurrentWeaponItem;
    m_CurrentWeaponItem = m_NextWeaponItem;
    m_NextWeaponItem = INVEN_NULL;

    // zero out the reticle radius
    m_ReticleRadius = 0.0f;
    m_ReticleGrowSpeed = 0.0f;
    m_AimAssistData.bReticleOn = false;

    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        pWeapon->SetupRenderInformation();
        pWeapon->SetRotation(m_AnimPlayer.GetPitch(), m_AnimPlayer.GetYaw());
        OnMoveWeapon();

        // Bring the new weapon up.
        SetAnimState(ANIM_STATE_SWITCH_TO);
    } else {
        SetAnimState(ANIM_STATE_UNDEFINED);
    }
}

void player::ForceMutationChange(bool bMutate)
{
    SetupMutationChange(bMutate);

    if (m_bIsMutated && (m_CurrentWeaponItem != INVEN_WEAPON_MUTATION)) {
        ForceNextWeapon();
    }
    m_bIsMutantVisionOn = m_bIsMutated;
}

Vector3 player::GetBonePos(int BoneIndex)
{
    // for NPCs or 3rd person view of avatar
    if (IsAvatar()) {
        return actor::GetBonePos(BoneIndex);
    }

    // First-person player arms
    return m_AnimPlayer.GetBonePosition(BoneIndex);
}

void player::ContagionDOT()
{
}
