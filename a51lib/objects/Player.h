#pragma once

#include "Actor.h"
#include "../RTTI.h"
#include "../objectManager/ObjectManager.h"
#include "../view/View.h"
#include "../zoneManager/ZoneManager.h"
#include "../Bitmap.h"
#include "../loco/CharacterPhysics.h"
#include "PlayerLoco.h"

#include "../trigger/actions/lock_player_view.h"

#include "../animation/AnimPlayer.h"

// #include "Objects\Render\SkinInst.hpp"
#include "../animation/CharAnimPlayer.h"
// #include "AudioMgr\AudioMgr.hpp"
// #include "ZoneMgr\ZoneMgr.hpp"
#include "../EventMgr.h"

// #include "Objects\SpawnPoint.hpp"

// #include "Objects\NewWeapon.hpp"
// #include "Objects\ProjectileBullett.hpp"
// #include "Objects\Pickup.hpp"

// #include "..\Support\NetworkMgr\NetObj.hpp"
// #include "..\Support\NetworkMgr\Blender.hpp"

#include "../inputMgr/GamePad.h"

#define STUN_PLAYER

#define MAX_MELEE_STATES 5 // ANIM_STATE_MELEE to ANIM_STATE_MELEE_END.  Keep this in sync.
#define MAX_LORE_ITEMS 5

//==============================================================================
// KSS -- TO ADD NEW WEAPON
#define WB_SMP (1 << 0)  // SMP
#define WB_DE (1 << 1)   // Desert Eagle
#define WB_SG (1 << 2)   // Shotgun
#define WB_SR (1 << 3)   // Sniper Rifle
#define WB_MC (1 << 4)   // Meson Cannon
#define WB_MM (1 << 5)   // Mutant Melee
#define WB_MP (1 << 6)   // Mutant Parasites
#define WB_FG (1 << 7)   // Frag Grenades
#define WB_BBG (1 << 8)  // BBG (NAW)
#define WB_JBG (1 << 9)  // Jumping Bean Grenades
#define WB_SCN (1 << 10) // Jumping Bean Grenades
#define WB_MS (1 << 11)  // Mutant secondary (contagion)

#define MAX_COMBO_HITS 3

// Forward Declarations
class weapon_mutation;
class collectable_anti_mutagen;
class ladder_field;
class check_point_mgr;
class collision_mgr;

//==============================================================================
//  TYPES
//==============================================================================

class corpse;
class third_person_camera;

class HudObject;

struct mtwt
{
    const char* pLevelName;
    int         MemoryBallast;
    inven_item  StartItem;
    int         AvailableWeapons;
    int         StartWeapons;
    bool        bLoadWarnsLowAmmo;
};

//------------------------------------------------------------------------------
// aim assist info
//------------------------------------------------------------------------------
struct AimAssistData
{
    AimAssistData(); // struct constructor

    Vector3 BulletAssistDir;      // our aim assist direction for the bullet
    bool    bReticleOn;           // is the reticle on a target
    float   BulletAssistBestDist; // the best distance to the target used for bullet assist
    float   TurnDampeningT;       // what is the interpolation distance for turn dampening
    guid    TargetGuid;           // this is the best target for our aim assist
    float   LOFCollisionDist;     // used to test if we hit something before we got to target
    float   LOFSpineDist;         // distance to the closest point on the spine that we are firing at
    Vector3 SpinePt;              // point on the spine
    Vector3 LOFPt;                // where the line of fire point ends
    float   LOFPtT;               // at what point along the line of fire is the spine point intersecting
    float   SpinePtT;             // where along the spine is the line of fire intersecting
    float   LOFPtDist;            // the actual distance along the line of fire point (AIMASSIST_LOF_DIST * LOFPtT)
    float   ReticleRadius;        // used for the pill that tells when the reticle is over an enemy (turns it on).
    float   BulletInnerRadius;    // what's the inner radius of the bullet assist
    float   BulletOuterRadius;    // what's the outer radius of the bullet assist
    float   TurnInnerRadius;      // what's the inner radius of the turn dampening
    float   TurnOuterRadius;      // what's the outer radius of the turn dampening

    // online stuff
    guid    OnlineFriendlyTargetGuid; // this is the friendly target for online
    Vector3 AimDelta;                 // used for online to determine the offset of where we are aiming
};

class player : public actor
{
public:
    CREATE_RTTI(player, actor, Object)

    enum cycle_direction
    {
        CYCLE_LEFT = 0,
        CYCLE_RIGHT
    };

    struct bullet_fly_by
    {
        Vector3 Start;
        Vector3 End;
        float   Age;
        float   Lifetime;
        int     VoiceID;
        bool    bIsActive;
    };

    struct convulsion_info
    {
        convulsion_info();
        float m_TimeSinceLastConvulsion;  // How long ago did we have a convulsion?
        float m_ConvulseAtTime;           // At what m_TimeSinceLastConvulsion do we convulse again?
        bool  m_bConvulsingNow;           // Are we convulsing now?
        float m_TimeLeftInThisConvulsion; // If we are convulsing now, how much longer?
    };

#define MAX_FLY_BYS (4)

    //------------------------------------------------------------------------------
    struct strain_control_modifiers
    {
        strain_control_modifiers();

        float   m_StrainProximityAlertRadius;
        float   m_StrainMaxFowardVelocity;    // Maximum foward/Backwards velocity
        float   m_StrainMaxStrafeVelocity;    // Maximum strafe velocity
        float   m_StrainJumpVelocity;         // How much velocity he has at the take off of the jump
        float   m_StrainMaxHealth;            // Player's maximum health
        Vector3 m_StrainEyesOffset;           // Offset for the strain
                                              //      float                     m_StrainStickSensitivity;             // Stick sensitivity
        float m_StrainMinWalkSpeed;           // Strain Min Walk Speed
        float m_StrainMinRunSpeed;            // Strain Min Run Speed
        float m_StrainDecelerationFactor;     // Rate of deceleration for this strain.
        float m_StrainCrouchChangeRate;       // Speed of crouch.
        float m_StrainReticleMovementDegrade; // How quickly does the reticle degrade
        float m_fStrainForwardAccel;          // Forward acceleration
        float m_fStrainStrafeAccel;           // Strafe acceleration
        float m_fStrainYawSensitivity;        // Yaw sensitivity for this strain
        float m_fStrainPitchSensitivity;      // Pitch sensitivity for this strain
        float m_StrainYawAccelTime;           // How long it takes to get to max acceleration for yaw rotation
        float m_StrainPitchAccelTime;         // How long it takes to get to max acceleration for pitch rotation
    };

    enum view_flags
    {
        VIEW_NULL = 0,
        VIEW_SHAKE = 1,
        VIEW_ALL = 0xFFFFFFFF
    };

    enum non_exclusive_states
    {
        NE_STATE_NULL = 0,
        NE_STATE_STUNNED = 1
    };

    struct view_info
    {
        view_info()
        {
            XFOV = 0;
        }
        Radian XFOV;
    };

    //------------------------------------------------------------------------------
    enum weapon_state
    {
        WEAPON_STATE_NONE = 0,
        WEAPON_STATE_FIRST_PERSON,
        WEAPON_STATE_AVATAR
    };

    //------------------------------------------------------------------------------
    enum anim_priority
    {
        ANIM_PRIORITY_DEFAULT = 0,
        MAX_ANIM_PER_STATE = 4
    };

    //------------------------------------------------------------------------------
    struct state_anims
    {
        uint8_t nPlayerAnims;
        uint8_t nWeaponAnims;
        uint8_t PlayerAnim[MAX_ANIM_PER_STATE];
        uint8_t WeaponAnim[MAX_ANIM_PER_STATE];
    };

    //------------------------------------------------------------------------------
    // List of the different animation states for player stage1 and 2.
    enum animation_state
    {
        ANIM_STATE_UNDEFINED = -1,

        ANIM_STATE_SWITCH_TO,
        ANIM_STATE_SWITCH_FROM,
        ANIM_STATE_IDLE,
        ANIM_STATE_RUN,
        ANIM_STATE_THROW,
        ANIM_STATE_PICKUP,
        ANIM_STATE_DISCARD,

        ANIM_STATE_FIRE,
        ANIM_STATE_ALT_FIRE,
        ANIM_STATE_GRENADE,
        ANIM_STATE_ALT_GRENADE,
        ANIM_STATE_MUTATION_SPEAR,
        ANIM_STATE_MELEE,
        ANIM_STATE_MELEE_FROM_CENTER,
        ANIM_STATE_MELEE_FROM_DOWN,
        ANIM_STATE_MELEE_FROM_LEFT,
        ANIM_STATE_MELEE_FROM_RIGHT,
        ANIM_STATE_MELEE_FROM_UP,
        ANIM_STATE_MELEE_TRAVEL,
        ANIM_STATE_MELEE_END,

        // combo melee stuff
        ANIM_STATE_COMBO_BEGIN,
        ANIM_STATE_COMBO_HIT,
        ANIM_STATE_COMBO_END,

        ANIM_STATE_RELOAD,
        ANIM_STATE_RELOAD_IN,
        ANIM_STATE_RELOAD_OUT,
        ANIM_STATE_RAMP_UP,
        ANIM_STATE_RAMP_DOWN,
        ANIM_STATE_ALT_RAMP_UP,
        ANIM_STATE_ALT_RAMP_DOWN,
        ANIM_STATE_HOLD,
        ANIM_STATE_ALT_HOLD,

        ANIM_STATE_ZOOM_IN,
        ANIM_STATE_ZOOM_OUT,
        ANIM_STATE_ZOOM_IDLE,
        ANIM_STATE_ZOOM_RUN,
        ANIM_STATE_ZOOM_FIRE,

        ANIM_STATE_DEATH,
        ANIM_STATE_CHANGE_MUTATION,
        ANIM_STATE_FALLING_TO_DEATH,

        ANIM_STATE_CINEMA,
        ANIM_STATE_MISSION_FAILED,

        ANIM_STATE_MAX
    };

    //=========================================================================
    //
    // GetActivePlayer  -   This is to get the active player.
    // GetView          -   Gets the player view used for rendering.
    // GetIsActiveView  -   Tells whether or not this player has the active view.
    // ResetView        -   Resets the view to the original FOV
    // GetEyesPosition  -   Gets the position of the eyes of the player. (The view position)
    // GetPitch         -   Returns the pitch of the player
    // GetYaw           -   Returns the Yaw of the player
    // GetSpeed         -   Returns instantaneous velocity of the player
    // SetMaxPitch      -   Sets the maximum pitch of the player
    // SetMinPitch      -   Sets the minimum pitch of the player
    // SetStickSensitivity- How sensitive is the stick?
    // ResetStickSensitivity- Resets to original stick sensitivity.

    // AddHealth        -   Nothing yet.  Will update player's health when a health Object is collected
    // AddDefence       -   Nothing yet.  Will update player's defensive stats when defensive item is collected
    // AddBoost         -   Nothing yet.  Will update player's stats when boost item is collected

    // GetMaxHealth     -   Returns maximum health for the player.
    // OnDeath          -   Called when player dies.  Resets position, resets HUD.
    // OnReset          -   Resets the player's zone, hud, and health and moves the player to respawn position
    // GetPlayerObjectZone  -   Returns zone from m_PlayerTracker (ie. what the player feet are in)
    // GetPlayerViewZone    -   Returns zone from cinema (if playing) or m_PlayerTracker if not
    // ShakeView        -   Shakes the view of this player Object.

    // OnPain           -   Called when something tries to hurt the player
    //
    //
    // SpottedByEnemy   -   Called by enemy when it spots the player.  Used to update time for figuring out when it's safe to save
    //
    //=========================================================================

public:
    player(ObjectManager* pObjectManager, collision_mgr* pCollisionMgr);
    virtual ~player();

    // Object description.
    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    static player* GetActivePlayer();
    void           SetAsActivePlayer(bool bActive) { m_bActivePlayer = bActive; }
    bool           IsActivePlayer() { return m_bActivePlayer; }

    void SetLocalPlayer(int LocalIndex);

    static view&     GetView(int Player);
    view&            GetView();
    const view_info& GetViewInfo() { return m_ViewInfo; }
    bool             IsAvatar();
    bool             IsMutated() { return m_bIsMutated; }
    bool             IsMutantVisionOn() { return m_bIsMutantVisionOn; }
    bool             SetMutated(bool bMutate) override;
    void             SetupMutationChange(bool bMutate) override;
    void             UpdateMutagen(float DeltaTime);
    float            GetSpeed();
    const view_info& GetOriginalViewInfo() { return m_OriginalViewInfo; }
    void             ResetView();
    void             SetStickSensitivity(const float& rMultiplier); //{ m_fStickSensitivity *= rMultiplier; }
    void             ResetStickSensitivity();                       //{ m_fStickSensitivity = m_fOriginalStickSensitivity; }
    void             OnKill() override;
    void             OnDeath() override;
    virtual void     OnReset();
    void             OnSpawn() override;
    void             OnMissionFailed(int TableName, int ReasonName);

    HudObject* GetHud();

    void ManTurret(guid TurretGuid,
                   guid Turret2Guid,
                   guid Turret3Guid,
                   guid AnchorGuid,
                   guid LeftBoundaryGuid,
                   guid RightBoundaryGuid,
                   guid UpperBoundaryGuid,
                   guid LowerBoundaryGuid);
    void ExitTurret();
    void Teleport(const Vector3& Position, Radian Pitch, Radian Yaw, bool DoBlend = true, bool DoEffect = false) override;
    void Teleport(const Vector3& Position, bool DoBlend = true, bool DoEffect = false) override;

    void              ParseOnPainForEffects(const pain& Pain);
    void              UpdateZoneTrack() override;
    zone_mgr::zone_id GetPlayerObjectZone() const;
    zone_mgr::zone_id GetPlayerViewZone() const;
    void              ShakeView(float Time, float Amount = 1.0f, float Speed = 1.0f);
    bool              InvalidSound();
    float             GetAimDegradation() { return m_AimDegradation; };
    Vector3           GetDefaultViewPos();

    void ComputeView(view& View, view_flags Flags = player::VIEW_NULL);
    void Push(const Vector3& PushVector) override;

    const std::vector<pain>& GetLastPainEvents() { return m_LastPainEvent; }
    void                     ClearPainEvent();
    void                     OnPain(const pain& Pain) override;
    void                     RespondToPain(const pain& Pain);
    void                     DoBasicPainFeedback(float Force);
    actor::eHitType          OverrideFlinchType(actor::eHitType hitType) override;
    virtual bool             AddHealth(float nDeltaHealth);
    virtual bool             AddAmmo2(inven_item WeaponItem, int Amount);
    void                     EmitMeleePain();
    void                     DoTendrilCollision();

    //void HandleBulletFlyby(bullet_projectile& Bullet);
    bool RenderSniperZoom();

    //virtual render_inst*       GetRenderInstPtr() { return &m_Skin; }
    AnimGroup::handle* GetAnimGroupHandlePtr() override { return &m_AnimGroup; }
    Vector3            GetBonePos(int BoneIndex) override;

    const char* GetLogicalName() override { return "PLAYER"; }

    //=========================================================================
    // SpottedByEnemy       -   Resets safe spot counter
    // GetIsSafeSpot        -   Checks some tickers and tells if the place where the player is is now safe.
    // SetCurrentSpotAsSafeSpot - Sets player's current position as the new safe spot.
    // ResetToLastSafeSpot  -     Resets the player to the last safe spot
    // GatherGameSpeakGuid  - Gathers possible candidate to speak to
    // DegradeAim           - Slight degradation of the bullet's trajectory.
    // SetAimRecoverSpeed   - How quickly does it take for the aim to recover after the player stops moving
    // PushViewCinematic    - Sets view to cinematic
    // IsStealthed          - Is this Object stealthed
    // OnPain               - Pain event handler
    // BackUpCurrentState   - Saves current state of the player
    // ResetPlayer          - SOMETHING TO DO WITH RIFT BOT.  DON'T KNOW.
    // CanSeeObject         - Can we see the specified Object.
    // GetMaxVelocity       - Maximum velocity
    // GetCurrentVelocity   - Current velocity.
    // GetSightYaw          - direction head is facing
    //=========================================================================
public:
    virtual void GetProjectileHitLocation(Vector3& EndPos, bool bUseBulletAssist = true);
    bool         IsAltFiring();
    bool         IsFiring();
    //virtual void SpottedByEnemy() { m_LastTimeSeenByEnemy = (float)x_GetTimeSec(); }
    virtual bool GetIsSafeSpot();
    virtual void SetCurrentSpotAsSafeSpot();
    virtual void ResetToLastSafeSpot();

    virtual void GatherGameSpeakGuid();

    void         AddNewWeapon2(inven_item WeaponItem) override;
    virtual bool TryAddAmmo2(inven_item WeaponItem);
    void         UpdateVirtualWeapons2();

    //=========================================================================
    // FLASHLIGHT STUFF
    //=========================================================================
    void          InitFlashlight(const Vector3& rInitPos);
    bool          IsFlashlightOn() override { return m_bUsingFlashlight; }
    bool          IsFlashlightActive();
    void          SetFlashlightActive(bool bOn);
    void          MoveFlashlight();
    void          UpdateFlashlightBattery(float nDeltaTime);
    virtual bool  AddBattery(const float& nDeltaBattery);
    virtual float GetBattery() { return m_Battery; }
    virtual float GetMaxBattery() { return m_MaxBattery; }

    virtual void   DegradeAim(float fAmountToDegradeBy);
    virtual void   SetAimRecoverSpeed(float fRecover) { m_AimRecoverSpeed = fRecover; }
    const Vector3& GetCurrentWeaponCollisionOffset() { return m_WeaponCollisionOffset; }

    bool IsPlayer() override { return true; }

    //void PushViewCinematic(lock_view_node* pLockViewBuffer);
    guid GetCinemaCameraGuid() const { return m_Cinema.m_CinemaCameraGuid; }

    virtual void BackUpCurrentState();
    virtual void RestoreState();

    void         ResetPlayer(const Vector3& rPos, const Radian3& rLookAt);
    virtual bool CanSeeObject(Object* pObject);

    float        GetMaxVelocity();
    float        GetCurrentVelocity();
    float        GetCollisionHeight() override;
    float        GetCollisionRadius() override;
    virtual void AddImpulse(Vector3& rImpulse);
    void         OnWeaponSwitch2(const cycle_direction& CycleDirection);
    void         SetNextWeapon2(inven_item WeaponItem, bool ForceSwitch = false, bool StateChange = true);
    bool         ShouldSwitchToWeapon2(inven_item WeaponItem, bool bFirstPickup);

    void         OnMove(const Vector3& NewPos) override;
    virtual void OnMoveFreeCam(view& View);
    virtual void OnExitFreeCam(Vector3& NewPos);
    int          GetActivePlayerPad() { return m_ActivePlayerPad; }
    int          GetLocalSlot() const { return m_LocalSlot; }
    void         SetLocalSlot(int Slot) { m_LocalSlot = Slot; }
    virtual void Jump();
    void         HitJumpPad(const Vector3& Velocity,
                            float          DeltaTime,
                            float          AirControl,
                            bool           BoostOnly,
                            bool           ReboostOnly,
                            bool           Instantaneous,
                            guid           JumpPadGuid);
    void         DoFeedback(float Duration, float Intensity);
    void         UpdateLean(float LeanValue);
    void         UpdateArmsOffsetForLean();
    float        GetLastTimeWeaponFired();

    //---------------------------------------------------------------------
    // Mutation weapon melee stuff
    //---------------------------------------------------------------------
    animation_state  SetupMutationMeleeWeapon();
    weapon_mutation* GetMutationMeleeWeapon();
    void             RandomizeMeleeAnimStateList();
    void             InitializeMeleeAnimStateList();
    animation_state  GetNextMeleeState();

    virtual void SetMeleeState(animation_state MeleeState);

    void   OnEvent(const event& Event) override;
    float  GetMovementNoiseLevel() override;
    Radian GetSightYaw() const override;

    bool AddItemToInventory2(inven_item Item) override;
    bool RemoveItemFromInventory2(inven_item Item, bool bRemoveAll = false) override;

    void AcquireAllLoreObjects();
    void LoadAllLoreObjects();
    bool GetJBGLoreAcquired() { return m_bJBGLoreAcquired; }
    void SetJBGLoreAcquired(bool bAcquired) { m_bJBGLoreAcquired = bAcquired; }
    bool GetClosestLoreObjectDist(float& ClosestDist); // returns FALSE if no objects are available or none are close enough

    guid GetEnemyOnReticle();
    guid GetFriendlyOnReticle();

    guid GetBestTargetGuid()
    {
        return m_AimAssistData.TargetGuid;
    }

    Vector3 GetAimDelta()
    {
        return m_AimAssistData.AimDelta;
    }

    float GetReticleRadius() const { return m_ReticleRadius; }
    bool  ReticleOnTarget() const { return m_AimAssistData.bReticleOn; }
    void  UpdateWeaponPullback();
    void  MoveAnimPlayer(const Vector3& Pos); // this adds in the arms offset and keeps the weapon
                                              // out of geometry
    Vector3 GetAnimPlayerOffset();

    void VoteCast(int Vote);
    void VoteStartKick(int Kick);
    void VoteStartMap(int Map);
    bool CanVote() { return m_VoteMode; }

    void OnTransform(const Matrix4& L2W) override;

    //=========================================================================
    //
    // GetLocalBBox         -   Gets the bbox of the player in local space
    // GetMaterial          -   Legacy code. (Will get removed with time)
    // OnMove               -   Any time of the player needs to change its position
    //                          we must call this function
    // OnEnumProp           -   Enumerates all the properties related to the player
    // OnProperty           -   Set/Gets properties for the player
    // OnInit               -   Gets call when initializing the Object.
    // OnColRender          -   Render the collision of the player. (Bounding spheres)
    // OnColCheck           -   Gets call when an Object wants to check the collision agains the player
    // OnRender             -   Gets call when ever we need to render the player
    // OnRenderShadowCast   -   Gets call when ever we render a player shadow
    // OnAdvanceLogic       -   This function gets call ones per game loop. Used to
    //                          advance the logic of the player.
    // OnAnimationInit      -   This function get call when ever an animation is
    //                          assign to the player.
    // GetHealth            -   Returns health. 0=dead, 100=full
    // UseFocusObject       -   Checks for focus objects and uses one if appropriate.  Returns true if used.
    // OnButtonInput        -   Handles button press events
    // OnGameSpeak          -   Handles speaking related button press events.
    // ProcessAnimEvents    -   Handles events from animations
    // UpdateRotation       -   Handles rotating the player (right thumbstick / other things that would effect view)
    // CalculateStrafeVelocity- Calculates the appropriate strafe velocity
    // CalculateForwardVelocity-Calculates appropriate forward velocity
    // ReversingStrafeDirection-Returns true if trying to strafe opposite direction of current strafe
    // ReversingMoveDirection - Returns true if trying to move opposite direction of current motion
    // ParseOnPainForEffects
    //
    // GetIsSafeSpot        -   Call to figure out if the current spot is safe enough to consider a save point
    //
    //=========================================================================

protected:
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;
    void OnInit() override;
    void OnRender() override;
    void OnRenderTransparent() override;
    void OnRenderShadowCast(uint64_t ProjMask) override;
    void OnAdvanceLogic(float DeltaTime) override;

    virtual void OnAnimEvents();
    bool         UseFocusObject();
    virtual void OnButtonInput();
    void         OnGameSpeak();
    void         UpdateStickInput();
    void         ClearStickInput();
    virtual void ProcessSfxEvents();
    bool         NearMutagenReservoir();

    bool DoFootfallCollisions();
    void PlayFootfall(float DeltaTime);

    // Static foot fall sfx lookup (so net ghost can play these sounds)
public:
    static const char* GetFootfallHeel(int Material);
    static const char* GetFootfallToe(int Material);
    static const char* GetFootfallSlide(int Material);
    static const char* GetFootfallLandSweetner(int Material);

protected:
    void ZoomOutAndReload();

    //------------------------------------------------------------------------------
    // Animation initialization routines
    //------------------------------------------------------------------------------
    virtual void OnAnimationInit();
    //void         OnWeaponAnimInit2(inven_item WeaponItem, new_weapon* pWeapon);
    void ResetAnimationTable();
    void ResetWeaponAnimTable2(inven_item WeaponItem);

    virtual Radian3 GetProjectileTrajectory();
    virtual Radian3 ApplyAimDegredation(Radian Pitch, Radian Yaw);

    virtual void UpdateRotation(const float& rDeltaTime);
    virtual void UpdateCameraShake(float DeltaTime);
    virtual void UpdateCharacterRotation(const float& DeltaTime);
    void         UpdateBulletSounds(float DeltaTime);

    // Used for determing various movement properties.
    void         CalculatePitchLimits(const float& rDeltaTime);
    void         CalculateRotationAccelerationFactors(float DeltaTime);
    void         UpdateRotationRates(float DeltaTime);
    virtual void CalculateStrafeVelocity(const Vector3& rViewX, const float& rDeltaTime);
    virtual void CalculateForwardVelocity(const Vector3& rViewZ, const float& rDeltaTime);
    bool         ReversingStrafeDirection(const float& rMaxStrafe);
    bool         ReversingMoveDirection(const float& rMaxForward);
    bool         HasSpeedReversed();

    // Used for determining necessary rig offsets.
    void    CalculateRigOffset(float DeltaTime);
    void    CalculateStrafeRigOffset(float DeltaTime);
    void    CalculateMoveRigOffset(float DeltaTime);
    void    CalculateLookHorozOffset(float DeltaTime);
    void    CalculateLookVertOffset(float DeltaTime);
    Vector3 GetLeanOffset();

    // Used for determing and applying aim assistance.
    void   UpdateAimAssistance(float DeltaTime);
    void   UpdateAimOffset(float DeltaTime);
    void   UpdateCurrentAimTarget(float DeltaTime);
    Radian CalculateNecessaryAimAssistYaw(Object* pObject);
    Radian CalculateNecessaryAimAssistPitch(Object* pObject);
    Radian CalculateActualYawToTarget(Object* pObject);
    Radian CalculateActualPitchToTarget(Object* pObject);

    // Used for scaling raw controller input to allow 'fine tuning'
    void ScaleYawAndPitchValues();

    void UpdateViewCinematic(const float& rDeltaTime);

    void         UpdateSafeSpot(float DeltaTime);
    void         UpdateUserInput(float DeltaTime);
    void         SwitchWeapon2(inven_item WeaponItem) override;
    virtual void OnMoveWeapon();
    virtual void OnTransformWeapon(const Matrix4& L2W);

    void DrawLabelInFront(const char* pLabel);

    // Non exclusive states.
    // Stunned
    void ComputeStunnedPitchYawOffset(Radian PitchOffset, Radian YawOffset);

    guid                 GetThirdPersonCameraGuid() const;
    third_person_camera* GetThirdPersonCamera() const;
    void                 SetupThirdPersonCamera();
    void                 UpdateThirdPersonCamera();

    void    UpdateReticleRadius(float DeltaTime);
    void    AttachWeapon();
    Vector3 GetWeaponCollisionOffset(guid WeaponGuid, const Vector3& FirePos);
    void    SetCurrentStrain();
    void    RenderAimAssistDebugInfo();

    //------------------------------------------------------------------------------
    // Animation initialization routines
    //------------------------------------------------------------------------------
    void LoadAimAssistTweakHandles();
    void LoadAimAssistTweaks();

    bool UsingLoco() override;

    //==============================================================================
    //  OLD PLAYER_ALL_STRAINS STUFF
    //==============================================================================
public:
    // Pickups and inventory
    bool OnPickup(pickup& Pickup) override;
    bool CanTakePickup(pickup& Pickup);
    void ItemAcquiredMessage(inven_item Item);
    void ItemFullMessage(inven_item Item);
    void NoWeapon_NoAmmoPickupMessage(inven_item Item);
    void TakePickup(pickup& Pickup);
    void RemoveAllWeaponInventory();
    int  GetAmmoFromWeaponType(inven_item Item);
    bool CheckForDualWeaponSetup();
    bool LoadWarnsLowAmmo();

    void            OnMoveViewPosition(const Vector3& rNewPos);
    bool            OnStrainProperty(prop_query& rPropQuery);
    void            EnumrateStrainControls(prop_enum& List);
    bool            OnStrainControlProperty(prop_query& I);
    animation_state GetCurrentAnimState() { return m_CurrentAnimState; }

    //------------------------------------------------------------------------------
    // Actor overloads.
    //------------------------------------------------------------------------------

    void DebugSetupInventory(const char* pLevelName);
    void DebugEnableWeapons(const char* pLevelName);
    void InitInventory() override;
    void ReInitInventory();
    bool IsRunning() override { return m_IsRunning; }
    bool InTurret() override { return m_bInTurret; }
    //------------------------------------------------------------------------------
    // Name checking routines.
    //------------------------------------------------------------------------------
    inven_item      GetWeaponFromAnimName(const char* pAnimName);
    animation_state GetAnimStateFromName(const char* pAnimName);

    //------------------------------------------------------------------------------
    // Type checking routines.
    //------------------------------------------------------------------------------
    bool       IsAnimStateAvailable2(inven_item WeaponItem, animation_state AnimState);
    inven_item GetCurrentGrenadeType2() { return m_CurrentGrenadeType2; }

    //------------------------------------------------------------------------------
    // Every frame, player specific.
    //------------------------------------------------------------------------------
    void         OnAliveLogic(float DeltaTime) override;
    void         OnDeathLogic(float DeltaTime) override;
    void         UpdateAudio(float DeltaTime);
    guid         IsInLadderField();
    virtual bool UpdateLadderMovement(float DeltaTime);
    virtual void UpdateMovement(float DeltaTime);
    void         UpdateCrouchHeight(const float& rDeltaTime);

    //------------------------------------------------------------------------------
    // Input handlers
    //------------------------------------------------------------------------------
    bool       ShouldSkipWeaponCycle(const inven_item& CurrentWeaponItem, const inven_item& NextWeapon);
    inven_item GetNextAvailableWeapon2(const cycle_direction& CycleDirection);
    inven_item GetCurrentWeapon2();
    guid       GetCurrentWeaponGuid2();

    void CameraFall(float fPercentHeight);

    //------------------------------------------------------------------------------
    // Strain related methods
    //------------------------------------------------------------------------------
    bool UseAntiMu(collectable_anti_mutagen* pAntiMu);
    void SetupStrain();

protected:
    bool IsChangingMutation() const;
    void HandleFireInput(bool IsAlternateFire);
    bool SetupDualWeaponDiscard(inven_item& WeaponItem);

    // grenade throwin
    void    GetThrowPoints(Vector3& Point1, Vector3& Point2, Vector3& Point3);
    Vector3 SetupGrenadeThrow(const Vector3& EventPos);

    //==============================================================================
    // ANIMATION STATES
    //==============================================================================
    virtual void SetAnimState(animation_state AnimState);
    virtual void SetAnimation(const animation_state& AnimState, const int& nAnimIndex, const float& fBlendTime = DEFAULT_BLEND_TIME);

    virtual void BeginState();
    virtual void UpdateState(const float& rDeltaTime);
    virtual void EndState();

    // ANIM_STATE_SWITCH_TO
    virtual void BeginSwitchTo();
    virtual void UpdateSwitchTo(const float& DeltaTime);
    virtual void EndSwitchTo();

    // ANIM_STATE_SWITCH_FROM
    virtual void BeginSwitchFrom();
    virtual void UpdateSwitchFrom(const float& DeltaTime);
    virtual void EndSwitchFrom();

    // ANIM_STATE_IDLE
    virtual void BeginIdle();
    virtual void UpdateIdle(const float& DeltaTime);
    virtual void EndIdle();

    // ANIM_STATE_RUN
    virtual void BeginRun();
    virtual void UpdateRun(const float& DeltaTime);
    virtual void EndRun();

    // ANIM_STATE_PICKUP
    virtual void BeginPickup();
    virtual void UpdatePickup(const float& DeltaTime);
    virtual void EndPickup();

    // ANIM_STATE_DISCARD
    virtual void BeginDiscard();
    virtual void UpdateDiscard(const float& DeltaTime);
    virtual void EndDiscard();

    //  ANIM_STATE_FIRE
    virtual void BeginFire();
    virtual void UpdateFire(const float& DeltaTime);
    virtual void EndFire();

    //  ANIM_STATE_ALT_FIRE
    virtual void BeginAltFire();
    virtual void UpdateAltFire(const float& DeltaTime);
    virtual void EndAltFire();

    //  ANIM_STATE_GRENADE
    virtual void BeginGrenade();
    virtual void UpdateGrenade(const float& DeltaTime);
    virtual void EndGrenade();

    //  ANIM_STATE_ALT_GRENADE
    virtual void BeginAltGrenade();
    virtual void UpdateAltGrenade(const float& DeltaTime);
    virtual void EndAltGrenade();

    // ANIM_STATE_MELEE_XXX -- Mutant Extreme Melee stuff
    virtual void BeginMelee_Special(const animation_state& AnimState);
    virtual void UpdateMelee_Special(const float& rDeltaTime, const animation_state& AnimState);
    virtual void EndMelee_Special(const animation_state& AnimState);

    //  ANIM_STATE_MELEE
    virtual void BeginMelee();
    virtual void UpdateMelee(const float& DeltaTime);
    virtual void EndMelee();

    // ANIM_STATE_COMBO_BEGIN
    virtual void BeginCombo();
    virtual void UpdateCombo(const float& rDeltaTime);
    virtual void EndCombo();
    virtual void ClearCombo();

    // ANIM_STATE_COMBO_HIT
    virtual void BeginCombo_Hit();
    virtual void UpdateCombo_Hit(const float& rDeltaTime);
    virtual void EndCombo_Hit();

    // ANIM_STATE_COMBO_HIT
    virtual void BeginCombo_End();
    virtual void UpdateCombo_End(const float& rDeltaTime);
    virtual void EndCombo_End();

    //  ANIM_STATE_RELOAD
    virtual void BeginReload();
    virtual void UpdateReload(const float& DeltaTime);
    virtual void EndReload();

    //  ANIM_STATE_RELOAD_IN
    virtual void BeginReloadIn();
    virtual void UpdateReloadIn(const float& DeltaTime);
    virtual void EndReloadIn();

    //  ANIM_STATE_RELOAD_OUT
    virtual void BeginReloadOut();
    virtual void UpdateReloadOut(const float& DeltaTime);
    virtual void EndReloadOut();

    //  ANIM_STATE_RAMP_UP
    virtual void BeginRampUp();
    virtual void UpdateRampUp(const float& DeltaTime);
    virtual void EndRampUp();

    //  ANIM_STATE_RAMP_DOWN
    virtual void BeginRampDown();
    virtual void UpdateRampDown(const float& DeltaTime);
    virtual void EndRampDown();

    //  ANIM_STATE_ALT_RAMP_UP
    virtual void BeginAltRampUp();
    virtual void UpdateAltRampUp(const float& DeltaTime);
    virtual void EndAltRampUp();

    //  ANIM_STATE_ALT_RAMP_DOWN
    virtual void BeginAltRampDown();
    virtual void UpdateAltRampDown(const float& DeltaTime);
    virtual void EndAltRampDown();

    //  ANIM_STATE_HOLD
    virtual void BeginHold();
    virtual void UpdateHold(const float& DeltaTime);
    virtual void EndHold();

    //  ANIM_STATE_ALT_HOLD
    virtual void BeginAltHold();
    virtual void UpdateAltHold(const float& DeltaTime);
    virtual void EndAltHold();

    //  ANIM_STATE_ZOOM_IN
    virtual void BeginZoomIn();
    virtual void UpdateZoomIn(const float& DeltaTime);
    virtual void EndZoomIn();

    //  ANIM_STATE_ZOOM_OUT
    virtual void BeginZoomOut();
    virtual void UpdateZoomOut(const float& DeltaTime);
    virtual void EndZoomOut();

    //  ANIM_STATE_ZOOM_IDLE
    virtual void BeginZoomIdle();
    virtual void UpdateZoomIdle(const float& DeltaTime);
    virtual void EndZoomIdle();

    //  ANIM_STATE_ZOOM_RUN
    virtual void BeginZoomRun();
    virtual void UpdateZoomRun(const float& DeltaTime);
    virtual void EndZoomRun();

    //  ANIM_STATE_ZOOM_FIRE
    virtual void BeginZoomFire();
    virtual void UpdateZoomFire(const float& DeltaTime);
    virtual void EndZoomFire();

    //  ANIM_STATE_DEAD
    virtual void BeginDeath();
    virtual void UpdateDeath(const float& DeltaTime);
    virtual void EndDeath();

    //  ANIM_STATE_CHANGE_MUTATION,
    virtual void BeginMutationChange();
    virtual void UpdateMutationChange(float DeltaTime);
    virtual void EndMutationChange();

    //  ANIM_STATE_FALLING_TO_DEATH
    virtual void BeginFallingToDeath();
    virtual void UpdateFallingToDeath(float DeltaTime);
    virtual void EndFallingToDeath();

    //  ANIM_STATE_CINEMA
    virtual void BeginCinema();
    virtual void UpdateCinema(float DeltaTime);
    virtual void EndCinema();

    //  ANIM_STATE_MISSION_FAILED
    virtual void BeginMissionFailed();
    virtual void UpdateMissionFailed(float DeltaTime);
    virtual void EndMissionFailed();

    //------------------------------------------------------------------------------
    // Generic state helper routines.
    //------------------------------------------------------------------------------
    animation_state GetMotionTransitionAnimState();
    void            GenerateFiringAnimPercentages();
    int             GetNextFiringAnimIndex();
    void            ForceNextWeapon();
    void            ForceMutationChange(bool bMutate) override;

    //------------------------------------------------------------------------------
    // Non exclusive state machine.
    //------------------------------------------------------------------------------
    void SetNonExclusiveState(non_exclusive_states nStateBit);
    void ClearNonExclusiveState(non_exclusive_states nStateBit);
    void ClearAllNonExclusiveStates();

    void BeginNonExclusiveState(non_exclusive_states nStateBit);
    void UpdateActiveNonExclusiveStates(float DeltaTime);
    void EndNonExclusiveState(non_exclusive_states nStateBit);

    // Non exclusive states.
    // Stunned
    void         BeginStunnedNE();
    void         UpdateStunnedNE(float DeltaTime);
    void         EndStunnedNE();
    virtual void ProcessStunnedPain(const pain& Pain);

    const char* GetCurrentWeaponName();

    void ResetWeaponFlags();
    // IJB new_weapon::reticle_radius_parameters GetReticleParams();
    virtual int GetWeaponRenderState() override;

public:
    Vector3 GetEyesPosition() const;
    void    GetEyesPitchYaw(Radian& Pitch, Radian& Yaw) const;
    Vector3 GetPositionWithOffset(eOffsetPos offset) override;

    const Matrix4& GetL2W() const;

    void EndZoomState();

    virtual bool  AddMutagen(const float& nDeltaMutagen);
    virtual float GetMutagen() { return m_Mutagen; }
    virtual float GetMaxMutagen() { return m_MaxMutagen; }
    virtual void  SetMutagen(float mutagenAmount) { m_Mutagen = mutagenAmount; }
    void          UpdateConvulsion();

    //  virtual         void            ContagionTick           ();
    void ContagionDOT() override;

    skin_inst&         GetThirdPersonInst() { return m_SkinInst; }
    AnimGroup::handle& GetAnimGroupHandle() { return m_hAnimGroup; }

    void UpdateFellFromAltitude() override;
    void TakeFallPain() override;

    bool ReloadWeapon(const new_weapon::ammo_priority& Priority, bool bCheckAmmo = true);
    void LogWeapons();

protected:
    int GetMaterial() const override { return MAT_TYPE_FLESH; }

public:
    void CreateAllWeaponObjects();

    void    OnColCheck() override;
    BBox    GetLocalBBox() const override;
    BBox    GetColBBox() override;
    Vector3 GetVelocity() const override;
    void    ScaleVelocity(const Vector3& PlaneNormal, float PerpScale, float ParaScale) override;

    bool IsCinemaRunning() { return m_Cinema.m_bCinemaOn; }

    //--------------------------------------------------------------------------
    // Input handlers
    //--------------------------------------------------------------------------
    void OnPrimaryFire();
    void OnSecondaryFire();
    void OnReload();
    bool AllowedToFire();

protected:
    static view m_Views[MAX_LOCAL_PLAYERS]; // Views for all the players
    view_info   m_ViewInfo;                 // persistent information for the player's view
    view_info   m_OriginalViewInfo;         // original persistent information for the player's view
    Vector3     m_RespawnPosition;          // Position where the player re-spawns after dying
    uint8_t     m_RespawnZone;              // Zone to respawn in
    guid        m_ThirdPersonCameraGuid;    // GUID of third person camera if we're using one

    float  m_PitchAccelTime;                 // How long does it take to get to full pitch turning speed?
    float  m_YawAccelTime;                   // How long does it take to get to full yaw turning speed?
    Radian m_PitchRate;                      // How fast pitch is changing, positive or negative, radians/second
    Radian m_YawRate;                        // How fast yaw   is changing, positive or negative, radians/second
    float  m_PitchAccelFactor;               // Ranges from 0 - 1 over the course of m_PitchAccelTime.
    float  m_YawAccelFactor;                 // Ranges from 0 - 1 over the course of m_YawAccelTime.
    float  m_PitchMax;                       // Maximum pitch of the player
    float  m_PitchMin;                       // Minimum pitch of the player
    float  m_DesiredPitchMax;                // Desired maximum pitch for the player
    float  m_DesiredPitchMin;                // Desired minimum pitch for the player
    float  m_fYawStickSensitivity;           // How responsive is the right stick ( 0.f - Any.f )
    float  m_fPitchStickSensitivity;         // How responsive is the right stick ( 0.f - Any.f )
    float  m_fOriginalYawStickSensitivity;   // Original responsiveness of the right stick.
    float  m_fOriginalPitchStickSensitivity; // Original responsiveness of the right stick.

    Vector3 m_EyesOffset;  // This offset is from the top of his head.
    float   m_ShakeTime;   // Shake time remaining
    float   m_ShakeAngle;  // Shake sine angle
    float   m_ShakeAmount; // How much to shake the camera.
    float   m_ShakeSpeed;  // How fast to shake the camera.

    int   m_ActivePlayerPad; // The pad that this player is currently using.
    int   m_LocalSlot;
    float m_MaxFowardVelocity; // Maximum foward/Backwards velocity
    float m_MaxStrafeVelocity; // Maximum strafe velocity
    float m_JumpVelocity;      // How much velocity he has at the take off of the jump

    Vector3 m_ForwardVelocity;     // Actual forward velocity
    Vector3 m_StrafeVelocity;      // Actual strafe velocity
    float   m_fForwardAccel;       // Scalar rate of forward acceleration
    float   m_fStrafeAccel;        // Scalar rate of strafe acceleration
    float   m_fForwardSpeed;       // Scalar forward speed
    float   m_fCurrentYawOffset;   // Current yaw offset for character
    float   m_fCurrentPitchOffset; // Current pitch offset for character
    float   m_fPitchChangeSpeed;   // Rate of change for the pitch
    float   m_fPrevForwardSpeed;   // Forward speed last frame
    float   m_fStrafeSpeed;        // Scalar strafe speed
    float   m_fDecelerationFactor; // Multiply acceleration by this when slowing down so the player slows down quicker than he speeds up.
    float   m_SoftLeanAmount;      // Lean amount softened by a sine wave
    Vector3 m_LeanWeaponOffset;    // Keeps the weapon in the center when leaning

    float m_PitchArmsScalerPositive; // Scales the pitch>0 before is used for the arms
    float m_PitchArmsScalerNegative; // Scales the pitch<0 before is used for the arms

    float m_fCurrentCrouchFactor;
    float m_fCrouchChangeRate;
    bool  m_bInTurret;

    const char* m_pPlayerTitle; // The title of the player such (Stage 5)

    float m_fMinWalkSpeed; // Minimum speed needed to walk
    float m_fMinRunSpeed;  // Minimum speed needed to run

    int m_iCameraBone; // Which Bone in the skeleton is the camera bone

    int m_NonExclusiveStateBitFlag; // Bits representing which non-exclusive states are active.

    // Stunned state variables.
    Radian m_PreStunPitch;       // What was the pitch going into the stun.
    Radian m_PreStunYaw;         // What was the yaw going into the stun.
    Radian m_PreStunRoll;        // What was the roll going into the stun.
    float  m_fStunnedTime;       // How long have we been stunned.
    float  m_fMaxStunTime;       // How long do we stay stunned for?
    Radian m_MaxStunPitchOffset; // Maximum pitch offset while stunned.
    Radian m_MaxStunYawOffset;   // Maximum yaw offset while stunned.
    Radian m_MaxStunRollOffset;  // Maximum roll offset while stunned.
    float  m_fStunYawChangeSpeed;
    float  m_fStunPitchChangeSpeed;
    float  m_fStunRollChangeSpeed;

    bullet_fly_by m_BulletFlyBy[MAX_FLY_BYS];

    float m_fShakeAmpScalar;
    float m_fShakeFreqScalar;
    float m_fShakeMaxPitch;
    float m_fShakeMaxYaw;
    float m_PeakLandVelocity; // The maximum vertival velocity the player reached while landing.
    float m_PeakJumpVelocity; // The maximum vertival velocity the player reached while jumping.

    guid m_LoreObjectGuids[MAX_LORE_ITEMS]; // The lore objects for this level
    bool m_bAllLoreObjectsCollected;        // have we collected all the lore objects
    bool m_bJBGLoreAcquired;

    float m_ReticleMovementDegrade;

    float m_InvalidSoundTimer;

protected:
    // Values for adjusting the rig's position relative to the camera to cause 'gun movement'
    Vector3 m_vRigOffset;
    float   m_fRigMaxStrafeOffset;
    float   m_fRigMaxMoveOffset;
    float   m_fRigMoveOffsetVelocity;
    float   m_fRigStrafeOffsetVelocity;
    float   m_fCurrentMoveRigOffset;
    float   m_fCurrentStrafeRigOffset;

    // Values for adjusting the rig's rotation relative to the camera to cause 'gun movement'
    Radian3 m_RigLookOffset;
    Radian  m_RigLookMaxVertOffset;
    Radian  m_RigLookMaxHorozOffset;
    Radian  m_RigLookVertVelocity;
    Radian  m_RigLookHorozVelocity;
    Radian  m_CurrentVertRigOffset;
    Radian  m_CurrentHorozRigOffset;

    // Values for augmenting the control input relating to aim assistance
    float m_fCurrentPitchAimModifier;
    float m_fCurrentYawAimModifier;

    // Values for scaling raw controller input to allow 'fine tuning'
    float m_fFineTuneThreshold;
    float m_fYawValueAtFineTuneThreshold;
    float m_fPitchValueAtFineTuneThreshold;

    float m_fMidRangeThreshold;
    float m_fYawValueAtMidrangeThreshold;
    float m_fPitchValueAtMidrangeThreshold;

    Radian m_YawAimOffset;

    float m_TimeSinceLastZonePain;

    std::vector<pain> m_LastPainEvent;

    int m_AudioEarID;

protected:
    static guid s_ActivePlayerGuid; // The guid of the active player

    //controller stick input for this frame.
    float m_fMoveValue;
    float m_fStrafeValue;
    float m_fYawValue;
    float m_fPitchValue;
    float m_fRawControllerYaw;
    float m_fRawControllerPitch;

    bool m_bVoteButtonPressed;
    bool m_bRespawnButtonPressed;

    // Controller input from last frame.
    float m_fPreviousYawValue;
    float m_fPreviousPitchValue;

    //  Values used to help determine if the player is currently safe
    float m_LastTimeSeenByEnemy; // Last time a unit fired at us
    float m_LastTimeTookDamage;  // Last time player took damage

    Vector3           m_PositionOfLastSafeSpot; // Position of last safe spot
    zone_mgr::zone_id m_ZoneIDOfLastSafeSpot;   // Zone ID of last safe spot

    Vector3           m_NextPositionOfLastSafeSpot; // Spot that will become the new safe spot
    zone_mgr::zone_id m_NextZoneIDOfLastSafeSpot;   // ID that will become Zone ID of last safe spot

    float m_AimDegradation;  // How bad aim is (0 best, 1 worst)
    float m_AimRecoverSpeed; // How quickly to recover aim (0.1 = slow, 1 = fast)

    float m_YawMod; // Altered view based on pain impacts;
    float m_PitchMod;
    float m_RollMod;
    float m_ShakePitch;
    float m_ShakeYaw;
    xtick m_NearbyObjectCounter;
    xtick m_GameSpeakCounter;
    guid  m_SpeakToGuid; // Guid of person who the player MAY speak to.
                         // Updated every .25 seconds.
    guid m_GameSpeakEmitterGuid;

    float m_ProximityAlertRadius;

    guid                        m_CurrentTargetingAssistTarget; // guid of the Object that we are targeting.
    float                       m_DistanceToAimAssistTarget;
    Radian3                     m_CurrentTargetingModifation;
    Radian3                     m_OffsetToTarget;
    float                       m_AimAssistPct;
    std::vector<prop_container> m_SaveSpotProperties; // backup of properties for save spots

protected:
    //Quick implementation of the lock view, needs to be replaced by the cinematic engine...

    lock_view_node m_LockViewTable[lock_player_view::MAX_TABLE_SIZE];
    xtick          m_TimeStartTick;
    bool           m_ViewCinematicPlaying;
    int            m_CurrentViewNode;
    Quaternion     m_StartView;
    Quaternion     m_DesiredView;

    float m_CScale;
    float m_CTimeSum;

protected:
    bool m_bActivePlayer;
    bool m_bSpeaking;

    guid m_LastLadderGuid;
    guid m_JumpedOffLadderGuid;

    weapon_state m_WeaponState;
    float        m_ReticleRadius;
    float        m_ReticleGrowSpeed;
    float        m_ReticleShotPenalty;

    Vector3       m_ArmsOffset;
    Vector3       m_ArmsVelocity;
    Vector3       m_WeaponCollisionOffset;
    float         m_LastWeaponCollisionOffsetScalar;
    float         m_WeaponCollisionOffsetScalar; // this is the one we use for springing pullback
    AimAssistData m_AimAssistData;

    // Hide player arms for things like the decontamination sequence
    bool m_bHidePlayerArms;
    bool m_bArmsWereHidden; // used so we can tell if we need to play the switch to anim
    bool m_bPlaySwitchTo;   // if this is true, this will play the "switchto" animation after arms re-appear from m_bHidePlyerArms

    // Can we do a tap fire?  Resets when the player releases the button (or maybe in the future after a timer?).
    bool  m_bCanTapFire;
    float m_TapRefireTime;

    //------------------------------------------------------------------------------
    // Flashlight info
    //------------------------------------------------------------------------------
    bool  m_bUsingFlashlight;
    bool  m_bUsingFlashlightBeforeCinema;
    guid  m_FlashlightGuid;    // guid to a projected texture (attached to the gun)
    float m_BatteryChangeTime; // what is the accumulated time for changing battery value
    float m_Battery;
    float m_MaxBattery;
    float m_FlashlightTimeout;

    float m_fLastItemFullTime;
    float m_fLastItemAcquiredTime;
    //------------------------------------------------------------------------------

    //===================================================================
    // CINEMATIC MEMBERS
    //===================================================================
protected:
    struct cinema
    {
        bool    m_bCinemaOn;              // Is cinema mode on?
        bool    m_bPlayerZoneInitialized; // Has player zone been initialized to cameras?
        guid    m_LookAtTargetGuid;       // Look at a target during the cinema
        Vector3 m_CurrentLookDir;         // Current looking direction
        Vector3 m_DesiredLookDir;         // Desired looking direction
        float   m_BlendInTime;            // Cinema view blend in time
        float   m_CurrentBlendInTime;     // Current view blend in time
        guid    m_CinemaCameraGuid;       // Cinema camera
        Vector3 m_ViewCorrectionDelta;    // Used to put view back to the player
        bool    m_bUseViewCorrection;     // Use the view correction blending?
        Matrix4 m_CameraV2W;              // Camera view to world matrix for final view
        Radian  m_CameraXFOV;             // Field of view to use for final view
    };

    cinema m_Cinema;

    //===================================================================
    // OLD PLAYER_ALL_STRAINS MEMBERS
    //===================================================================
protected:
    strain_control_modifiers m_StrainControls;
    bool                     m_bStrainInitialized;
    bool                     m_PlayMeleeSound;
    bool                     m_IsRunning;
    int                      m_ZoomLevelAfterReload;

    //animation tables / players for all of the strains.
    state_anims          m_Anim[INVEN_NUM_WEAPONS][ANIM_STATE_MAX];
    AnimGroup::handle    m_AnimGroup;         // Animation file for the player
    ResourceHandle<char> m_hAudioPackage;     // Audio resource for the player.
    skin_inst            m_Skin;              // Geometry use to render the player
    int                  m_iCameraTargetBone; // Which bone in the skeleton is the target bone
    uint32_t             m_StrainFriendFlags; //  Flags of our friends.

    char_anim_player m_AnimPlayer; // Animation player used to play the arms animations

    animation_state m_CurrentAnimState;  //current animation state of the player
    animation_state m_PreviousAnimState; //previous animation state of the player
    animation_state m_NextAnimState;     //next animation state of the player
    int             m_AnimStage;         //used to track where in a series of anims, that we are
                                         //valid until next state change

    animation_state m_MeleeAnimStates[MAX_MELEE_STATES]; // randomized list to make sure we don't play the same attack consecutively
    int             m_MeleeAnimStateIndex;               // Save off the index we are using so that we can walk the list.

    inven_item m_CurrentGrenadeType2; // The Current grenade that is equiped.

    int m_CurrentAnimIndex;       // Current animation index that is playing
    int m_PreviousAnimIndex;      // Previous animation index that was playing.
    int m_CurrentAnimStateIndex;  // Current animation state index that is playing
    int m_PreviousAnimStateIndex; // Previous animation state index that is playing

    float m_fAnimationTime; // Time animation has been playing
    float m_fMaxAnimTime;   // Max amount of time an animation can play

    float m_fAnimPriorityPercentage[MAX_ANIM_PER_STATE]; //Values used to determine which animations we are going to play

    Vector3 m_PosOverrideCamera;   // Override camera position (used for death rather than using physic position)
    float   m_WpnHoldTime;         // How long the weapon has been held
    float   m_LastTimeWeaponFired; // Last time player fired his weapon

    // Ladder variables
    bool    m_bOnLadder;    // true if player is on a ladder
    Vector3 m_LadderOutDir; // Ladder out direction (only valid if on a ladder)

    float      m_MaxAnimWeaponHoldTime; // How long till be get to "jerkiest" animation when in weapon hold state;
    int        m_MutationAudioLoopSfx;
    bool       m_NeedRelaodIn;
    int        m_LastFireAnimStateIndex;
    int        m_nLoreDiscoveries; // 1st pass at unlocking secret galleries
    float      m_DebounceTime;
    bool       m_bWasMutated;
    bool       m_bIsMutantVisionOn;  // toggles based on super-event in weapon animation
    inven_item m_PreMutationWeapon2; // what was our weapon before we mutated
public:
    bool m_bMutationMeleeEnabled;         // Can we use the mutation melee attack?
    bool m_bPrimaryMutationFireEnabled;   // Can we fire our primary mutation ammo?
    bool m_bSecondaryMutationFireEnabled; // Can we fire our secondary mutation ammo?
protected:
    bool            m_bMeleeLunging;       // are we in the middle of a lunge?
    bool            m_bHolsterWeapon;      // is our weapon in its holster?
    float           m_MeleeDamage;         // How much damage per melee (not mutant melee)
    float           m_MeleeForce;          // How much force from our melee (not mutant melee)
    bool            m_bInMutationTutorial; // is the mutation tutorial running?
    convulsion_info m_ConvulsionInfo;      // stores info for our mutation tutorial convulsions
    float           m_ConvulsionFeedbackDuration;
    float           m_ConvulsionFeedbackIntensity;
    bool            m_bHitCombo;        // did we hit the melee button and activate another combo within the threshold?
    bool            m_bCanRequestCombo; // are we in the request threshold?
    bool            m_bLastMeleeHit;    // flag to make sure we hit something during a weapon melee
    char            m_ComboCount;       // how many times have we combo'd?

    bool m_bTweakHandlesLoaded; // have the tweak handles been loaded?

    //------------------------------------------------------------------------------
    // Begin members from ghost.cpp/hpp
    //------------------------------------------------------------------------------

protected:
    float   m_Mutagen;
    float   m_MaxMutagen;   // Player's maximum mutagen level
    Vector3 m_EyesPosition; // Where the player's eyes are after the last ComputeView() call
    Radian  m_EyesPitch;    // Eye's pitch after the last ComputeView() call
    Radian  m_EyesYaw;      // Eye's yaw after the last ComputeView() call

    // voice_id m_SuckingMutagenLoopID; // the sound getting mutagen from a SCDB makes.

#if !defined(CONFIG_RETAIL)
    bool m_bRenderSkeleton;      // Whether to render the skeleton or not
    bool m_bRenderSkeletonNames; // When ever the skeleton gets render whther to also print the name of the bones
    bool m_bRenderBBox;          // Renders the bbox of the player this is use mainly in the editor
#endif                           // !defined( CONFIG_RETAIL )

public:
    inven_item m_PrevWeaponItem;
    inven_item m_NextWeaponItem;

protected:
    // Locomotion
    character_physics m_Physics; // Physics to drive the motion of the player
    player_loco       m_Loco;
    float             m_DeathTime;

    bool    m_bFalling; // used for pain when we land
    bool    m_bJustLanded;
    Vector3 m_DeltaPos; // current velocity / delta time
    bool    m_bCanJump;

    float m_DeltaTime;
    float m_TimeInState;

    int                    m_MissionFailedTableName;
    int                    m_MissionFailedReasonName;
    ResourceHandle<Bitmap> m_MissionFailedBmp;

    //------------------------------------------------------------------------------
    // End members from ghost.cpp/hpp
    //------------------------------------------------------------------------------

    bool m_VoteMode;

    //------------------------------------------------------------------------------
    // Footfall members
    //------------------------------------------------------------------------------

    float m_DelayTillNextStep;
    float m_DistanceTraveled;
    float m_DelayCountDown;
    int   m_HeelID;
    int   m_SlideID;
    int   m_ToeID;
    int   m_TrailStep;

    //------------------------------------------------------------------------------
    // Manned turret members
    //------------------------------------------------------------------------------
    struct manned_turret
    {
        guid       TurretGuid;        // guid of the turret we're manning -- this will be hidden
        guid       Turret2Guid;       // guid of a part of the turret we're manning -- this will be hidden
        guid       Turret3Guid;       // guid of a part of the turret we're manning -- this will be hidden
        guid       LeftBoundaryGuid;  // guid of the Object whose position is to be used for left aim boundary
        guid       RightBoundaryGuid; // guid of the Object whose position is to be used for right aim boundary
        guid       UpperBoundaryGuid; // guid of the Object whose position is to be used for upper aim boundary
        guid       LowerBoundaryGuid; // guid of the Object whose position is to be used for lower aim boundary
        inven_item PreviousWeapon;    // weapon before we manned the turret
        Matrix4    PreviousL2W;       // L2W before we manned the turret
        Matrix4    AnchorL2W;
    };
    manned_turret m_Turret;
    float         m_MutationChangeTime;
    float         m_UseTime; // how long since we last used an item

    //------------------------------------------------------------------------------
    //  State manager movie control
    //------------------------------------------------------------------------------
public:
    static bool s_bPlayerDied; // Set in player::OnDeath.  Cleared by level_loader::LoadLevel
                               // when level initially loads.  Checked by
                               // state mgr in state_mgr::EnterSinglePlayerLoadMission
protected:
    friend class check_point_mgr;
    friend class state_mgr;
};

//=========================================================================

inline Vector3 player::GetVelocity() const
{
    // Use physics
    return Vector3(0, 0, 0); //m_Physics.GetVelocity();
}

//=========================================================================

inline void player::ResetStickSensitivity()
{
    m_fPitchStickSensitivity = m_fOriginalPitchStickSensitivity;
    m_fYawStickSensitivity = m_fOriginalYawStickSensitivity;
}

//===========================================================================

inline void player::SetStickSensitivity(const float& rMultiplier)
{
    m_fPitchStickSensitivity *= rMultiplier;
    m_fYawStickSensitivity *= rMultiplier;
}

//===========================================================================

inline float player::GetCollisionHeight()
{
    return 0.0; //m_Physics.GetColHeight();
}

//===========================================================================

inline float player::GetCollisionRadius()
{
    return 0.0; //m_Physics.GetColRadius();
}

//===========================================================================

inline void player::AddImpulse(Vector3& rImpulse)
{
    //m_Physics.AddVelocity(rImpulse);
}

//===========================================================================

inline inven_item player::GetCurrentWeapon2()
{
    return m_CurrentWeaponItem;
}

//===========================================================================
inline guid player::GetThirdPersonCameraGuid() const
{
    return m_ThirdPersonCameraGuid;
}

//==============================================================================

inline Vector3 player::GetEyesPosition() const
{
    return m_EyesPosition;
}

//==============================================================================

inline void player::GetEyesPitchYaw(Radian& Pitch, Radian& Yaw) const
{
    Pitch = m_EyesPitch;
    Yaw = m_EyesYaw;
}

//==============================================================================
inline player::convulsion_info::convulsion_info()
    : m_TimeSinceLastConvulsion(0.0f)
    , m_ConvulseAtTime(0.0f)
    , m_bConvulsingNow(false)
    , m_TimeLeftInThisConvulsion(0.0f)
{
}

inline bool player::IsChangingMutation() const
{
    return ((m_CurrentAnimState == ANIM_STATE_SWITCH_TO) || (m_CurrentAnimState == ANIM_STATE_SWITCH_FROM)) && (m_CurrentWeaponItem == INVEN_WEAPON_MUTATION);
}
