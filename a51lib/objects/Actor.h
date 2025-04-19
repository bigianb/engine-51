#pragma once

#include "Object.h"
#include "Pickup.h"
#include "../RTTI.h"
#include "../loco/Loco.h"
#include "../characters/factions.h"
#include "../zoneManager/ZoneManager.h"
#include "../painManager/Pain.h"
#include "../decals/DecalPackage.h"
#include "render/SkinInst.h"

#include "CorpsePain.h"
#include "NewWeapon.h"
#include "Pickup.h"
#include "../inventory/Inventory.h"
#include "../characters/FloorProperties.h"
#include "../loco/LocoAnimController.h"
#include "../math/Random.h"

#include <cassert>

class actor_effects;
class pain_queue;
class corpse;
class check_point_mgr;

class actor_health
{
protected:
    float m_Health;
    float m_Inc;
    float m_Dec;

public:
    actor_health();
    ~actor_health() {};

    void  Reset(float Health = 100.0f);         // Set Health and Inc, clear Dec.
    void  Dead();                               // Set Dec to Inc, and thus Health to 0.
    void  Add(float Delta, bool bForce = true); // Add to health.
    void  Sub(float Delta, bool bForce = true); // Subtract from health.
    void  SetInc(float Inc);                    // If greater, use new Inc, update Health.
    void  SetDec(float Dec);                    // If greater, use new Dec, update Health.
    float GetHealth() { return (m_Health); };   // Get current health.
    float GetInc() { return (m_Inc); };         // Get ALL increases to health.
    float GetDec() { return (m_Dec); };         // Get ALL reductions in health.
};

//=========================================================================

inline actor_health::actor_health()
{
    m_Health = -42.0f; // Nonsense value.
    m_Inc = 0.0f;
    m_Dec = -42.0f;
}

//=========================================================================

inline void actor_health::Reset(float Health)
{
    m_Health = Health;
    m_Inc = Health;
    m_Dec = 0.0f;
}

//=========================================================================

inline void actor_health::Dead()
{
    m_Health = 0.0f;
    m_Inc = m_Dec;
}

//=========================================================================

inline void actor_health::Add(float Delta, bool bForce /* = true */)
{
    if (Delta > 0.0f) {
        m_Inc += Delta;
    } else {
        m_Dec -= Delta;
    }
    m_Health = m_Inc - m_Dec;
    if (m_Health < 0.0f) {
        m_Health = 0.0f;
    } else if (!bForce && (m_Health > 100.0f)) {
        m_Health = 100.0f;
        m_Inc = m_Health + m_Dec;
    }
}

//=========================================================================

inline void actor_health::Sub(float Delta, bool bForce /* = true */)
{
    if (Delta > 0.0f) {
        m_Dec += Delta;
    } else {
        m_Inc -= Delta;
    }
    m_Health = m_Inc - m_Dec;
    if (m_Health < 0.0f) {
        m_Health = 0.0f;
    } else if (!bForce && (m_Health > 100.0f)) {
        m_Health = 100.0f;
        m_Dec = m_Inc - m_Health;
    }
}

//=========================================================================

inline void actor_health::SetInc(float Inc)
{
    if (Inc > m_Inc) {
        m_Inc = Inc;
        m_Health = m_Inc - m_Dec;
    }
}

//=========================================================================

inline void actor_health::SetDec(float Dec)
{
    if (Dec > m_Dec) {
        m_Dec = Dec;
        m_Health = m_Inc - m_Dec;
        if (m_Health < 0.0f) {
            m_Health = 0.0f;
        }
    }
}

class actor : public Object
{
public:
    CREATE_RTTI(actor, Object, Object);
    // Dialog types
    enum eDialogType
    {
        DIALOG_NONE = 0,
        DIALOG_ALERT,
        DIALOG_RUSH,
        DIALOG_KILL,
        DIALOG_CLEAR,
        DIALOG_FLEE,
        DIALOG_FRIENDLY_HIT,
        DIALOG_FRIENDLY_WOUND,
        DIALOG_GRENADE_THROW,
        DIALOG_GRENADE_SPOT,
        DIALOG_COVER,
        DIALOG_COVER_REQ,
        DIALOG_MANDOWN,
        DIALOG_RELOAD,
        DIALOG_UNDER_FIRE,
        DIALOG_HIT,
        DIALOG_HIT_MELEE,
        DIALOG_FLINCH,
        DIALOG_DIE_MELEE,
        DIALOG_DIE_GUNFIRE,
        DIALOG_DIE_EXPLOSION,
        DIALOG_DIE_FALL,
        DIALOG_TYPE_COUNT = DIALOG_DIE_FALL,

        DIALOG_SCANNER_VO,
        DIALOG_ANIM_EVENT,
        DIALOG_GOAL_DIALOG,
        DIALOG_TRIGGER_NO_INTERRUPT,
    };

    // Deaths
    enum death_type
    {
        DEATH_BY_ANIM,
        DEATH_BY_EXPLOSION,
        DEATH_BY_RAGDOLL,
        DEATH_SIMPLE
    };

    // Locomotion character animation player controller mappings
    enum anim_flags
    {
        ANIM_FLAG_IMPACT_CONTROLLER = loco::ANIM_FLAG_CONTROLLER0,
        ANIM_FLAG_SHOOT_CONTROLLER = loco::ANIM_FLAG_CONTROLLER1
    };

    // General hit location
    enum general_hit_location
    {
        HIT_INVALID = -1,
        HIT_HIGH,
        HIT_MIDDLE,
        HIT_LOW
    };

    // Hit type - used to decide which impact animations to play
    enum eHitType
    {
        HITTYPE_HARD,
        HITTYPE_LIGHT,
        HITTYPE_IDLE,
        HITTYPE_PLAYER_MELEE_1,
    };

    // Motion direction when dying
    enum death_motion_direction
    {
        DEATH_MOVE_INVALID = -1,
        DEATH_MOVE_LEFT,
        DEATH_MOVE_RIGHT,
        DEATH_MOVE_FORWARD,
        DEATH_MOVE_BACK
    };

    // Offsets to use when returning position
    enum eOffsetPos
    {
        OFFSET_NONE,        // None
        OFFSET_CENTER,      // ?
        OFFSET_EYES,        // Between the eyes (if the actor has both of them)
        OFFSET_AIM_AT,      // Insert ammo here
        OFFSET_TOP_OF_BBOX, // ?
    };

    // Avatar mutation state info
    enum avatar_mutation_state
    {
        AVATAR_NORMAL,
        AVATAR_MUTATING,
        AVATAR_MUTANT,
        AVATAR_NORMALING // mreed: heh heh
    };

    enum mutagen_burn_mode
    {
        MBM_NORMAL_CAMPAIGN,
        MBM_FORCED,
        MBM_AT_WILL,
        MBM_MAX
    };

    // cloaking state info
    enum cloak_state
    {
        CLOAKING_ON,
        CLOAKING_TURNING_OFF,
        CLOAKING_OFF,
        CLOAKING_TURNING_ON,
    };

    // Lean state
    enum lean_state
    {
        LEAN_NONE,
        LEAN_LEFT,
        LEAN_RIGHT,
        LEAN_RETURN_FROM_LEFT,
        LEAN_RETURN_FROM_RIGHT,

        LEAN_COUNT
    };

public:
    actor(ObjectManager* pObjectManager);
    virtual ~actor();

    // Property functions
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;

    // Object virtual functions
    void OnInit() override;
    void OnKill() override;
    void OnAdvanceLogic(float DeltaTime) override;

    void OnRender() override;
    void OnRenderShadowCast(uint64_t ProjMask) override;
    void OnRenderTransparent() override;

    void    OnColCheck() override;
    void    OnMove(const Vector3& NewPos) override;
    void    OnTransform(const Matrix4& L2W) override;
    BBox    GetColBBox() override;
    BBox    GetLocalBBox() const override;
    Vector3 GetVelocity() const override;

    virtual void OnRenderWeapon();
    virtual void OnAliveLogic(float DeltaTime);
    virtual void OnDeathLogic(float DeltaTime);

    void OnAdvanceGhostLogic(float DeltaTime);

    // Health functions
    bool          AddHealth(float DeltaHealth);
    float         GetHealth() override;
    virtual void  ResetHealth(float Health);
    virtual float GetMaxHealth();
    virtual void  SetMaxHealth(float MaxHealth);
    virtual float GetParametricHealth();
    bool          DoAliveLogic();
    virtual bool  GetCanDie() { return m_bCanDie; }
    void          SetCanDie(bool bCanDie) { m_bCanDie = bCanDie; }

    // Faction/friend functions
    factions GetFaction() const;
    factions GetFactionForGuid(guid Guid) const;
    void     SetFaction(factions NewFaction);
    bool     IsFriendlyFaction(factions Faction) const;
    bool     IsEnemyFaction(factions Faction) const;
    void     SetFriendFlags(uint32_t NewFriends);
    uint32_t GetFriendFlags() const;
    bool     IsAlly(const actor* pActor) const;
    bool     IsEnemy(const actor* pActor) const;

    // Inventory functions
    virtual void ReloadAllWeapons();
    virtual bool AddItemToInventory2(inven_item Item);
    virtual bool RemoveItemFromInventory2(inven_item Item, bool bRemoveAll = false);
    virtual void AddAmmoToInventory2(inven_item Item, int Amount);
    virtual void ClearInventory2();
    inventory2&  GetInventory2() { return m_Inventory2; }
    virtual void InitInventory();
    void         ReInitInventory();
    bool         HasItemInInventory2(inven_item Item);
    virtual bool OnPickup(pickup& Pickup);
    virtual bool InTurret() { return false; }
    // Kill/damage functions
    virtual void OnKilledEnemy(guid deadEnemy) { (void)deadEnemy; };
    virtual void OnKilledAlly(guid deadAlly) { (void)deadAlly; };
    virtual void OnDamagedEnemy(guid damagedEnemy) { (void)damagedEnemy; };
    virtual void OnDamagedAlly(guid damagedAlly) { (void)damagedAlly; };
    virtual void OnBeingShotAt(Object::type ProjectileType, guid ShooterID)
    {
        (void)ProjectileType;
        (void)ShooterID;
    }
    virtual void  OnHitFriendly(guid FriendlyID) { (void)FriendlyID; } //do nothing
    virtual void  OnHitByFriendly(guid ShooterID) { (void)ShooterID; }
    virtual bool  OnPlayFullBodyImpactAnim(loco::anim_type AnimType, float BlendTime, uint32_t Flags);
    virtual void  OnDeath();
    virtual void  OnSpawn();
    virtual float GetSpawnFadeTime() { return m_SpawnFadeTime; }
    corpse*       CreateCorpseObject(bool BodyFades = true);
    void          CreateCorpse();
    guid          GetCorpseGuid() { return m_CorpseGuid; }
    bool          IsAlive() const { return (!m_bDead); };
    bool          IsDead() const { return (m_bDead); };
    bool          HasSpawnInvulnerability() const { return (m_SpawnNeutralTime > 0) ? true : false; }
    void          SetSpawnNeutralTime(float Time) { m_SpawnNeutralTime = Time; }

    // Type functions
    virtual bool IsPlayer() { return false; }
    virtual bool IsCharacter() { return false; }
    virtual bool IsNetGhost() { return false; }

    // Tweak stuff
    virtual float ModifyDamageByDifficulty(float Damage);
    virtual float ModifyDamageByTurret(float Damage);

    // Locomotion functions
    virtual void  InitLoco();
    virtual bool  IsRunning();
    bool          IsMoving();
    virtual bool  IsStaggering();
    loco*         GetLocoPointer() { return m_pLoco; }
    virtual bool  UsingLoco() { return true; }
    virtual float GetPitch();
    virtual float GetYaw();
    virtual void  SetPitch(Radian Pitch);
    virtual void  SetYaw(Radian Yaw);

    // Animation functions
    bool HasAnim(loco::anim_type animType);
    bool IsAnimInPackage(const char* pAnimGroup, const char* pName);
    bool IsPlayingFullBodyLipSync();
    bool IsPlayingFullBodyCinema();
    bool IsPlayingLipSync();
    bool IsPlayingCinema();

    // Zone functions
    virtual void Teleport(const Vector3& Position, bool DoBlend = true, bool DoEffect = false);
    virtual void Teleport(const Vector3& Position, Radian Pitch, Radian Yaw, bool DoBlend = true, bool DoEffect = false);
    virtual void InitZoneTracking();
    virtual void UpdateZoneTrack();
    void         UpdateZone(uint8_t Zone);
    // const zone_mgr::tracker& GetZoneTracker() const { return m_ZoneTracker; }

    // Weapon functions
    virtual void AddNewWeapon2(inven_item WeaponItem) {}
    virtual void PickupWeapon2(inven_item WeaponItem) {}
    virtual void SwitchWeapon2(inven_item WeaponItem) {}
    new_weapon*  GetCurrentWeaponPtr();
    new_weapon*  GetWeaponPtr(inven_item WeaponItem);

    virtual void MoveWeapon(bool UpdateWeaponRenderState);
    void         UpdateWeapon(float DeltaTime);
    //bool         HasWeaponEquiped() { return GetCurrentWeaponPtr() != nullptr; }
    virtual bool EquipWeapon2(inven_item WeaponItem);
    virtual void UnequipCurrentWeapon();
    virtual int  GetWeaponRenderState();
    virtual bool IsFlashlightOn();

    // Collision functions
    void          SetCollidedActor(guid collidedActor) { m_CollidedActor = collidedActor; }
    virtual float GetActorCollisionRadius();
    virtual void  Push(const Vector3& PushVector);
    virtual float GetCollisionHeight();
    virtual float GetCollisionRadius();
    void          WakeUpDoors();
    void          ResetRidingPlatforms();

    // Floor functions
    floor_properties& GetFloorProperties() { return m_FloorProperties; }
    Colour            GetFloorColor() { return m_FloorProperties.GetColor(); }
    uint32_t          GetFloorMaterial() { return m_FloorProperties.GetMaterial(); }
    float GetFloorIntensity();

    // AI related functions
    virtual Radian  GetSightYaw() const; // Get direction head is facing
    virtual Vector3 GetPositionWithOffset(eOffsetPos offset);
    virtual float   GetMovementNoiseLevel(); // return a value of how noisy we are (0.0f - 1.0f)
    virtual void    GetHeadAndRootPosition(Vector3& HeadPos, Vector3& RootPos);

    // Activate functions
    bool IsActive() override { return m_bIsActive; }
    void SetIsActive(bool bIsActive);

    // Hit location functions
    virtual Geom::Bone::HitLocation GetHitLocation(const pain& Pain);
    const char*                     GetHitLocationName(Geom::Bone::HitLocation Loc);

    // Animation functions
    virtual void PlayImpactAnim(const pain& Pain, eHitType hitType);
    virtual bool HandleSpecialImpactAnim(const eHitType hitType)
    {
        (void)hitType;
        return false;
    }
    loco::anim_type GetDeathAnim(const pain& painThatKilledUs);

    // Event functions
    virtual void SendAnimEvents();
    virtual bool OnAnimEvent(const anim_event& Event, const Vector3& WorldPos);

    // Pain related functions
    virtual bool TakeDamage(const pain& Pain);

    virtual void     PlayFlinch(const pain& Pain);
    void             PlayPainSfx();
    void             UpdatePainSfx(float DeltaTime);
    virtual eHitType OverrideFlinchType(eHitType hitType) { return hitType; }
    const pain&      GetPainThatKilledUs() { return m_PainThatKilledUs; }
    corpse_pain&     GetCorpseDeathPain() { return m_CorpseDeathPain; }
    eHitType     GetHitType(const pain& Pain);
    virtual void UpdateFellFromAltitude() {}
    virtual void TakeFallPain() {}
    virtual bool IgnorePain(const pain& Pain)
    {
        (void)Pain;
        return false;
    }
    virtual bool IgnoreFlinches() { return false; }
    virtual bool IgnoreFullBodyFlinches() { return false; }
    virtual void BroadcastActorDeath(guid actorKiller) { (void)actorKiller; }

    // cloaking functions
    virtual void UpdateCloak(float DeltaTime);
    virtual void Cloak();
    virtual void Decloak();

    // contagion functions
    virtual bool IsContagious() { return m_bContagious; }
    virtual void InitContagion(int Origin = -1);
    virtual void KillMPContagion();
    virtual void ContagionLogic(float DeltaTime);
    virtual void ContagionDOT();
    void         RenderContagion();

    // Dialog functions
    virtual bool PlayDialog(eDialogType dialogType,
                            const char* dialogName = nullptr,
                            bool        hotVoice = false,
                            const char* animName = nullptr,
                            const char* animPkg = nullptr,
                            uint32_t    Flags = 0,
                            uint8_t     DialogFlags = 0,
                            float       BlendOutTime = 0.25f);

    // Effects functions
    bool         IsBloodEnabled() const;
    virtual void CreateDamageEffects(const pain& Pain, bool bDoLargeEffects = true, bool bDoDebris = true);
    void         CreateSplatDecalOnGround();
    void         CreateSplatDecalOnWall(const pain& rPain);

    // Attach point functions
    void        EnumAttachPoints(std::string& String) const override;
    int         GetAttachPointIDByName(const char* pName) const override;
    std::string GetAttachPointNameByID(int iAttachPt) const override;
    bool        GetAttachPointData(int iAttachPt, Matrix4& L2W, uint32_t Flags = 0) override;
    void        OnAttachedMove(int iAttachPt, const Matrix4& L2W) override;

    // Decal functions
    std::string GetBloodDecalPackage() const;
    int         GetBloodDecalGroup() const;

    // Rendering functions
    const Matrix4*     GetBonesForRender(uint64_t LODMask, int& nActiveBones);
    void               RenderHitLocations();
    float              TimeSinceLastRender();
    virtual skin_inst& GetSkinInst() { return m_SkinInst; }
    AnimGroup::handle& GetAnimGroupHandle();
    virtual void       SetSkinVMesh(bool bMutant);

    virtual bool      SetMutated(bool bMutate);
    virtual void      SetupMutationChange(bool bMutate);
    virtual void      ForceMutationChange(bool bMutate);
    void              PrepPlayerAvatar();
    void              SetCanToggleMutation(bool bCanToggleMutation);
    mutagen_burn_mode GetMutagenBurnMode() const { return m_MutagenBurnMode; }
    void              SetMutagenBurnMode(mutagen_burn_mode MutagenBurnMode);

    void                  SetAvatarMutationState(avatar_mutation_state State);
    avatar_mutation_state GetAvatarMutationState() const { return m_AvatarMutationState; }
    float                 GetTimeLeftInAvatarMutationState() const { return m_TimeLeftInAvatarMutationState; }
    void                  UpdateAvatarMutation(float DeltaTime);

    //    virtual render_inst*        GetRenderInstPtr() { return &m_SkinInst; }
    virtual AnimGroup::handle* GetAnimGroupHandlePtr() { return &m_hAnimGroup; }
    virtual Vector3            GetBonePos(int BoneIndex);

    const char* GetLogicalName() override { return "ACTOR"; }

    actor_effects* GetActorEffects(bool bCreate = false);

    // Flag functions
    bool IsCrouching() const;
    void SetIsCrouching(bool bIsCrouching);

    bool IsAirborn() const;
    void SetIsAirborn(bool bIsAirborn);

    float      GetLeanAmount() const;
    void       SetLeanAmount(float LeanAmount);
    lean_state GetLeanState() const;
    void       SetLeanState(lean_state State);

    void SetVoteCanCast(bool CanCast);
    void SetVoteCanStartKick(uint32_t KickMask);
    void SetVoteCanStartMap(bool CanStartMap);

    bool     GetVoteCanCast() const;
    uint32_t GetVoteCanStartKick() const;
    bool     GetVoteCanStartMap() const;

    bool IsDumbAndFast() const { return m_bDumbAndFast; };

    void           SetWayPoint(int Index, const Vector3& V) { m_WayPoint[Index] = V; };
    const Vector3& GetWayPoint(int Index) { return m_WayPoint[Index]; };

    float GetInactiveTime() const { return m_InactiveTime; };
    void  ClearInactiveTime() { m_InactiveTime = 0.0f; }
    void  UpdateInactive(float DeltaTime);

    //=========================================================================
    // DATA
    //=========================================================================
public:
    // Actor active list
    static actor* m_pFirstActive;
    static int    m_nActive;
    actor*        m_pNextActive;
    actor*        m_pPrevActive;

protected:
    // Active info
    bool m_bIsActive; // true if character is within active area

    // Faction/team info
    factions m_Faction;     // which faction this Object belongs to
    uint32_t m_FriendFlags; // Flags of our friends.

    // Health
    actor_health m_Health;           // Amount of health that the actor has.
    float        m_MaxHealth;        // Actors's maximum health
    bool         m_bDead;            // If true, then you are DEAD!
    bool         m_bCanDie;          // Defaults to true
    death_type   m_DeathType;        // Type of death
    guid         m_CorpseGuid;       // Guid of dead body
    bool         m_bWantToSpawn;     // Network related
    float        m_SpawnFadeTime;    // Time left for spawn fade
    float        m_SpawnNeutralTime; // Time left for neutral spawn behavior (invuln./can't shoot)

    // Zone tracking
    zone_mgr::tracker m_ZoneTracker; // Tracks the zones.

    // Inventory
    inventory2 m_Inventory2; // Inventory
    guid       m_WeaponGuids[INVEN_NUM_WEAPONS];
    bool       m_WeaponsCreated;    // true when weapon objects have been created
    inven_item m_CurrentWeaponItem; // Current weapon item, see inventory2.hpp for mapping

    // Pain
    int   m_LastAnimPainID;
    int   m_LastMeleeEventID;
    int   m_CurrentPainEventID; // the id of our current pain events.
    float m_BigPainTakenTime;
    pain  m_PainThatKilledUs; // info from the pain that killed us.
    float m_TimeSinceLastPain;
    corpse_pain m_CorpseDeathPain; // Last pain applied to corpse
    float m_SafeFallAltitude;
    float m_DeathFallAltitude;
    float m_FellFromAltitude; // where did we start falling?

    // Conversation/audio properties
    ResourceHandle<char> m_hAudioPackage; // Audio resource for this Object.
                                          // voice_id      m_VoiceID;            // The voice id used for dialog, pain grunts etc.
    int   m_PreferedVoiceActor;           // 0-3 the voice that says our dialog a,b,c,d
    float m_PainSfxInterval;              // Time to wait before playing another pain grunt
    float m_PainSfxDelay;                 // Delay time before playing impact sound ( 0 = none queued )

    // cloaking properties
    //  voice_id    m_CloakVoiceID;         // voice id for cloaking transition sound
    float       m_CloakShieldPainTimer; // time just after taking pain where the cloak shield is messed up
    float       m_CloakTransitionTime;  // how long we have been in a transition state (going into or out of cloak)
    cloak_state m_CloakState;           // are we cloaked, uncloaked, or transitioning between the two?

    // Collision
    guid  m_CollidedActor;       // guid of the actor we ran into
    float m_TimeActorColliding;  // time we have spent actor colliding
    float m_TimeObjectColliding; // time we have spent colliding with non-actors

    // Decals
    ResourceHandle<decal_package> m_hBloodDecalPackage; // the blood package
    int                           m_BloodDecalGroup;    // the blood group (within the package)

    // Locomotion
    bool  m_LocoAllocated; // If true, must delete m_pLoco pointer.
    loco* m_pLoco;
    bool  m_bIgnoreLocoInTransform; // annoying bool so we ignore the loco on transform in the editor.
    float m_LastTimeStaggered;

    // Rendering
    floor_properties   m_FloorProperties;
    skin_inst         m_SkinInst;            // Render instance
    AnimGroup::handle m_hAnimGroup;          // Animation group handle
    float             m_TimeSinceLastRender; // Last time character was rendered
    float             m_LeanAmount;          // -1.0f to 1.0f indicates leaning all the way left
                                             //       to all the way right, respectively
    lean_state m_LeanState;                  // Current lean state

    // Mutation vision glow properties
    Colour m_FriendlyGlowColor;
    Colour m_EnemyGlowColor;
    float  m_PulseGlowDelta;   // rate of pulse based character state
    float  m_CurrentGlowPulse; // glow can pulse based on character state
    bool   m_bAllowedToGlow;   // is this guy allowed to glow?

    // Contagion timers
    float m_ContagionTimer;    // Time left as contagious.
    float m_ContagionDOTTimer; // Time to next DOT.

    // Flags
    uint32_t m_CanCloak : 1,
        m_MustCloak : 1,
        m_bIsCrouching : 1, // Is the actor crouching
        m_bIsAirborn : 1,   // Is the in the air
        m_VoteCanCast : 1,
        m_VoteCanStartMap : 1,
        m_bIsMutated : 1, // Are we mutated?
        m_bCanToggleMutation : 1,
        m_bDumbAndFast : 1,
        m_bPrimaryFired : 1,   // Flags primary has been fired (used in online)
        m_bEndPrimaryFire : 1, // Used to stop primary fire (used in online)
        m_bContagious : 1;

    int m_FireState;

    uint32_t m_VoteCanStartKick;
    int      m_VoteAction;   // 1=Cast 2=CallKick 3=CallMap
    int      m_VoteArgument; // Payload for VoteAction

    Radian m_Pitch; // Head Logical orientation. Used for movement
    Radian m_Yaw;   // Head Logical orientation. Used for movement

    actor_effects* m_pEffects;

    actor::mutagen_burn_mode     m_MutagenBurnMode; // what percentage of mutagen we burn every seconde
    actor::avatar_mutation_state m_AvatarMutationState;
    float                        m_TimeLeftInAvatarMutationState;

    // Aiming stuff.
    int     m_TargetNetSlot;
    Vector3 m_AimOffset;

    uint32_t m_WayPointFlags;
    int      m_WayPointTimeOut;
    Vector3  m_WayPoint[2];

    bool m_bLockedDoors;

    //--------------------------------------------------------------------------
    //  Inactivity tracking
    //--------------------------------------------------------------------------

    float   m_InactiveTime;
    Vector3 m_RecentPosition;

    friend class save_mgr;
    friend class check_point_mgr;
    friend class state_mgr;
};

inline Vector3 actor::GetVelocity() const
{
    // Use loco physics?
    //if (m_pLoco) {
    //    return m_pLoco->m_Physics.GetVelocity();
    // }

    // Just use base class
    return Object::GetVelocity();
}

inline float actor::GetHealth()
{
    return m_Health.GetHealth();
}

//=========================================================================

inline void actor::ResetHealth(float Health)
{
    return m_Health.Reset(Health);
}

//=========================================================================

inline float actor::GetMaxHealth()
{
    return m_MaxHealth;
}

//=========================================================================

inline void actor::SetMaxHealth(float MaxHealth)
{
    m_MaxHealth = MaxHealth;
}

//=========================================================================

inline float actor::GetParametricHealth()
{
    return (GetHealth() / m_MaxHealth);
}

//=========================================================================

inline bool actor::DoAliveLogic()
{
    bool bNormalLogic = ((m_Health.GetHealth() > 0.f) || (!m_bCanDie));
    return bNormalLogic;
}

//===========================================================================
// FACTION FUNCTIONS
//===========================================================================

inline factions actor::GetFaction() const
{
    assert(m_Faction != FACTION_NOT_SET);

    // Special case when contagious...
    if (m_bContagious) {
        return FACTION_DEATHMATCH;
    } else {
        return m_Faction;
    }
}

//===========================================================================

inline void actor::SetFaction(factions NewFaction)
{
    m_Faction = NewFaction;
}

//===========================================================================

inline void actor::SetFriendFlags(uint32_t NewFriends)
{
    m_FriendFlags = NewFriends | FACTION_NEUTRAL;
    m_FriendFlags = m_FriendFlags & !FACTION_DEATHMATCH;
}

//===========================================================================

inline uint32_t actor::GetFriendFlags() const
{
    return m_FriendFlags;
}

//=========================================================================

inline bool actor::IsFriendlyFaction(factions Faction) const
{
    if (Faction == FACTION_NOT_SET) {
        return false;
    }

    // deathmatch special case
    if (Faction == FACTION_DEATHMATCH ||
        GetFaction() == FACTION_DEATHMATCH) {
        return false;
    }

    return ((m_FriendFlags & Faction) ||
            (Faction == FACTION_NEUTRAL) ||
            (GetFaction() == FACTION_NEUTRAL));
}

//=========================================================================

inline bool actor::IsEnemyFaction(factions Faction) const
{
    if (Faction == FACTION_NOT_SET) {
        return false;
    }

    if (((Faction == FACTION_TEAM_ONE) && (GetFaction() == FACTION_TEAM_ONE)) ||
        ((Faction == FACTION_TEAM_TWO) && (GetFaction() == FACTION_TEAM_TWO))) {
        return false;
    }

    // deathmatch special case
    if (Faction == FACTION_DEATHMATCH ||
        GetFaction() == FACTION_DEATHMATCH) {
        return true;
    }

    return (((~m_FriendFlags) & Faction) &&
            (Faction != FACTION_NEUTRAL) &&
            (GetFaction() != FACTION_NEUTRAL));
}

//===========================================================================

inline std::string actor::GetBloodDecalPackage() const
{
    return m_hBloodDecalPackage.getName();
}

//===========================================================================

inline int actor::GetBloodDecalGroup() const
{
    return m_BloodDecalGroup;
}

//===========================================================================

inline AnimGroup::handle& actor::GetAnimGroupHandle()
{
    return m_hAnimGroup;
}

//=========================================================================

inline float actor::TimeSinceLastRender()
{
    return m_TimeSinceLastRender;
}

//===========================================================================
// Dialog functions

inline bool actor::PlayDialog(eDialogType dialogType,
                              const char* dialogName,
                              bool        hotVoice,
                              const char* animName,
                              const char* animPkg,
                              uint32_t    Flags,
                              uint8_t     DialogFlags,
                              float       BlendOutTime)
{
    return false;
}

//===========================================================================
// Flag functions

inline bool actor::IsCrouching() const
{
    return (m_bIsCrouching != 0);
}

//===========================================================================

inline void actor::SetIsCrouching(bool bIsCrouching)
{
    // State change?
    if (IsCrouching() != bIsCrouching) {
        // Record new state.
        m_bIsCrouching = bIsCrouching;
    }
}

//===========================================================================

inline bool actor::IsAirborn() const
{
    return (m_bIsAirborn != 0);
}

//===========================================================================

inline void actor::SetIsAirborn(bool bIsAirborn)
{
    // State change?
    if (IsAirborn() != bIsAirborn) {
        // Record new state.
        m_bIsAirborn = bIsAirborn;
    }
}

//===========================================================================

inline float actor::GetLeanAmount() const
{
    return m_LeanAmount;
}

//===========================================================================

inline void actor::SetLeanAmount(float LeanAmount)
{
    // Update?
    if (m_LeanAmount != LeanAmount) {
        m_LeanAmount = LeanAmount;
    }
}

//===========================================================================

inline actor::lean_state actor::GetLeanState() const
{
    return m_LeanState;
}

//===========================================================================

inline void actor::SetLeanState(lean_state State)
{
    // State change?
    if (m_LeanState != State) {
        // Record new state.
        m_LeanState = State;
    }
}

//===========================================================================

inline void actor::SetVoteCanCast(bool CanCast)
{
    m_VoteCanCast = CanCast;
}

//===========================================================================

inline bool actor::GetVoteCanCast() const
{
    return (m_VoteCanCast);
}

//===========================================================================

inline void actor::SetVoteCanStartKick(uint32_t KickMask)
{
    m_VoteCanStartKick = KickMask;
}

//===========================================================================

inline uint32_t actor::GetVoteCanStartKick() const
{
    return (m_VoteCanStartKick);
}

//===========================================================================

inline void actor::SetVoteCanStartMap(bool CanStartMap)
{
    m_VoteCanStartMap = CanStartMap;
}

//===========================================================================

inline bool actor::GetVoteCanStartMap() const
{
    return (m_VoteCanStartMap);
}
