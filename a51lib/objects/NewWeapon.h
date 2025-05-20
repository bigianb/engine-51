#pragma once

#include "../objectManager/ObjectManager.h"
#include "render/SkinInst.h"
//#include "AudioMgr\AudioMgr.hpp"
#include "../characters/factions.h"
#include "../inventory/Inventory.h"
#include "render/RigidInst.h"
#include "../fx/fx_Mgr.h"
#include "../animation/CharAnimPlayer.h"

class bullet_projectile;
class base_projectile;
class HudObject;
class player;
class pain_handle;

//=========================================================================

// Animation controller that positions the left/right weapons on each hand
// (since both weapons are part of the same geometry, it has to be done at
//  the compute matrices level in the animation player - this controller
//  adjust the keys of the correct weapon bones to achieve this)
class dual_weapon_anim_controller : public track_controller
{

public:
    dual_weapon_anim_controller(ObjectManager* om);
    virtual ~dual_weapon_anim_controller() {}

    // Initializes
    void Init(guid                     WeaponGuid,
              const AnimGroup::handle& hAnimGroup,
              const char*              pLeftBone,
              const char*              pRightBone);

    // Sets location of animation data package
    virtual void SetAnimGroup(const AnimGroup::handle& hGroup);

    // Clears the animation to a safe unused state
    virtual void Clear() {}

    // Advances the current track by logic time
    virtual void Advance(float DeltaTime) { (void)DeltaTime; }

    // Controls the influence this anim has during the mixing process
    virtual void  SetWeight(float Weight) { (void)Weight; }
    virtual float GetWeight() { return 1.0f; }

    // Returns the raw keyframe data
    virtual void GetInterpKeys(AnimKey* pKey) { (void)pKey; }
    virtual void GetInterpKey(int iBone, AnimKey& Key)
    {
        (void)iBone;
        (void)Key;
    }

    // Mixes the anims keyframes into the dest keyframes
    virtual void MixKeys(AnimKey* pDestKey);

    // Removes yaw from root node of turn animations
    virtual void SetRemoveTurnYaw(bool bRemove) { (void)bRemove; }

    // Data
protected:
    ObjectManager*    objectManager;
    AnimGroup::handle m_hAnimGroup;   // Group of anims we are using
    int               m_iLWeaponBone; // Index of left weapon bone
    int               m_iRWeaponBone; // Index of right weapon bone
    guid              m_WeaponGuid;   // Guid of weapon owner
};

//=========================================================================
class new_weapon : public Object
{
public:
    //weapons can either be rendered by the player or if an NPC is holding them.
    enum render_state
    {
        RENDER_STATE_UNDEFINED = -1,
        RENDER_STATE_PLAYER,
        RENDER_STATE_NPC,
        RENDER_STATE_MAX
    };

    //slots for the weapon's ammo to be held
    enum ammo_priority
    {
        AMMO_PRIMARY = 0,
        AMMO_SECONDARY,
        AMMO_MAX
    };

    //type of projectile that this weapon uses for ammunition.
    enum projectile_type
    {
        PROJECTILE_UNDEFINED = -1, //Not a valid value.
        PROJECTILE_NONE,           //Used for MagLight and LAB Scanner

        BULLET_MUTATION,
        BULLET_MACHINE_GUN,

        // KSS -- TO ADD NEW WEAPON - if weapon has ammo
        BULLET_SHOTGUN,

        BULLET_SNIPER_RIFLE,
        BULLET_GAUSS_RIFLE,
        BULLET_PISTOL,
        BULLET_RIFT_BOT,
        BULLET_MHG,
        BULLET_MSN,
        BULLET_MSN_SECONDARY,

        GRENADE_FRAG,
        GRENADE_FLASH,
        GRENADE_HAND_NUKE,

        MINE_GRAVITATIONAL_CHARGE,

        PROJECTILE_MAX
    };

    enum weapon_events
    {
        EVENT_NULL,

        EVENT_FIRE,
        EVENT_ALT_FIRE,
        EVENT_GRENADE,
        EVENT_ALT_GRENADE,

        EVENT_FIRE_LEFT,
        EVENT_ALT_FIRE_LEFT,
        EVENT_FIRE_RIGHT,
        EVENT_ALT_FIRE_RIGHT,
        EVENT_CINEMA_FIRE,
    };

    enum fire_points
    {
        FIRE_POINT_DEFAULT,
        FIRE_POINT_LEFT,
        FIRE_POINT_RIGHT,

        FIRE_POINT_COUNT
    };

    enum zoom_state
    {
        NO_ZOOM_WEAPON_STATE,
        WEAPON_STATE_ZOOM_IN,
        WEAPON_STATE_ZOOM_OUT,
    };

    struct reticle_radius_parameters
    {
        float m_MaxRadius;              // in screen pixels, how big the reticle can be
        float m_MinRadius;              // in screen pixels, how small the reticle can be
        float m_CrouchBonus;            // in screen pixels, how much the radius increases over m_MaxRadius when crouching
        float m_MaxMovementPenalty;     // worst penalty, in screen pixels, for actor movement
        float m_MoveShrinkAccel;        // acceleration rate for shrinking the reticle from movement
        float m_ShotShrinkAccel;        // acceleration rate for shrinking the reticle from shot penalties
        float m_GrowAccel;              // acceleration rate for growing the reticle
        float m_PenaltyForShot;         // penalty in screen pixels for a single shot
        float m_ShotPenaltyDegradeRate; // how fast the shot penalty degrades, in pixels/second
    };

    CREATE_RTTI(new_weapon, Object, Object)

    new_weapon(ObjectManager* pObjectManager);

    virtual ~new_weapon();

    //=========================================================================
    //
    // FireWeapon           -   Fires a projectile from the weapon.  Public interface for player.
    // FireSecondary        -   Secondary fire for the weapon
    // IsWeaponReady        -   Returns true if the weapon is ready to fire.  False otherwise.
    // CanReload            -   Returns true if the weapon can be reloaded.
    // Reload               -   Reloads the weapon (Updates ammo counts)
    // GetCurrentAnimGroup  -   Returns the current AnimGroup (reference) to the caller.
    // SetAnimation         -   Sets the animation on the weapon.  This will only be used by the player.
    // SetRotation          -   Sets the Rotation of the weapon in world space.
    // ProcessSfx           -   Handles sound related animation events.
    // GetLocalBBox         -   Bounding box information for the weapon.
    // GetMaterial          -   Legacy code.  Returns material type.
    // OnEnumProp           -   Enumerates all the properties related to the player
    // OnProperty           -   Set/Gets properties for the player
    // OnAdvanceLogic       -   Called once per loop.  Advances animation player and makes the calls to handle animation events.
    // OnMove               -   Updates objects position in the world.
    // OnColCheck           -   Overload.  Does nothing (weapons do not currently collide with other objects.)
    // OnColNotify          -   Overload.  Does nothing (weapons do not currently collide with other objects.)
    // OnRender             -   Render routine for the weapons.
    // GetRenderState       -   Tells if the weapon is being rendered by the player or an NPC.
    // GetAmmoCount         -   Returns the ammo amount for the object.
    // GetSecondaryAmmoCount-   Returns the ammo amount for the object.
    // InitWeapon           -   Initializes the weapon's animations, bind pose and position if the weapon is being created on the fly.
    // IsInited             -   Tells if the weapon has been initialized.
    // GetTypeDesc          -   Returns the type descriptor of the object.
    // GetObjectType        -   Returns the type of the object.
    //
    //=========================================================================

    void OnEnumProp(prop_enum& list) override;
    bool OnProperty(prop_query& rPropQuery) override;

    //These fire weapon functions are dispatch functions...
    void SetTarget(guid TargetGuid) { m_TargetGuid = TargetGuid; }
    bool FireWeapon(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint);
    bool FireSecondary(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint);

    bool NPCFireWeapon(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier = 1.0f, bool isHit = true);
    void NPCFireSecondary(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier = 1.0f, bool isHit = true);

    virtual bool FireGhostPrimary(int iFirePoint, bool bUseFireAt, Vector3& FireAt);
    virtual bool FireGhostSecondary(int iFirePoint);

    bool         IsUsingSplitScreen(); // for multiplayer splitscreen
    virtual bool IsWeaponReady(const ammo_priority& rAmmoPriority);
    virtual bool CanReload(const ammo_priority& Priority);
    virtual void Reload(const ammo_priority& Priority);
    virtual void SetupDualAmmo(inven_item OtherWeaponItem);
    virtual bool CanFire(bool bIsAltFire);

    char_anim_player& GetCurrentAnimPlayer();
    const AnimGroup&  GetCurrentAnimGroup();
    bool              HasAnimGroup();
    virtual void      SetAnimation(const int& nAnimIndex, const float& fBlendTime, const bool& bResetFrames = false);
    virtual float     GetFrameParametric();
    virtual void      SetFrameParametric(float Frame);

    Vector3      GetVelocity() { return m_Velocity; }
    Radian       GetAngularSpeed() { return m_AngularSpeed; }
    Radian       GetYaw() { return (m_FiringPointBoneIndex[FIRE_POINT_DEFAULT] >= 0) ? m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_FiringPointBoneIndex[FIRE_POINT_DEFAULT]).GetRotation().yaw : 0; }
    void         SetRotation(const float& rPitch, const float& rYaw);
    virtual void ProcessSfx();

    virtual void BeginPrimaryFire();
    virtual void EndPrimaryFire();
    virtual void BeginAltFire();
    virtual void EndAltFire();
    virtual void ReleaseAudio();

    virtual void BeginAltRampUp() {}
    virtual void EndAltRampUp(bool bGoingIntoHold, bool bSwitchingWeapon)
    {
        (void)bGoingIntoHold;
        (void)bSwitchingWeapon;
    }
    virtual void EndAltHold(bool bSwitchingWeapon) { (void)bSwitchingWeapon; }
    virtual void BeginIdle(bool bNormalIdle = true) { (void)bNormalIdle; }
    virtual void EndIdle() {}
    virtual bool CanSwitchIdleAnim() { return true; }

    virtual void BeginSwitchFrom() {}
    virtual void BeginSwitchTo() {}
    virtual void EndSwitchTo() {}

    //mandatory overloads
    BBox GetLocalBBox() const override;
    int  GetMaterial() const override;

    virtual void ResetWeapon();

    void OnAdvanceLogic(float DeltaTime) override;
    void OnMove(const Vector3& NewPos) override;
    void OnTransform(const Matrix4& L2W) override;

    void OnColCheck() override;
    void OnKill() override;

    void DegradeAim(Radian3& Rot, Radian Amt, const Vector3& InitPos, guid Owner);

    void         OnRender() override;
    void         OnRenderTransparent() override;
    virtual void RenderWeapon(bool bDebug, const Colour& Ambient, bool Cloaked);
    void         SetVisible(bool bVisible) { m_IsVisible = bVisible; }

    void GetAmmoState(ammo_priority Priority,
                      int&          AmmoAmount,
                      int&          AmmoMax,
                      int&          AmmoPerClip,
                      int&          AmmoInCurrentClip);

    void SetAmmoState(ammo_priority Priority,
                      int           AmmoAmount,
                      int           AmmoMax,
                      int           AmmoPerClip,
                      int           AmmoInCurrentClip);

    void SetAmmoState2(ammo_priority Priority,
                       int           AmmoAmount,
                       int           AmmoInCurrentClip);

    static inven_item GetDualWeaponID(inven_item ParentItem);
    static inven_item GetParentIDForDualWeapon(inven_item DualItem);

    render_state GetRenderState();

    // IJB virtual void        DrawLaserFixupBitmap    ( Bitmap* pBitmap, float Radius, Colour cColor, collision_mgr::collision& Coll );

    bool IsReloadCompleted() { return m_bCompletedReload; }
    void SetReloadCompleted(bool bCompleted) { m_bCompletedReload = bCompleted; }

    void RefillClip(ammo_priority Priority);
    int  GetAmmoCount();
    int  GetAmmoCount(ammo_priority Priority);
    int  GetSecondaryAmmoCount();

    int GetAmmoPerClip();
    int GetAmmoPerClip(ammo_priority Priority);
    int GetSecondaryAmmoPerClip();

    int GetAmmoAmount(ammo_priority Priority);

    int         GetTotalPrimaryAmmo();
    virtual int GetTotalSecondaryAmmo();

    int GetMaxPrimaryAmmo();
    int GetMaxSecondaryAmmo();

    void         ClearAmmo(const ammo_priority& rAmmoPriority = AMMO_PRIMARY);
    void         ClearClipAmmo(const ammo_priority& rAmmoPriority = AMMO_PRIMARY);
    void         DecrementAmmo(const ammo_priority& rAmmoPriority = AMMO_PRIMARY, const int& nAmt = 1);
    virtual void FillAmmo();
    virtual void NotifyAmmoFull(player* pPlayer);

    guid  GetParentGuid() override { return m_ParentGuid; }
    float GetAccuracyPercent(float distance);

    virtual bool GetFiringStartPosition(Vector3& Pos);

    void AddAmmoToWeapon(int nAmmoPrimary, int nAmmoSecondary);

    virtual void InitWeapon(const char* pSkinFileName, const char* pAnimFileName, const Vector3& rInitPos, const render_state& rRenderState = RENDER_STATE_PLAYER, const guid& rParentGuid = 0);
    virtual void InitWeapon(const Vector3& rInitPos, render_state rRenderState, guid OwnerGuid);

    void InitNPCDualAnimController(dual_weapon_anim_controller* pController,
                                   const char*                  pLeftRootBone,
                                   const char*                  pRightRootBone);

    inline bool IsInited() { return m_WeaponInited; }

    const object_desc&        GetTypeDesc() const override;
    static const object_desc& GetObjectType();

    bool HasSecondaryAmmo() { return m_HasSecondaryAmmo; }

    virtual bool GetFiringBonePosition(Vector3& Pos, int iBone = FIRE_POINT_DEFAULT);
    virtual bool GetAimBonePosition(Vector3& Pos, int iBone = FIRE_POINT_DEFAULT);

    bool CheckFirePoint();
    bool CheckFlashlightPoint();

    Bitmap*               GetCenterReticleBmp();
    Bitmap*               GetEdgeReticleBmp();
    float                 GetCenterPixelOffset();
    virtual ammo_priority GetPrimaryAmmoPriority() { return AMMO_PRIMARY; }
    virtual ammo_priority GetSecondaryAmmoPriority() { return AMMO_PRIMARY; }

    virtual bool CanFastTapFire() { return false; }
    virtual bool CanIntereptPrimaryFire(int nFireAnimIndex);
    virtual bool CanIntereptSecondaryFire(int nFireAnimIndex);

    virtual bool CanAltChainFire() { return true; }

    virtual bool ContinueReload();

    virtual void       SetupRenderInformation();
    bool               GetIdleMode() { return m_bIdleMode; }
    static const char* GetWeaponPrefixFromInvType2(inven_item WeaponItem);

    virtual void SetRenderState(render_state RenderState);

    reticle_radius_parameters GetReticleRadiusParameters() const { return m_ReticleRadiusParameters; }
    reticle_radius_parameters GetAltReticleRadiusParameters() const { return m_AltReticleRadiusParameters; }
    virtual bool              ShouldDrawReticle() { return true; }

    virtual bool CheckReticleLocked();
    virtual void UpdateReticle(float DeltaTime);
    virtual bool ShouldUpdateReticle() { return false; }

    uint8_t GetAutoSwitchRating() const { return m_AutoSwitchRating; }

    virtual void UpdateAmmoWarning();
    virtual void SetAmmoHudColor(player* pPlayer, HudObject* Hud, Colour HudColor);

    render_inst*       GetRenderInstPtr() override;
    AnimGroup::handle* GetAnimGroupHandlePtr() override;
    Geom*              GetGeomPtr() override;

    //=========================================================================
    // ZOOM STATS
    //=========================================================================

    bool         IsZoomEnabled();
    void         ZoomInComplete(bool bZoomInComplete) { m_bZoomComplete = bZoomInComplete; }
    bool         IsZoomInComplete() { return m_bZoomComplete; }
    Radian       GetXFOV();
    float        GetZoomLevel();
    int          GetZoomStep();
    int          GetnZoomSteps() { return m_nZoomSteps; }
    virtual int  IncrementZoom();
    virtual void ClearZoom();
    float        GetZoomMovementMod();

    //=========================================================================
    // FLASHLIGHT STUFF
    //=========================================================================
    virtual bool HasFlashlight() { return true; }
    virtual bool GetFlashlightTransformInfo(Matrix4& incMatrix, Vector3& incVect);

    //=========================================================================
    // CUSTOM SNIPER SCOPE
    //=========================================================================
    static void CreateScopeTexture();

    //this is a structure that contains ammunition information
    struct ammo
    {
        ammo();

        void OnEnumProp(prop_enum& List);
        bool OnProperty(prop_query& rPropQuery);

        int m_AmmoAmount;        //Amount of ammo.
        int m_AmmoMax;           //Maximum amount of ammo.
        int m_AmmoPerClip;       //Amount of ammo per clip
        int m_AmmoInCurrentClip; //Amount of ammo in current clip

        int                    m_ProjectileTemplateID; // Index to the g_TemplateDictionary for this weapons bullet.
        ResourceHandle<Bitmap> m_Bitmap;

        projectile_type m_ProjectileType;
    };

protected:
    // functions for the custom sniper scope texture
    Bitmap* GetScopeTexture();
    void    InstallCustomScope();
    void    UninstallCustomScope();

    //The protected weapon fire functions are the real overloaded funcitons...
    virtual bool FireWeaponProtected(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint);
    virtual bool FireSecondaryProtected(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint);

    virtual bool FireNPCWeaponProtected(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit);
    virtual bool FireNPCSecondaryProtected(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit);

    virtual void MoveMuzzleFx();
    virtual void RenderMuzzleFx();
    virtual void AdvanceMuzzleFX(float DeltaTime);

    //Overload this to define the type of muzzle flash for a particular gun, can be NULL
public:
    virtual void InitMuzzleFx(bool bIsPrimary, int iFirePoint);
    void         KillAllMuzzleFX();

protected:
    base_projectile* CreateBullet(const char*               pWeaponLogicalName,
                                  const Vector3&            InitPos,
                                  const Radian3&            InitRot,
                                  const Vector3&            InheritedVelocity,
                                  const guid                OwnerGuid,
                                  const pain_handle&        PainHandle,
                                  new_weapon::ammo_priority Priority = AMMO_PRIMARY,
                                  bool                      bHitLiving = true,
                                  int                       iFirePoint = FIRE_POINT_DEFAULT);

    //=========================================================================
    // Functions & Data from inventory_item
    //=========================================================================

public:
    inline inven_item  GetInvenItem() { return m_Item; }
    inline rigid_inst& GetRigidInst() { return m_Inst; }
    inline void        SetOwnerGuid(guid Guid)
    {
        m_OwnerGuid = Guid;
        m_ParentGuid = Guid;
    }
    //            uint8_t                  GetFlags            ()        { return m_Flags; }
protected:
    rigid_inst m_Inst;      // Render Instance for the item.
    guid       m_OwnerGuid; // Who is carrying this inventory item.
    uint8_t    m_Flags;     // Any special flags associated with the inventory item.
    inven_item m_Item;      // Inventory item corresponding to this weapon

    //=========================================================================
    //=========================================================================

public:
    ammo m_WeaponAmmo[AMMO_MAX]; // Actual ammo information
protected:
    ammo                 m_OriginalWeaponAmmo[AMMO_MAX]; // Actual ammo information
    render_state         m_CurrentRenderState;
    skin_inst            m_Skin[RENDER_STATE_MAX];       // Geometry use to render the weapon
    AnimGroup::handle    m_AnimGroup[RENDER_STATE_MAX];  // Animation file for the weapon
    ResourceHandle<char> m_hAudioPackage;                // Audio package for the weapon.
    char_anim_player     m_AnimPlayer[RENDER_STATE_MAX]; // Animation player used for weapon animations.
    bool                 m_WeaponInited;
    int                  m_FlashlightBoneIndex;                       // The bone index of the flashlight point
    int                  m_FiringPointBoneIndex[FIRE_POINT_COUNT];    // Bone index on the weapon of the firing point.
    int                  m_AltFiringPointBoneIndex[FIRE_POINT_COUNT]; // Alt firing point for the weapon.
    int                  m_AimPointBoneIndex[FIRE_POINT_COUNT];
    int                  m_AltAimPointBoneIndex[FIRE_POINT_COUNT];
    Vector3              m_LastPosition;
    Vector3              m_Velocity;
    Radian               m_LastRotation; // just tracking yaw here
    Radian               m_AngularSpeed; // just tracking yaw here

    bool                 m_EnableMuzzleFx;                      // Flag to enable muzzle effects..
    ResourceHandle<char> m_hMuzzleFXPrimary;                    // Primary muzzle effect for the weapon.
    ResourceHandle<char> m_hMuzzleFXSecondary;                  // Secondary muzzle effect for the weapon.
    fx_handle            m_MuzzleNPCFX[FIRE_POINT_COUNT];       // Unique ID for NPC/Avatar muzzle flash
    fx_handle            m_MuzzleFX[FIRE_POINT_COUNT];          // Unique ID for muzzle flash
    fx_handle            m_MuzzleSecondaryFX[FIRE_POINT_COUNT]; // Unique ID for the secondary muzzle flash
    const char*          m_NPCMuzzleSoundFx;                    // Name of sound effect for muzzle flash for NPC

    ResourceHandle<Bitmap> m_ReticleCenter;
    ResourceHandle<Bitmap> m_ReticleEdge;
    float                  m_ReticleCenterPixelOffset;

    float m_AimDegradePrimary;   // how much to degrade aim for each primary fire
    float m_AimDegradeSecondary; // how much to degrade aim for each secondary fire
    float m_AimRecoverSpeed;     // how quickly to recover from fire

    reticle_radius_parameters m_ReticleRadiusParameters;
    reticle_radius_parameters m_AltReticleRadiusParameters;

    float m_ShortRange;               // max distance considered short rnage for this weapon
    float m_LongRange;                // min distance considered long range for this weapon
    int   m_AccuracyPercentLongRange; // percent of short ranges accuracy at long range

    bool       m_IsVisible; // Is the weapon visible (used for thrown grenades)
    bool       m_HasSecondaryAmmo;
    guid       m_ParentGuid; // guid to the owner of this weapon..
    bool       m_bIdleMode;
    zoom_state m_ZoomState;
    Radian     m_CurrentViewX; // Current x-view
    Radian     m_CurrentViewY; // Current y-view
    float      m_ZoomMovementSpeed;
    int        m_FactionFireSfx[MAX_FACTION_COUNT];
    int        m_ZoomStep;
    int        m_nZoomSteps;
    bool       m_bZoomComplete;
    guid       m_TargetGuid;

    int m_ScopeMesh;     // sniper-scope custom mesh (-1 if not present)
    int m_ScopeLensMesh; // sniper-scope custom lens mesh (-1 if not present)

    static int m_OrigScopeVramId;    // original vram id of scope texture
    static int m_ScopeRefCount;      // reference count to scope texture
    static int m_ScopeTextureVramId; // custom texture pulled from the screen

    bool    m_bLockedOn;         // so we can tell a sound to play
    bool    m_bCompletedReload;  // are we past the required reload threshold?
    bool    m_bCanWarnLowAmmo;   // flag so we don't flood the player with low ammo warnings.
    float   m_fLastAmmoFullTime; // what was the last time we told the player that they were full of a particular ammo
    uint8_t m_AutoSwitchRating;  // what is the auto-switch rating of this weapon?  Larger number = better weapon
};

//===========================================================================

inline int new_weapon::GetTotalPrimaryAmmo()
{
    return m_WeaponAmmo[AMMO_PRIMARY].m_AmmoAmount;
}

//===========================================================================

inline int new_weapon::GetTotalSecondaryAmmo()
{
    return m_WeaponAmmo[AMMO_SECONDARY].m_AmmoAmount;
}

//===========================================================================

inline int new_weapon::GetMaxPrimaryAmmo()
{
    return m_WeaponAmmo[AMMO_PRIMARY].m_AmmoMax;
}

//===========================================================================

inline int new_weapon::GetMaxSecondaryAmmo()
{
    return m_WeaponAmmo[AMMO_SECONDARY].m_AmmoMax;
}

//===========================================================================

inline float new_weapon::GetCenterPixelOffset()
{
    return m_ReticleCenterPixelOffset;
}

//==============================================================================

inline new_weapon::render_state new_weapon::GetRenderState()
{
    return m_CurrentRenderState;
}
