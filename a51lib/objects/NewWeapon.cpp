
#include "../objectManager/ObjectManager.h"
#include "../objectManager/ObjectPtr.h"
#include "NewWeapon.h"
#include "../render/SkinGeom.h"
#include "../xfiles/xfs.h"

#include "../tweakManager/TweakMgr.h"

//#include "Entropy\e_Draw.hpp"
#include "Player.h"
// #include "ProjectileBullett.hpp"
// #include "..\MiscUtils\SimpleUtils.hpp"
// #include "objects\GrenadeProjectile.hpp"
// #include "Dictionary\global_dictionary.hpp"
// #include "TemplateMgr\TemplateMgr.hpp"
#include "../EventMgr.h"
// #include "Dictionary\Global_Dictionary.hpp"
// #include "render\LightMgr.hpp"
// #include "GameLib\RenderContext.hpp"
// #include "hud_Player.hpp"
#include "HudObject.h"
// #include "hud_Ammo.hpp"
// #include "OccluderMgr\OccluderMgr.hpp"
// #include "Characters\character.hpp"

int new_weapon::m_OrigScopeVramId = -1;
int new_weapon::m_ScopeRefCount = 0;
int new_weapon::m_ScopeTextureVramId = -1;

// IJB extern bool s_bDegradeAim;
bool s_bDegradeAim = false;

//=========================================================================
// CONSTS
//=========================================================================

static const int kScopeTextureW = 64;
static const int kScopeTextureH = 64;

static const float k_npc_frag_scaling_const = 2.5f;
static Vector3     s_ZeroVec(0.0f, 0.0f, 0.0f);

// message timing tweaks
tweak_handle Ammo_Full_Msg_FadeTimeTweak("Ammo_Full_Msg_FadeTime");
tweak_handle Ammo_Full_Msg_DelayTimeTweak("Ammo_Full_Msg_DelayTime");

class new_weapon_desc : public object_desc
{
public:
    new_weapon_desc()
        : object_desc(Object::TYPE_WEAPON,
                      "Weapon Item",
                      "WEAPON",
                      Object::ATTR_SPACIAL_ENTRY |
                          Object::ATTR_NEEDS_LOGIC_TIME |
                          Object::ATTR_SOUND_SOURCE |
                          Object::ATTR_RENDERABLE,
                      FLAGS_IS_DYNAMIC)
    {
    }

    Object* Create(ObjectManager* om, collision_mgr* cm)
    {
        return new new_weapon(om);
    }
};

static new_weapon_desc s_NewWeaponDesc;

//==============================================================================

const object_desc& new_weapon::GetTypeDesc() const
{
    return s_NewWeaponDesc;
}

//==============================================================================

const object_desc& new_weapon::GetObjectType(void)
{
    return s_NewWeaponDesc;
}

//==============================================================================
// WEAPON_DUAL_SMP_CONTROLLER
//==============================================================================

dual_weapon_anim_controller::dual_weapon_anim_controller(ObjectManager* om)
    : m_iLWeaponBone(-1)
    , m_iRWeaponBone(-1)
    , m_WeaponGuid(0)
    , objectManager(om)
{
}

//==============================================================================

// Initializes
void dual_weapon_anim_controller::Init(guid                     WeaponGuid,
                                       const AnimGroup::handle& hAnimGroup,
                                       const char*              pLeftBone,
                                       const char*              pRightBone)
{
    // Keep weapon guid
    m_WeaponGuid = WeaponGuid;

    // Switch to new anim group
    SetAnimGroup(hAnimGroup);

    // Init bone indices
    const AnimGroup* pAnimGroup = m_hAnimGroup.getPointer();
    if (pAnimGroup) {
        // Lookup bones
        m_iLWeaponBone = pAnimGroup->GetBoneIndex(pLeftBone);
        m_iRWeaponBone = pAnimGroup->GetBoneIndex(pRightBone);

        // Make sure they are present
        assert(m_iLWeaponBone != -1); //, xfs("%s bone is missing from NPC dual .skingeom!", pLeftBone));
        assert(m_iRWeaponBone != -1); //, xfs("%s bone is missing from NPC dual .skingeom!", pRightBone));
    }
}

//==============================================================================

// Sets location of animation data package
void dual_weapon_anim_controller::SetAnimGroup(const AnimGroup::handle& hGroup)
{
    // Keep group
    m_hAnimGroup = hGroup;
}

//==============================================================================

void dual_weapon_anim_controller::MixKeys(AnimKey* pDestKey)
{
    // Lookup owning weapon
    object_ptr<new_weapon> pWeapon(m_WeaponGuid, objectManager);
    if (!pWeapon) {
        return;
    }

    // Lookup owning actor
    object_ptr<actor> pActor(pWeapon->GetParentGuid(), objectManager);
    if (!pActor) {
        return;
    }

    // Lookup actor loco
    loco* pLoco = pActor->GetLocoPointer();
    if (!pLoco) {
        return;
    }

    // Compute inverse root bone L2W
    Matrix4 InvRootL2W;
    InvRootL2W.Setup(Vector3(1, 1, 1),
                     pDestKey[0].rotation,
                     pDestKey[0].translation);
    InvRootL2W.InvertRT();

    // Overwrite left weapon bone (all child bones will follow since they are relative)
    if (m_iLWeaponBone != -1) {
        // Get world space weapon transform and make relative to parent bone
        Matrix4 L2W = InvRootL2W * pLoco->GetWeaponL2W(1);
        pDestKey[m_iLWeaponBone].translation = L2W.GetTranslation();
        pDestKey[m_iLWeaponBone].rotation = L2W.GetRotation();
    }

    // Overwrite right weapon bone (all child bones will follow since they are relative)
    if (m_iRWeaponBone != -1) {
        // Get world space weapon transform and make relative to parent bone
        Matrix4 L2W = InvRootL2W * pLoco->GetWeaponL2W(0);
        pDestKey[m_iRWeaponBone].translation = L2W.GetTranslation();
        pDestKey[m_iRWeaponBone].rotation = L2W.GetRotation();
    }
}

new_weapon::ammo::ammo()
    : m_AmmoAmount(1)
    , m_AmmoMax(1)
    , m_AmmoPerClip(1)
    , m_ProjectileTemplateID(-1)
    , m_ProjectileType(new_weapon::PROJECTILE_UNDEFINED)
{
}

//==============================================================================

new_weapon::new_weapon(ObjectManager* om)
    : Object(om)
    , m_OwnerGuid(0)
    , m_Flags(~0)
    , m_Item(INVEN_NULL)
    ,

    m_CurrentRenderState(new_weapon::RENDER_STATE_NPC)
    , m_EnableMuzzleFx(true)
    , m_ReticleCenterPixelOffset(8.0f)
    , m_AimDegradePrimary(0.0f)
    , m_AimDegradeSecondary(0.0f)
    , m_AimRecoverSpeed(1.0f)
    , m_ShortRange(500.0f)
    , m_LongRange(2000.0f)
    , m_AccuracyPercentLongRange(25)
    , m_IsVisible(true)
    , m_HasSecondaryAmmo(false)
    , m_ParentGuid(NULL_GUID)
    , m_bIdleMode(false)
    , m_ZoomState(NO_ZOOM_WEAPON_STATE)
    , m_CurrentViewX(R_0)
    , m_CurrentViewY(R_0)
    , m_ZoomMovementSpeed(1.0f)
    , m_ZoomStep(0)
    , m_nZoomSteps(0)
    , m_TargetGuid(NULL_GUID)
    , m_ScopeMesh(-1)
    , m_ScopeLensMesh(-1)
    , m_bLockedOn(false)
    , m_bCanWarnLowAmmo(true)
    , m_fLastAmmoFullTime(0.0f)
    ,                     // put this at 0 and not current time so the message will hit the first time
    m_AutoSwitchRating(0) // what is the auto-switch rating of this weapon?  Larger number = better weapon
{
    int i;
    for (i = 0; i < FIRE_POINT_COUNT; i++) {
        m_FiringPointBoneIndex[i] = -1;
        m_AltFiringPointBoneIndex[i] = -1;
        m_AimPointBoneIndex[i] = -1;
        m_AltAimPointBoneIndex[i] = -1;
    }

    for (i = 0; i < MAX_FACTION_COUNT; i++) {
        m_FactionFireSfx[i] = -1;
    }

    // initialize flashlight bone index
    m_FlashlightBoneIndex = -1;

    m_ReticleRadiusParameters.m_MaxRadius = 20.0f;
    m_ReticleRadiusParameters.m_MinRadius = 6.5f;
    m_ReticleRadiusParameters.m_CrouchBonus = 5.0f;
    m_ReticleRadiusParameters.m_MaxMovementPenalty = 8.0f;
    m_ReticleRadiusParameters.m_MoveShrinkAccel = 75.0f;
    m_ReticleRadiusParameters.m_ShotShrinkAccel = 75.0f;
    m_ReticleRadiusParameters.m_GrowAccel = 1000.0f;
    m_ReticleRadiusParameters.m_PenaltyForShot = 4.0f;
    m_ReticleRadiusParameters.m_ShotPenaltyDegradeRate = 17.0f;

    m_AltReticleRadiusParameters.m_MaxRadius = 20.0f;
    m_AltReticleRadiusParameters.m_MinRadius = 6.5f;
    m_AltReticleRadiusParameters.m_CrouchBonus = 5.0f;
    m_AltReticleRadiusParameters.m_MaxMovementPenalty = 8.0f;
    m_AltReticleRadiusParameters.m_MoveShrinkAccel = 75.0f;
    m_AltReticleRadiusParameters.m_ShotShrinkAccel = 75.0f;
    m_AltReticleRadiusParameters.m_GrowAccel = 1000.0f;
    m_AltReticleRadiusParameters.m_PenaltyForShot = 4.0f;
    m_AltReticleRadiusParameters.m_ShotPenaltyDegradeRate = 17.0f;

    m_LastPosition.Zero();
    m_Velocity.Zero();
    m_LastRotation = 0;
    m_AngularSpeed = 0;
}

//==============================================================================

new_weapon::~new_weapon()
{
    KillAllMuzzleFX();
    EndPrimaryFire();
}

//===========================================================================

void new_weapon::InitWeapon(const Vector3& rInitPos, render_state rRenderState, guid OwnerGuid)
{
    //set the render state
    m_CurrentRenderState = rRenderState;

    //set the owner of this weapon
    m_OwnerGuid = m_ParentGuid = OwnerGuid;

    // Get zones of owner
    {
        Object* pOwner = objectManager->GetObjectByGuid(OwnerGuid);
        if (pOwner) {
            SetZone1(pOwner->GetZone1());
            SetZone2(pOwner->GetZone2());
        }
    }

    //change the attribs flags...
    switch (m_CurrentRenderState) {
    case RENDER_STATE_PLAYER:
    {
        //remove the object from dBase
        objectManager->RemoveFromSpatialDBase(GetGuid());

        //set appropriate attribute bits as needed
        SetAttrBits(Object::ATTR_NULL);
    } break;
    case RENDER_STATE_NPC:
    {
        //remove the object from dBase
        objectManager->RemoveFromSpatialDBase(GetGuid());

        //set appropriate attribute bits as needed
        SetAttrBits(Object::ATTR_SOUND_SOURCE);
    } break;
    default:
        //no-op
        break;
    }

    //initialize the animations for this animation group.
    if (m_AnimGroup[m_CurrentRenderState].getPointer()) {
        m_AnimPlayer[m_CurrentRenderState].SetAnimGroup(m_AnimGroup[m_CurrentRenderState]);
        m_AnimPlayer[m_CurrentRenderState].SetAnim(0, true, true);

        m_FiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint");
        m_AltFiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint");
        m_AimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint");
        m_AltAimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint");

        m_FiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_left");
        m_AltFiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_left");
        m_AimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_left");
        m_AltAimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_left");

        m_FiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_right");
        m_AltFiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_right");
        m_AimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_right");
        m_AltAimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_right");

        m_WeaponInited = true;
    } else {
        m_WeaponInited = false;
    }

    //set initial position of the object.
    OnMove(rInitPos);

    m_ZoomStep = 0;
}

//==============================================================================

void new_weapon::InitWeapon(const char* pSkinFileName, const char* pAnimFileName, const Vector3& rInitPos,
                            const render_state& rRenderState, const guid& rParentGuid)
{
    //set the render state
    m_CurrentRenderState = rRenderState;

    //set the owner of this weapon
    m_OwnerGuid = m_ParentGuid = rParentGuid;

    // Get zones of owner
    {
        Object* pOwner = objectManager->GetObjectByGuid(rParentGuid);
        if (pOwner) {
            SetZone1(pOwner->GetZone1());
            SetZone2(pOwner->GetZone2());
        }
    }

    //change the attribs flags...
    switch (m_CurrentRenderState) {
    case RENDER_STATE_PLAYER:
    {
        //remove the object from dBase
        objectManager->RemoveFromSpatialDBase(GetGuid());

        //set appropriate attribute bits as needed
        SetAttrBits(Object::ATTR_NULL);
    } break;
    case RENDER_STATE_NPC:
    {
        //remove the object from dBase
        objectManager->RemoveFromSpatialDBase(GetGuid());

        //set appropriate attribute bits as needed
        SetAttrBits(Object::ATTR_SOUND_SOURCE |
                    Object::ATTR_RENDERABLE);
    } break;
    default:
        //no-op
        break;
    }

    //don't re-initialize the animation group ever.
    if (!m_AnimGroup[m_CurrentRenderState].getPointer()) {
        // Initialize the skin
        // IJB m_Skin[m_CurrentRenderState].OnProperty(g_PropQuery.WQueryExternal("RenderInst\\File", pSkinFileName));

        //set up the animation groups.
        m_AnimGroup[m_CurrentRenderState].setName(pAnimFileName);

        //initialize the animations for this animation group.
        if (m_AnimGroup[m_CurrentRenderState].getPointer()) {
            m_AnimPlayer[m_CurrentRenderState].SetAnimGroup(m_AnimGroup[m_CurrentRenderState]);
            m_AnimPlayer[m_CurrentRenderState].SetAnim(0, true, true);

            m_FiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint");
            m_AltFiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint");
            m_AimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint");
            m_AltAimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint");

            m_FiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_left");
            m_AltFiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_left");
            m_AimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_left");
            m_AltAimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_left");

            m_FiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_right");
            m_AltFiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_right");
            m_AimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_right");
            m_AltAimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_right");

            m_WeaponInited = true;
        } else {
            m_WeaponInited = false;
        }
    }

    //set initial position of the object.
    OnMove(rInitPos);

    m_ZoomStep = 0;
}

//==============================================================================

void new_weapon::InitNPCDualAnimController(dual_weapon_anim_controller* pController,
                                           const char*                  pLeftRootBone,
                                           const char*                  pRightRootBone)
{
    // Init the controller
    pController->Init(GetGuid(),                                           // WeaponGuid
                      m_AnimPlayer[RENDER_STATE_NPC].GetAnimGroupHandle(), // hAnimGroup
                      pLeftRootBone,                                       // pLeftBone
                      pRightRootBone);                                     // pRightBone

    // Add to animation player
    m_AnimPlayer[RENDER_STATE_NPC].SetTrackController(1, pController);
}

//==============================================================================

bool new_weapon::IsUsingSplitScreen(void)
{
    return (false);
}

//==============================================================================

bool new_weapon::IsWeaponReady(const ammo_priority& rAmmoPriority)
{
    return (m_WeaponAmmo[rAmmoPriority].m_AmmoInCurrentClip > 0);
}

//==============================================================================

int new_weapon::GetAmmoCount(void)
{
    return m_WeaponAmmo[AMMO_PRIMARY].m_AmmoInCurrentClip;
}

//==============================================================================

int new_weapon::GetSecondaryAmmoCount(void)
{
    return m_WeaponAmmo[AMMO_SECONDARY].m_AmmoInCurrentClip;
}

//==============================================================================

int new_weapon::GetAmmoPerClip(void)
{
    return m_WeaponAmmo[AMMO_PRIMARY].m_AmmoPerClip;
}

//==============================================================================

int new_weapon::GetSecondaryAmmoPerClip(void)
{
    return m_WeaponAmmo[AMMO_SECONDARY].m_AmmoPerClip;
}

//==============================================================================

int new_weapon::GetAmmoCount(ammo_priority Priority)
{
    return m_WeaponAmmo[Priority].m_AmmoInCurrentClip;
}

//==============================================================================

int new_weapon::GetAmmoPerClip(ammo_priority Priority)
{
    return m_WeaponAmmo[Priority].m_AmmoPerClip;
}

//==============================================================================

int new_weapon::GetAmmoAmount(ammo_priority Priority)
{
    return m_WeaponAmmo[Priority].m_AmmoAmount;
}

//==============================================================================

const char* new_weapon::GetWeaponPrefixFromInvType2(inven_item WeaponItem)
{
    switch (WeaponItem) {
    case INVEN_WEAPON_DESERT_EAGLE:
        return "EGL";
        break;
    case INVEN_GRENADE_FRAG:
        return "FRG";
        break;
    case INVEN_GRENADE_GRAV:
        return "GRV";
        break;
    case INVEN_WEAPON_MESON_CANNON:
        return "MSN";
        break;

    case INVEN_WEAPON_BBG:
        return "BBG";
        break;

    case INVEN_WEAPON_TRA:
        return "TRA";
        break;

        // KSS -- TO ADD NEW WEAPON
    case INVEN_WEAPON_SHOTGUN:
        return "SHT";
        break;
    case INVEN_WEAPON_SMP:
        return "SMP";
        break;
    case INVEN_WEAPON_SCANNER:
        return "SCN";
        break;
    case INVEN_WEAPON_SNIPER_RIFLE:
        return "SNI";
        break;
    case INVEN_WEAPON_MUTATION:
        return "MUT";
        break;
    default:
        break;
    }

    return nullptr;
}

//==============================================================================

float new_weapon::GetAccuracyPercent(float distanceToTarget)
{
    if (distanceToTarget <= m_ShortRange) {
        return 1.0f;
    } else if (distanceToTarget >= m_LongRange) {
        return ((float)m_AccuracyPercentLongRange) / 100.0f;
    } else {
        // we are somewhere in between....
        float shortToLong = m_LongRange - m_ShortRange;
        float distToShort = distanceToTarget - m_ShortRange;
        float percentOfRange = distToShort / shortToLong;
        float finalAccuracy = 1.0f - (1.0f - (((float)m_AccuracyPercentLongRange) / 100.0f)) * percentOfRange;
        return finalAccuracy;
    }
}

//==============================================================================
bool new_weapon::GetFiringStartPosition(Vector3& Pos)
{
    return false;
}

//==============================================================================

bool new_weapon::CanReload(const ammo_priority& Priority)
{
    return m_WeaponAmmo[Priority].m_AmmoAmount > m_WeaponAmmo[Priority].m_AmmoInCurrentClip && m_WeaponAmmo[Priority].m_AmmoInCurrentClip < m_WeaponAmmo[Priority].m_AmmoPerClip;
}

//==============================================================================

bool new_weapon::CanFire(bool bIsAltFire)
{
    int AmmoCount = 0;

    ammo_priority AmmoPriority = bIsAltFire
                                     ? GetSecondaryAmmoPriority()
                                     : GetPrimaryAmmoPriority();
    AmmoCount = GetAmmoCount(AmmoPriority);

    return (AmmoCount > 0);
}

//==============================================================================

void new_weapon::Reload(const ammo_priority& Priority)
{
    // For GameDay, we do not want Zach to run out of ammo.  Remove the next line after GameDay.
    //m_WeaponAmmo[Priority].m_AmmoAmount = m_WeaponAmmo[Priority].m_AmmoMax;
    int nNewAmmoCount = std::min(m_WeaponAmmo[Priority].m_AmmoAmount, m_WeaponAmmo[Priority].m_AmmoPerClip);

    m_WeaponAmmo[Priority].m_AmmoInCurrentClip = nNewAmmoCount;
}

//=========================================================================

void new_weapon::SetupDualAmmo(inven_item OtherWeaponItem)
{
    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

    // get our parent's pointer
    if (!pObj) {
        return;
    }

    actor* pParent = (actor*)pObj;

    // other weapon is the "parent" weapon, i.e. INVEN_WEAPON_SMP is the parent of INVEN_WEAPON_DUAL_SMP
    new_weapon* pOtherWeapon = pParent->GetWeaponPtr(OtherWeaponItem);

    // don't have it... get out.
    if (!pOtherWeapon) {
        return;
    }

    // get other weapon's ammo.
    int nCurrentAmmoCount = pOtherWeapon->GetTotalPrimaryAmmo();
    int nAmmoBonus = (m_WeaponAmmo[AMMO_PRIMARY].m_AmmoPerClip - pOtherWeapon->GetAmmoPerClip());
    int nAmmoMissing = (pOtherWeapon->GetAmmoPerClip() - pOtherWeapon->GetAmmoCount());

    // make sure our counts "match up".  We want this weapon to look like it just gave us X rounds for free
    m_WeaponAmmo[AMMO_PRIMARY].m_AmmoMax = pOtherWeapon->GetMaxPrimaryAmmo() + nAmmoBonus;

    // give the new "dual" weapon the ammo from the "parent" weapon.
    // We have to do this the weird way because we are adding an absolute amount and there may be ammo missing.
    m_WeaponAmmo[AMMO_PRIMARY].m_AmmoAmount = nCurrentAmmoCount + nAmmoBonus + nAmmoMissing;
    m_WeaponAmmo[AMMO_PRIMARY].m_AmmoInCurrentClip = m_WeaponAmmo[AMMO_PRIMARY].m_AmmoPerClip;
}

//=========================================================================

inven_item new_weapon::GetDualWeaponID(inven_item ParentItem)
{
    // get the dual item id for this parent object
    if (ParentItem == INVEN_WEAPON_SMP) {
        return INVEN_WEAPON_DUAL_SMP;
    } else if (ParentItem == INVEN_WEAPON_SHOTGUN) {
        return INVEN_WEAPON_DUAL_SHT;
    } else if (ParentItem == INVEN_WEAPON_DESERT_EAGLE) {
        //return INVEN_WEAPON_DUAL_EAGLE;
    }

    return INVEN_NULL;
}

//=========================================================================

inven_item new_weapon::GetParentIDForDualWeapon(inven_item DualItem)
{
    // get the dual item id for this parent object
    if (DualItem == INVEN_WEAPON_DUAL_SMP) {
        return INVEN_WEAPON_SMP;
    } else if (DualItem == INVEN_WEAPON_DUAL_SHT) {
        return INVEN_WEAPON_SHOTGUN;
    } else if (DualItem == INVEN_WEAPON_DUAL_EAGLE) {
        //return INVEN_WEAPON_DESERT_EAGLE;
    }

    return INVEN_NULL;
}

//=========================================================================

bool new_weapon::FireWeapon(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint)
{
    bool Rval = FireWeaponProtected(InitPos, BaseVelocity, Power, InitRot, Owner, iFirePoint);

    if (Rval == true) {
        // Start the muzzle FX
        InitMuzzleFx(true, iFirePoint);

        //if this is the player, degrade aim
        if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
            object_ptr<player> PlayerObj(m_ParentGuid, objectManager);

            if (PlayerObj.IsValid()) {
                PlayerObj.m_pObject->DegradeAim(m_AimDegradePrimary);
                PlayerObj.m_pObject->SetAimRecoverSpeed(m_AimRecoverSpeed);
            }
        }
    }

    m_TargetGuid = NULL_GUID;

    return Rval;
}

//=========================================================================

bool new_weapon::FireSecondary(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint)
{
    bool Rval = FireSecondaryProtected(InitPos, BaseVelocity, Power, InitRot, Owner, iFirePoint);

    if (Rval == true) {
        // Start the muzzle FX
        InitMuzzleFx(false, iFirePoint);

        //if this is the player, degrade aim
        if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
            object_ptr<player> PlayerObj(m_ParentGuid, objectManager);

            if (PlayerObj.IsValid()) {
                PlayerObj.m_pObject->DegradeAim(m_AimDegradeSecondary);
                PlayerObj.m_pObject->SetAimRecoverSpeed(m_AimRecoverSpeed);
            }
        }
    }

    return Rval;
}

//==============================================================================

bool new_weapon::NPCFireWeapon(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit)
{
    // If the weapon has no ammo left, do nothing.
    if (m_WeaponAmmo[AMMO_PRIMARY].m_AmmoInCurrentClip <= 0) {
        return false;
    }

    bool Rval = FireNPCWeaponProtected(BaseVelocity, Target, Owner, fDegradeMultiplier, isHit);

    if (Rval == true) {
        // Start the muzzle FX
        InitMuzzleFx(true, FIRE_POINT_DEFAULT);

        // decrement count of bullets in current clip
        DecrementAmmo();
    }

    return Rval;
}

//==============================================================================

void new_weapon::NPCFireSecondary(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit)
{
    bool Rval = FireNPCSecondaryProtected(BaseVelocity, Target, Owner, fDegradeMultiplier, isHit);

    if (Rval) {
        // Start the muzzle FX
        InitMuzzleFx(false, FIRE_POINT_DEFAULT);
    }
}

//==============================================================================

bool new_weapon::FireGhostPrimary(int iFirePoint, bool bUseFireAt, Vector3& FireAt)
{
    assert(iFirePoint >= 0);
    assert(iFirePoint < FIRE_POINT_COUNT);

    // Ran out of ammo?
    if (m_WeaponAmmo[AMMO_PRIMARY].m_ProjectileTemplateID < 0) {
        return false;
    }

    // Start the muzzle FX
    InitMuzzleFx(true, iFirePoint);

    // No fire point?
    if (m_FiringPointBoneIndex[iFirePoint] == -1) {
        return false;
    }

    // Compute position and direction of bullet to create
    Matrix4 Mat = m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_FiringPointBoneIndex[iFirePoint]);
    Vector3 InitPos = m_AnimPlayer[m_CurrentRenderState].GetBonePosition(m_FiringPointBoneIndex[iFirePoint]);

    // THIS IS BAD, THANK YOU std::max EXPORTER.
    Mat.PreRotateY(R_180);
    Radian3 InitRot = Mat.GetRotation();

    if (bUseFireAt) {
        Vector3 AimVector = FireAt - InitPos;
        AimVector.GetPitchYaw(InitRot.pitch, InitRot.yaw);
    }

    // Create bullet
    pain_handle PainHandle(xfs("PLAYER_%s", GetLogicalName()));

    if (s_bDegradeAim) {
        Vector3 Dir(0, 0, 1);
        Dir.RotateX(R_3 * x_frand(-1.0f, 1.0f)); // Pitch Z-axis up or down by spread angle
        Dir.RotateZ(x_frand(0, R_360));          // Roll dir around Z
        Dir.RotateX(InitRot.pitch);              // Orient around original direction
        Dir.RotateY(InitRot.yaw);

        float P, Y;
        Dir.GetPitchYaw(P, Y);

        InitRot = Radian3(P, Y, 0);

        // Reset so next bullet doesn't try to degrade its aim.
        s_bDegradeAim = false;
    }

    CreateBullet(GetLogicalName(), InitPos, InitRot, s_ZeroVec, m_ParentGuid, PainHandle, AMMO_PRIMARY, true, iFirePoint);

    // add a muzzle light where the bullet was fired from (will fade out really quickly)
    // IJB g_LightMgr.AddFadingLight( InitPos, Colour( 76, 76, 38 ), 200.0f, 3.0f, 0.1f );

    // Play the sound associated with this actor's faction.
    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);
    actor&  OwnerActor = actor::GetSafeType(*pObj);

    factions Faction = OwnerActor.GetFaction();
    /* IJB
    int BitIndex = factions_manager::GetFactionBitIndex( Faction );

    if( m_FactionFireSfx[ BitIndex ] != -1 )
    {
        // Do not generate bullet sounds for the SMP since it's built into the loop
        if( ( m_Item != INVEN_WEAPON_SMP ) && ( m_Item != INVEN_WEAPON_DUAL_SMP ) )
        {
            voice_id VoiceID = g_AudioMgr.Play( g_StringMgr.GetString( m_FactionFireSfx[ BitIndex ] ),
                                                GetPosition(), GetZone1(), true );
            g_AudioManager.NewAudioAlert( VoiceID,
                                        audio_manager::GUN_SHOT,
                                        GetPosition(),
                                        GetZone1(),
                                        GetGuid() );
        }
    }
        */
    return true;
}

//==============================================================================

bool new_weapon::FireGhostSecondary(int iFirePoint)
{
    assert(iFirePoint >= 0);
    assert(iFirePoint < FIRE_POINT_COUNT);

    Vector3 Dummy(0.0f, 0.0f, 0.0f);
    bool    RetVal = FireGhostPrimary(iFirePoint, false, Dummy);

    return RetVal;
}

//==============================================================================

bool new_weapon::FireNPCWeaponProtected(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit)
{
    //derived classes should overload this to fire the approriate bullets..

    return false;
}

//==============================================================================

bool new_weapon::FireNPCSecondaryProtected(const Vector3& BaseVelocity, const Vector3& Target, const guid& Owner, float fDegradeMultiplier, const bool isHit)
{
    //derived classes should overload this to fire the approriate bullets..

    return false;
}

//==============================================================================

bool new_weapon::FireWeaponProtected(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint)
{
    //derived classes should overload this to fire the approriate bullets..

    return false;
}

//==============================================================================

bool new_weapon::FireSecondaryProtected(const Vector3& InitPos, const Vector3& BaseVelocity, const float& Power, const Radian3& InitRot, const guid& Owner, int iFirePoint)
{
    //derived classes should overload this to fire the approriate bullets..

    return false;
}

//==============================================================================

float new_weapon::GetFrameParametric(void)
{
    return m_AnimPlayer[m_CurrentRenderState].GetFrameParametric();
}

//==============================================================================

void new_weapon::SetFrameParametric(float Frame)
{
    m_AnimPlayer[m_CurrentRenderState].SetFrameParametric(Frame);
}

//==============================================================================

void new_weapon::OnRender(void)
{
    // If no one owns this weapon, render it.
    if (m_OwnerGuid == NULL_GUID && m_CurrentRenderState == RENDER_STATE_NPC) {
        /* IJB
        if (g_OccluderMgr.IsBBoxOccluded(GetBBox())) {
            return;
        }
*/
        //get the current anim group that needs to be rendered.
        AnimGroup::handle& CurAnimGroup = m_AnimGroup[m_CurrentRenderState];

        if (CurAnimGroup.getPointer() && m_Skin[m_CurrentRenderState].GetSkinGeom()) {
            uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;

            int            nBones = m_AnimPlayer[m_CurrentRenderState].GetNBones();
            Matrix4*       pBone = (Matrix4*)malloc(nBones * sizeof(Matrix4));
            const Matrix4* pAnimBone = m_AnimPlayer[m_CurrentRenderState].GetBoneL2Ws();

            for (int i = 0; i < nBones; i++) {
                pBone[i] = pAnimBone[i];
            }

            skin_inst& SkinInst = m_Skin[m_CurrentRenderState];
            SkinInst.Render(&GetL2W(),
                            pBone,
                            nBones,
                            Flags | GetRenderMode(),
                            SkinInst.GetLODMask(GetL2W()));
            free(pBone);
        } 
    }
}

//===========================================================================
bool g_DrawWeaponFireBone = false;
void new_weapon::OnRenderTransparent(void)
{
    // default behavior
    Object::OnRenderTransparent();

    // render all muzzle flashes
    RenderMuzzleFx();
}

//==============================================================================

void new_weapon::RenderWeapon(bool bDebug, const Colour& Ambient, bool Cloaked)
{
    if (!m_IsVisible) {
        return;
    }

    /* IJB
    if (g_OccluderMgr.IsBBoxOccluded(GetBBox())) {
        return;
    }

    //get the current anim group that needs to be rendered.
    AnimGroup::handle& CurAnimGroup = m_AnimGroup[m_CurrentRenderState];

    if (CurAnimGroup.getPointer() && m_Skin[m_CurrentRenderState].GetSkinGeom()) {
        // if we are owned by a player, then we need to ask for his offset
        Object* pOwner = objectManager->GetObjectByGuid(m_OwnerGuid);
        Vector3 Offset(0.0f, 0.0f, 0.0f);
        if (pOwner && pOwner->IsKindOf(player::GetRTTI())) {
            Offset = ((player*)pOwner)->GetCurrentWeaponCollisionOffset();
        }

        // accumulate clipping flags and whether or not we should glow
        uint32_t Flags;
        if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
            Flags = render::CLIPPED | render::DISABLE_SPOTLIGHT;
            if (g_RenderContext.m_bIsMutated && !g_RenderContext.m_bIsPipRender) {
                Flags |= render::GLOWING;
            }
        } else {
            const AnimGroup* pAnimGroup = CurAnimGroup.getPointer();
            BBox             RenderBBox = pAnimGroup->GetBBox();
            RenderBBox.Transform(GetL2W());
            RenderBBox.Translate(Offset);

            // Perform clipping test, and skip render if outside the view
            int InView = objectManager->IsBoxInView(RenderBBox, 0b111111);
            if (InView == -1) {
                return;
            }

            // "InView" contains the # of planes straddled, so if it's not zero, clipping is needed
            Flags = (InView != 0) ? render::CLIPPED : 0;
        }

        int            nBones = m_AnimPlayer[m_CurrentRenderState].GetNBones();
        Matrix4*       pBone = (Matrix4*)smem_BufferAlloc(nBones * sizeof(Matrix4));
        const Matrix4* pAnimBone = m_AnimPlayer[m_CurrentRenderState].GetBoneL2Ws();

        // if our owner is spawning, we need ALPHA for fading
        if (pOwner && pOwner->IsKindOf(actor::GetRTTI()) && (((actor*)pOwner)->GetSpawnFadeTime() > 0.0f)) {
            Flags |= render::FADING_ALPHA;
        }

        for (int i = 0; i < nBones; i++) {
            pBone[i] = pAnimBone[i];
            pBone[i].Translate(Offset);
        }

        skin_inst& SkinInst = m_Skin[m_CurrentRenderState];

        if (Cloaked) {
            SkinInst.RenderDistortion(&GetL2W(),
                                      pBone,
                                      nBones,
                                      Flags,
                                      SkinInst.GetLODMask(GetL2W()),
                                      Radian3(R_0, R_0, R_0),
                                      Ambient);
        } else {
            uint64_t MeshMask = SkinInst.GetLODMask(GetL2W());

            // render the scope mesh if one is there
            if ((m_ScopeMesh != -1) &&
                (MeshMask & (1 << m_ScopeMesh))) {
                uint32_t ScopeMask = (1 << m_ScopeMesh);
                if (m_ScopeLensMesh != -1) {
                    ScopeMask |= (1 << m_ScopeLensMesh);
                }

                // render the scope mesh
                MeshMask &= ~ScopeMask;
                SkinInst.Render(&GetL2W(),
                                pBone,
                                nBones,
                                Flags | render::FORCE_LAST,
                                ScopeMask,
                                Ambient);
            }

            // render the normal mesh
            SkinInst.Render(&GetL2W(),
                            pBone,
                            nBones,
                            Flags,
                            MeshMask,
                            Ambient);
        }
    }
        */
}

//==============================================================================
float s_LowAmmoPercent = 0.3f;
float s_LowAmmoMsgFadeTime = 2.5f;
void  new_weapon::OnAdvanceLogic(float DeltaTime)
{
    const Vector3 Position = GetPosition();
    m_Velocity = (Position - m_LastPosition) / DeltaTime;
    m_LastPosition = Position;

    const Radian Rot = GetYaw();
    m_AngularSpeed = (Rot - m_LastRotation) / DeltaTime;
    m_LastRotation = Rot;

    Vector3 Pos;

    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        if (m_AnimGroup[m_CurrentRenderState].getPointer()) {
            //x_printfxy(0,4, "%s", m_AnimPlayer[m_CurrentRenderState].GetAnimInfo().getName() );
            m_AnimPlayer[m_CurrentRenderState].Advance(DeltaTime, Pos);
        }

        // do we play locked-on sound
        UpdateReticle(DeltaTime);
    }

    bool    bHandleSuperEvents = true;
    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

    if (pObj && pObj->IsKindOf(player::GetRTTI())) {
        player* pPlayer = (player*)pObj;

        // if this is a player and we're running a cinema, don't advance or handle logic
        if (pPlayer && (pPlayer->IsCinemaRunning())) {
            bHandleSuperEvents = false;
            KillAllMuzzleFX();
        }
    }

    // should we handle super events?
    if (bHandleSuperEvents) {
        g_EventMgr.HandleSuperEvents(m_AnimPlayer[m_CurrentRenderState], this);
    }

    // make sure we advance the particle logic for muzzle flashes
    AdvanceMuzzleFX(DeltaTime);
}

//==============================================================================

void new_weapon::NotifyAmmoFull(player* pPlayer)
{

    float currentTime = (float)x_GetTimeSec();

    // make sure we don't flood the player with info.
    if ((currentTime - m_fLastAmmoFullTime) > Ammo_Full_Msg_DelayTimeTweak.GetF32()) {
        // tell the player their ammo is full
        // IJB MsgMgr.Message(MSG_FULL_AMMO, pPlayer->net_GetSlot(), m_Item);

        // reset time
        m_fLastAmmoFullTime = currentTime;
    }
}

//==============================================================================

void new_weapon::DegradeAim(Radian3& Rot, Radian Amt, const Vector3& InitPos, guid Owner)
{
    // Shut off degradeaim.  Characters handle hit/miss on their own. -AndyT
}

//==============================================================================

void new_weapon::OnColCheck(void)
{
    if (m_OwnerGuid == NULL_GUID) {
        g_CollisionMgr.StartApply(GetGuid());
        g_CollisionMgr.ApplyAABBox(GetBBox());
        g_CollisionMgr.EndApply();
    }
}

//==============================================================================
int new_weapon::GetMaterial() const
{
    return 0;
}

//==============================================================================

BBox new_weapon::GetLocalBBox() const
{
    Geom* pGeom = m_Skin[m_CurrentRenderState].GetGeom();

    if (pGeom) {
        return pGeom->bbox;
    } else {
        return BBox(Vector3(-50, -50, -50), Vector3(50, 50, 50));
    }
}

//==============================================================================

char_anim_player& new_weapon::GetCurrentAnimPlayer(void)
{
    return m_AnimPlayer[m_CurrentRenderState];
}

//==============================================================================

const AnimGroup& new_weapon::GetCurrentAnimGroup(void)
{
    return m_AnimPlayer[m_CurrentRenderState].GetAnimGroup();
}

//==============================================================================

bool new_weapon::HasAnimGroup(void)
{
    // Get the current anim group that needs to be rendered.
    AnimGroup::handle& CurAnimGroup = m_AnimGroup[m_CurrentRenderState];

    if (CurAnimGroup.getPointer() && m_Skin[m_CurrentRenderState].GetSkinGeom()) {
        return true;
    }

    return false;
}

//==============================================================================

void new_weapon::OnMove(const Vector3& NewPos)
{
    Object::OnMove(NewPos);

    if (m_AnimGroup[m_CurrentRenderState].getPointer()) {
        m_AnimPlayer[m_CurrentRenderState].SetPosition(NewPos);
    }

    // for other weapons that need to do stuff
    MoveMuzzleFx();
}

//==============================================================================

void new_weapon::OnTransform(const Matrix4& L2W)
{
    Object::OnTransform(L2W);

    // for other weapons that need to do stuff
    MoveMuzzleFx();

    if (m_AnimGroup[m_CurrentRenderState].getPointer()) {
        m_AnimPlayer[m_CurrentRenderState].SetRotationAndPosition(L2W);
    }
}

//==============================================================================

void new_weapon::RenderMuzzleFx(void)
{
    // Do the players muzzle render logic...
    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        // if we are owned by a player, then we need to ask for his offset
        Object* pOwner = objectManager->GetObjectByGuid(m_OwnerGuid);
        Vector3 Offset(0.0f, 0.0f, 0.0f);
        if (pOwner && pOwner->IsKindOf(player::GetRTTI())) {
            Offset = ((player*)pOwner)->GetCurrentWeaponCollisionOffset();
        }

        for (int i = 0; i < FIRE_POINT_COUNT; i++) {
            if ((m_FiringPointBoneIndex[i] != -1)) {
                if (m_MuzzleFX[i].Validate()) {
                    Vector3 Pos(m_MuzzleFX[i].GetTranslation());
                    m_MuzzleFX[i].SetTranslation(Pos + Offset);
                    m_MuzzleFX[i].Render();
                    m_MuzzleFX[i].SetTranslation(Pos);
                }
            }

            if ((m_AltFiringPointBoneIndex[i] != -1)) {
                m_MuzzleSecondaryFX[i].Render();
            }
        }
    }

    // Do the NPC muzzle render logic..
    // Don't need || IsUsingSplitScreen() because player logic handles the render state properly in this case.
    if (m_CurrentRenderState == RENDER_STATE_NPC) {
        // render it
        for (int i = 0; i < FIRE_POINT_COUNT; i++) {
            if (m_FiringPointBoneIndex[i] != -1) {
                m_MuzzleNPCFX[i].Render();
            }
        }
    }
}

//==============================================================================

void new_weapon::AdvanceMuzzleFX(float DeltaTime)
{
    // Do the players muzzle render logic...
    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        for (int i = 0; i < FIRE_POINT_COUNT; i++) {
            if (m_MuzzleFX[i].Validate()) {
                // if it finished, get rid of it
                if (m_MuzzleFX[i].IsFinished()) {
                    m_MuzzleFX[i].KillInstance();
                } else if ((m_FiringPointBoneIndex[i] != -1)) {
                    // transform the effect
                    Matrix4 L2W = m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_FiringPointBoneIndex[i]);
                    L2W.PreTranslate(m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_FiringPointBoneIndex[i]));
                    m_MuzzleFX[i].SetTransform(L2W);

                    // the run the logic
                    m_MuzzleFX[i].AdvanceLogic(DeltaTime);
                }
            }

            if (m_MuzzleSecondaryFX[i].Validate()) {
                // if it finished, get rid of it
                if (m_MuzzleSecondaryFX[i].IsFinished()) {
                    m_MuzzleSecondaryFX[i].KillInstance();
                } else if ((m_AltFiringPointBoneIndex[i] != -1)) {
                    // transform the effect
                    Matrix4 L2W = m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_AltFiringPointBoneIndex[i]);
                    L2W.PreTranslate(m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_AltFiringPointBoneIndex[i]));
                    m_MuzzleSecondaryFX[i].SetTransform(L2W);

                    // then run the logic
                    m_MuzzleSecondaryFX[i].AdvanceLogic(DeltaTime);
                }
            }
        }
    }

    // Do the NPC muzzle move logic..
    if (m_CurrentRenderState == RENDER_STATE_NPC || IsUsingSplitScreen()) {
        for (int i = 0; i < FIRE_POINT_COUNT; i++) {
            if (m_MuzzleNPCFX[i].Validate()) {
                // if it finished, get rid of it
                if (m_MuzzleNPCFX[i].IsFinished()) {
                    m_MuzzleNPCFX[i].KillInstance();
                } else if ((m_FiringPointBoneIndex[i] != -1)) {
                    Vector3      AimPos;
                    Vector3      BonePos;
                    Radian       Pitch, Yaw;
                    render_state OldState = m_CurrentRenderState;

                    // save off old state, just to be safe.
                    SetRenderState(RENDER_STATE_NPC);

                    // move the effect
                    if (GetAimBonePosition(AimPos, i) && GetFiringBonePosition(BonePos, i)) {
                        Vector3 Rot = AimPos - BonePos;
                        Matrix4 M;

                        Rot.GetPitchYaw(Pitch, Yaw);
                        M.Setup(Vector3(1.0f, 1.0f, 1.0f), Radian3(Pitch, Yaw, 0.0f), BonePos);

                        // point muzzle flash in a direction toward what we're aiming at
                        m_MuzzleNPCFX[i].SetTransform(M);
                    } else {
                        // transform the effect to the fire point so mp dual smps work
                        Matrix4 L2W = m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_FiringPointBoneIndex[i]);
                        L2W.PreTranslate(m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_FiringPointBoneIndex[i]));
                        m_MuzzleNPCFX[i].SetTransform(L2W);
                    }

                    // then run the logic
                    m_MuzzleNPCFX[i].AdvanceLogic(DeltaTime);

                    // put this back, just in case.
                    SetRenderState(OldState);
                }
            }
        }
    }
}

//==============================================================================

void new_weapon::InitMuzzleFx(bool bIsPrimary, int iFirePoint)
{
    assert(iFirePoint >= 0);
    assert(iFirePoint < FIRE_POINT_COUNT);

    // Start player muzzle flash?
    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        // make sure there's a name
        if (m_hMuzzleFXPrimary.getPointer() && bIsPrimary) {
            // if it's still valid, restart it.
            if (m_MuzzleFX[iFirePoint].Validate()) {
                m_MuzzleFX[iFirePoint].Restart();
            } else if (m_FiringPointBoneIndex[iFirePoint] != -1) {
                m_MuzzleFX[iFirePoint].InitInstance(m_hMuzzleFXPrimary.getPointer());
            }
        } else
            // make sure there's a name
            if (m_hMuzzleFXSecondary.getPointer()) {
                // if it's still valid, restart it.
                if (m_MuzzleSecondaryFX[iFirePoint].Validate()) {
                    m_MuzzleSecondaryFX[iFirePoint].Restart();
                } else if (m_AltFiringPointBoneIndex[iFirePoint] != -1) {
                    m_MuzzleSecondaryFX[iFirePoint].InitInstance(m_hMuzzleFXSecondary.getPointer());
                }
            }
    }

    // Start 3rd person muzzle flash?
    if (m_CurrentRenderState == RENDER_STATE_NPC || IsUsingSplitScreen()) {
        // if it's still valid, restart it.
        if (m_MuzzleNPCFX[iFirePoint].Validate()) {
            m_MuzzleNPCFX[iFirePoint].Restart();
        } else {
            // make sure there's a name
            if (m_hMuzzleFXPrimary.getPointer()) {
                m_MuzzleNPCFX[iFirePoint].InitInstance(m_hMuzzleFXPrimary.getPointer());
            }
        }
    }
}

//==============================================================================

void new_weapon::MoveMuzzleFx(void)
{
}

//==============================================================================

void new_weapon::KillAllMuzzleFX(void)
{
    // Clear all of the muzzle effect objects.
    for (int i = 0; i < FIRE_POINT_COUNT; i++) {
        m_MuzzleFX[i].KillInstance();
        m_MuzzleSecondaryFX[i].KillInstance();
        m_MuzzleNPCFX[i].KillInstance();
    }
}

//==============================================================================

void new_weapon::SetAnimation(const int& nAnimIndex, const float& fBlendTime, const bool& bResetFrames)
{
    if (m_CurrentRenderState == RENDER_STATE_NPC) {
        return;
    }

    //set the animation.
    m_AnimPlayer[m_CurrentRenderState].SetAnim(nAnimIndex, false, false, fBlendTime, bResetFrames);
}

//==============================================================================

void new_weapon::SetRotation(const float& rPitch, const float& rYaw)
{
    Matrix4 L2W = GetL2W();
    //now we transform
    L2W.SetRotation(Radian3(rPitch, rYaw, 0));
    OnTransform(L2W);
}

//==============================================================================

void new_weapon::OnEnumProp(prop_enum& PropEnumList)
{
    Object::OnEnumProp(PropEnumList);
    PropEnumList.PropEnumHeader("Inventory", "Stats for this item", PROP_TYPE_HEADER);

    int ID = PropEnumList.PushPath("Inventory\\");
    PropEnumList.PopPath(ID);

    PropEnumList.PropEnumHeader("Inventory\\Useable", "Is this inventory item useable by the this strain", 0);
    PropEnumList.PropEnumBool("Inventory\\Useable\\Human", "Can a human use me", 0);
    PropEnumList.PropEnumBool("Inventory\\Useable\\Strain One", "Can a strain one use me", 0);
    PropEnumList.PropEnumBool("Inventory\\Useable\\Strain Two", "Can a strain two use me", 0);
    PropEnumList.PropEnumBool("Inventory\\Useable\\Strain Three", "Can a strain three use me", 0);
    //    PropEnumList.AddEnum    ( "Inventory\\DropFadeTime", "INF\0TEN SEC\0THIRTY SEC\0ONE std::min\0", "How long to wait before destroying the object, infinite, 10 sec, 30 sec or 1 min" );
    PropEnumList.PropEnumExternal("Inventory\\Label", "Sound\0soundemitter\0", "Sound Descriptor (Label)", PROP_TYPE_MUST_ENUM);

    PropEnumList.PropEnumHeader("Render", "Render information.", 0);

    //--------------------------------------------------------------------------
    // Render instance.
    //--------------------------------------------------------------------------
    PropEnumList.PropEnumHeader("Render\\High res Inst", "Player weapon render instance", 0);
    ID = PropEnumList.PushPath("Render\\High res Inst\\");
    m_Skin[RENDER_STATE_PLAYER].OnEnumProp(PropEnumList);
    PropEnumList.PropEnumExternal("RenderInst\\Anim", "Resource\0anim\0", "Resource File", PROP_TYPE_MUST_ENUM);
    PropEnumList.PopPath(ID);

    //--------------------------------------------------------------------------
    // Lo-res RenderInst.
    //--------------------------------------------------------------------------

    //Enumerate the Lo res Render instance and animation file
    PropEnumList.PropEnumHeader("Render\\Lo res Inst", "NPC / Pickup weapon render instance", 0);
    ID = PropEnumList.PushPath("Render\\Lo res Inst\\");
    m_Skin[RENDER_STATE_NPC].OnEnumProp(PropEnumList);
    PropEnumList.PropEnumExternal("RenderInst\\Anim", "Resource\0anim\0", "Resource File", PROP_TYPE_MUST_ENUM);
    PropEnumList.PopPath(ID);

    // Enumerate the ammo information
    PropEnumList.PropEnumHeader("Ammunition", "Primary and secondary ammunition data", 0);

    PropEnumList.PropEnumHeader("Ammunition\\Primary Ammo", "Primary ammo info", 0);
    ID = PropEnumList.PushPath("Ammunition\\Primary Ammo\\");
    m_WeaponAmmo[AMMO_PRIMARY].OnEnumProp(PropEnumList);
    PropEnumList.PopPath(ID);

    PropEnumList.PropEnumHeader("Ammunition\\Secondary Ammo", "Secondary ammo info", 0);
    ID = PropEnumList.PushPath("Ammunition\\Secondary Ammo\\");
    m_WeaponAmmo[AMMO_SECONDARY].OnEnumProp(PropEnumList);
    PropEnumList.PopPath(ID);

    PropEnumList.PropEnumHeader("Weapon", "This is the properties that are unique for the weapons", 0);
    PropEnumList.PropEnumExternal("Weapon\\Audio Package", "Resource\0audiopkg", "The audio package associated with this weapon.", 0);

    PropEnumList.PropEnumExternal("Weapon\\Muzzle Flash Primary", "Resource\0fxo", "The particle effect associated with the primary muzzle flash.", 0);
    PropEnumList.PropEnumExternal("Weapon\\Muzzle Flash Secondary", "Resource\0fxo", "The particle effect associated with the secondary muzzle flash", 0);

    PropEnumList.PropEnumExternal("Weapon\\Reticle Center", "Resource\0xbmp\0", "The center reticle piece", 0);
    PropEnumList.PropEnumExternal("Weapon\\Reticle Edge", "Resource\0xbmp\0", "The edge reticle piece", 0);
    PropEnumList.PropEnumFloat("Weapon\\Reticle Center Pixel Offset", "How many pixels to add to the center reticle to offset the edge reticle by.", 0);

    PropEnumList.PropEnumFloat("Weapon\\Short Range", "Max distance considered short range for this weapon.", 0);
    PropEnumList.PropEnumFloat("Weapon\\Long Range", "Min distance considered long range for this weapon.", 0);
    PropEnumList.PropEnumInt("Weapon\\Accuracy Percent Long Range", "Accuracy is this percent of what it was at short range.", 0);

    //-------------------------------------------------------------------------
    // Reticle Radius Parameters
    //-------------------------------------------------------------------------
    PropEnumList.PropEnumHeader("Reticle Radius", "Parameters describing the behavior of the reticle's radius during play", 0);

    PropEnumList.PropEnumFloat("Reticle Radius\\Maximum Radius", "Maximum reticle radius, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Minimum Radius", "Minimum reticle radius, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Crouch Bonus", "Reticle radius bonus for crouching, added to \"Maximum Radius,\" in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Maximum Movement Penalty", "Worst radius penalty for moving at speed, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Movement Shrink Acceleration", "Acceleration rate for shrinking the reticle from movement, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Shot Shrink Acceleration", "Acceleration rate for shrinking the reticle from a shot, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Grow Acceleration", "Acceleration rate for growing the reticle, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Shot Penalty", "Radius penalty for a single shot from this weapon, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Reticle Radius\\Shot Penalty Degrade Rate", "How fast the accumulated shot penalties degrade to zero, in screen pixels/second.", 0);

    //-------------------------------------------------------------------------
    // Alt Reticle Radius Parameters
    //-------------------------------------------------------------------------
    PropEnumList.PropEnumHeader("Alt Reticle Radius", "Parameters describing the behavior of the reticle's radius during play", 0);

    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Maximum Radius", "Maximum reticle radius, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Minimum Radius", "Minimum reticle radius, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Crouch Bonus", "Reticle radius bonus for crouching, added to \"Maximum Radius,\" in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Maximum Movement Penalty", "Worst radius penalty for moving at speed, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Movement Shrink Acceleration", "Acceleration rate for shrinking the reticle from movement, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Shot Shrink Acceleration", "Acceleration rate for shrinking the reticle from a shot, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Grow Acceleration", "Acceleration rate for growing the reticle, in screen pixels/second^2.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Shot Penalty", "Radius penalty for a single shot from this weapon, in screen pixels.", 0);
    PropEnumList.PropEnumFloat("Alt Reticle Radius\\Shot Penalty Degrade Rate", "How fast the accumulated shot penalties degrade to zero, in screen pixels/second.", 0);

    /* IJB
    PropEnumList.PropEnumHeader("Faction Fire Sound", "Weapon fire sounds for the factions.", 0);
    for (int i = 0; i < (factions_manager::s_FactionList.GetCount() - 1); i++) {
        PropEnumList.PropEnumExternal(xfs("Faction Fire Sound\\%s", factions_manager::s_FactionList.GetStringFromIndex(i)), "Sound\0soundexternal\0",
                                      "What firing sound to play for this faction, this only for AI's", 0);
    }
                                      */
}

//==============================================================================

bool new_weapon::OnProperty(prop_query& PropQuery)
{
    // Base classes
    if (Object::OnProperty(PropQuery)) {
        return true;
    }

    // External
    if (PropQuery.IsVar("Weapon\\Audio Package")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_hAudioPackage.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = PropQuery.GetVarExternal();

            if (pString[0]) {
                if (std::string(pString) == "<null>") {
                    m_hAudioPackage.setName("");
                } else {
                    m_hAudioPackage.setName(pString);

                    // Load the audio package.
                    if (m_hAudioPackage.isLoaded() == false) {
                        m_hAudioPackage.getPointer();
                    }
                }
            }
        }
        return (true);
    }

    // External
    if (PropQuery.IsVar("Weapon\\Muzzle Flash Primary")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_hMuzzleFXPrimary.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = PropQuery.GetVarExternal();

            if (pString[0]) {
                m_hMuzzleFXPrimary.setName(pString);

                // Load the audio package.
                if (m_hMuzzleFXPrimary.isLoaded() == false) {
                    m_hMuzzleFXPrimary.getPointer();
                }
            }
        }
        return (true);
    }

    // External
    if (PropQuery.IsVar("Weapon\\Muzzle Flash Secondary")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_hMuzzleFXSecondary.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = PropQuery.GetVarExternal();

            if (pString[0]) {
                m_hMuzzleFXSecondary.setName(pString);

                // Load the audio package.
                if (m_hMuzzleFXSecondary.isLoaded() == false) {
                    m_hMuzzleFXSecondary.getPointer();
                }
            }
        }
        return (true);
    }

    // External
    if (PropQuery.IsVar("Weapon\\Reticle Center")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_ReticleCenter.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = PropQuery.GetVarExternal();

            if (pString[0]) {
                m_ReticleCenter.setName(pString);

                // Load the audio package.
                if (m_ReticleCenter.isLoaded() == false) {
                    m_ReticleCenter.getPointer();
                }
            }
        }
        return (true);
    }

    // External
    if (PropQuery.IsVar("Weapon\\Reticle Edge")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_ReticleEdge.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Get the FileName
            const char* pString = PropQuery.GetVarExternal();

            if (pString[0]) {
                m_ReticleEdge.setName(pString);

                // Load the audio package.
                if (m_ReticleEdge.isLoaded() == false) {
                    m_ReticleEdge.getPointer();
                }
            }
        }
        return (true);
    }

    if (PropQuery.VarFloat("Weapon\\Reticle Center Pixel Offset", m_ReticleCenterPixelOffset)) {
        m_ReticleCenterPixelOffset = std::max(0.0f, m_ReticleCenterPixelOffset);
        return true;
    }

    if (PropQuery.VarFloat("Weapon\\Short Range", m_ShortRange)) {
        m_ShortRange = std::max(0.0f, m_ShortRange);
        return true;
    }

    if (PropQuery.VarFloat("Weapon\\Long Range", m_LongRange)) {
        m_LongRange = std::max(0.0f, m_LongRange);
        return true;
    }

    if (PropQuery.VarInt("Weapon\\Accuracy Percent Long Range", m_AccuracyPercentLongRange)) {
        m_AccuracyPercentLongRange = std::max(0, m_AccuracyPercentLongRange);
        m_AccuracyPercentLongRange = std::min(100, m_AccuracyPercentLongRange);
        return true;
    }

    //
    // Human single handed.
    //
    int ID = PropQuery.PushPath("Render\\High res Inst\\");

    if (m_Skin[RENDER_STATE_PLAYER].OnProperty(PropQuery)) {
        if (!PropQuery.IsRead() && PropQuery.IsVar("RenderInst\\File")) {
            InstallCustomScope();
        }
        return true;
    }

    // Animation
    if (PropQuery.IsVar("RenderInst\\Anim")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_AnimGroup[RENDER_STATE_PLAYER].getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            if (strlen(PropQuery.GetVarExternal()) > 0) {
                m_AnimGroup[RENDER_STATE_PLAYER].setName(PropQuery.GetVarExternal());
                if (m_AnimGroup[RENDER_STATE_PLAYER].getPointer()) {
                    m_AnimPlayer[RENDER_STATE_PLAYER].SetAnimGroup(m_AnimGroup[RENDER_STATE_PLAYER]);
                    m_AnimPlayer[RENDER_STATE_PLAYER].SetAnim(0, true, true);
                    SetFlagBits(GetFlagBits() | FLAG_DIRTY_TRANSFORM);
                    OnTransform(GetL2W());
                }
            }
        }
        return true;
    }

    PropQuery.PopPath(ID);

    //--------------------------------------------------------------------------
    // Lo-res RenderInst.
    //-------------------------------------------   -------------------------------

    ID = PropQuery.PushPath("Render\\Lo res Inst\\");
    if (m_Skin[RENDER_STATE_NPC].OnProperty(PropQuery)) {
        return true;
    }
    // Animation
    if (PropQuery.IsVar("RenderInst\\Anim")) {
        if (PropQuery.IsRead()) {
            PropQuery.SetVarExternal(m_AnimGroup[RENDER_STATE_NPC].getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            // Anim changed?
            if (strlen(PropQuery.GetVarExternal()) > 0) {
                m_AnimGroup[RENDER_STATE_NPC].setName(PropQuery.GetVarExternal());
                if (m_AnimGroup[RENDER_STATE_NPC].getPointer()) {
                    m_AnimPlayer[RENDER_STATE_NPC].SetAnimGroup(m_AnimGroup[RENDER_STATE_NPC]);
                    m_AnimPlayer[RENDER_STATE_NPC].SetAnim(0, true, true);
                    //                    m_AnimPlayer[m_CurrentRenderState].SetPosition( GetL2W().GetTranslation() );
                    SetFlagBits(GetFlagBits() | FLAG_DIRTY_TRANSFORM);
                    OnTransform(GetL2W());
                }
            }
        }
        return true;
    }
    PropQuery.PopPath(ID);

    // Primary Ammo
    ID = PropQuery.PushPath("Ammunition\\Primary Ammo\\");
    if (m_WeaponAmmo[AMMO_PRIMARY].OnProperty(PropQuery)) {
        m_OriginalWeaponAmmo[AMMO_PRIMARY] = m_WeaponAmmo[AMMO_PRIMARY];
        return true;
    }
    PropQuery.PopPath(ID);

    // Secondary Ammo
    ID = PropQuery.PushPath("Ammunition\\Secondary Ammo\\");
    if (m_WeaponAmmo[AMMO_SECONDARY].OnProperty(PropQuery)) {
        m_OriginalWeaponAmmo[AMMO_SECONDARY] = m_WeaponAmmo[AMMO_SECONDARY];
        return true;
    }
    PropQuery.PopPath(ID);

    //
    // Reticle Radius Parameters
    //
    if (PropQuery.VarFloat("Reticle Radius\\Maximum Radius", m_ReticleRadiusParameters.m_MaxRadius)) {
        m_ReticleRadiusParameters.m_MaxRadius = std::max(m_ReticleRadiusParameters.m_MinRadius, m_ReticleRadiusParameters.m_MaxRadius);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Minimum Radius", m_ReticleRadiusParameters.m_MinRadius)) {
        m_ReticleRadiusParameters.m_MinRadius = std::min(m_ReticleRadiusParameters.m_MinRadius, m_ReticleRadiusParameters.m_MaxRadius);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Crouch Bonus", m_ReticleRadiusParameters.m_CrouchBonus)) {
        m_ReticleRadiusParameters.m_CrouchBonus = std::max(m_ReticleRadiusParameters.m_CrouchBonus, 0.0f);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Maximum Movement Penalty", m_ReticleRadiusParameters.m_MaxMovementPenalty)) {
        m_ReticleRadiusParameters.m_MaxMovementPenalty = std::max(0.0f, m_ReticleRadiusParameters.m_MaxMovementPenalty);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Movement Shrink Acceleration", m_ReticleRadiusParameters.m_MoveShrinkAccel)) {
        m_ReticleRadiusParameters.m_MoveShrinkAccel = std::max(0.0f, m_ReticleRadiusParameters.m_MoveShrinkAccel);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Shot Shrink Acceleration", m_ReticleRadiusParameters.m_ShotShrinkAccel)) {
        m_ReticleRadiusParameters.m_ShotShrinkAccel = std::max(0.0f, m_ReticleRadiusParameters.m_ShotShrinkAccel);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Grow Acceleration", m_ReticleRadiusParameters.m_GrowAccel)) {
        m_ReticleRadiusParameters.m_GrowAccel = std::max(0.0f, m_ReticleRadiusParameters.m_GrowAccel);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Shot Penalty", m_ReticleRadiusParameters.m_PenaltyForShot)) {
        m_ReticleRadiusParameters.m_PenaltyForShot = std::max(0.0f, m_ReticleRadiusParameters.m_PenaltyForShot);
        return true;
    }
    if (PropQuery.VarFloat("Reticle Radius\\Shot Penalty Degrade Rate", m_ReticleRadiusParameters.m_ShotPenaltyDegradeRate)) {
        m_ReticleRadiusParameters.m_ShotPenaltyDegradeRate = std::max(0.0f, m_ReticleRadiusParameters.m_ShotPenaltyDegradeRate);
        return true;
    }

    //
    // Alt Reticle Radius Parameters
    //
    if (PropQuery.VarFloat("Alt Reticle Radius\\Maximum Radius", m_AltReticleRadiusParameters.m_MaxRadius)) {
        m_AltReticleRadiusParameters.m_MaxRadius = std::max(m_AltReticleRadiusParameters.m_MinRadius, m_AltReticleRadiusParameters.m_MaxRadius);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Minimum Radius", m_AltReticleRadiusParameters.m_MinRadius)) {
        m_AltReticleRadiusParameters.m_MinRadius = std::min(m_AltReticleRadiusParameters.m_MinRadius, m_AltReticleRadiusParameters.m_MaxRadius);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Crouch Bonus", m_AltReticleRadiusParameters.m_CrouchBonus)) {
        m_AltReticleRadiusParameters.m_CrouchBonus = std::max(m_AltReticleRadiusParameters.m_CrouchBonus, 0.0f);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Maximum Movement Penalty", m_AltReticleRadiusParameters.m_MaxMovementPenalty)) {
        m_AltReticleRadiusParameters.m_MaxMovementPenalty = std::max(0.0f, m_AltReticleRadiusParameters.m_MaxMovementPenalty);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Movement Shrink Acceleration", m_AltReticleRadiusParameters.m_MoveShrinkAccel)) {
        m_AltReticleRadiusParameters.m_MoveShrinkAccel = std::max(0.0f, m_AltReticleRadiusParameters.m_MoveShrinkAccel);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Shot Shrink Acceleration", m_AltReticleRadiusParameters.m_ShotShrinkAccel)) {
        m_AltReticleRadiusParameters.m_ShotShrinkAccel = std::max(0.0f, m_AltReticleRadiusParameters.m_ShotShrinkAccel);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Grow Acceleration", m_AltReticleRadiusParameters.m_GrowAccel)) {
        m_AltReticleRadiusParameters.m_GrowAccel = std::max(0.0f, m_AltReticleRadiusParameters.m_GrowAccel);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Shot Penalty", m_AltReticleRadiusParameters.m_PenaltyForShot)) {
        m_AltReticleRadiusParameters.m_PenaltyForShot = std::max(0.0f, m_AltReticleRadiusParameters.m_PenaltyForShot);
        return true;
    }
    if (PropQuery.VarFloat("Alt Reticle Radius\\Shot Penalty Degrade Rate", m_AltReticleRadiusParameters.m_ShotPenaltyDegradeRate)) {
        m_AltReticleRadiusParameters.m_ShotPenaltyDegradeRate = std::max(0.0f, m_AltReticleRadiusParameters.m_ShotPenaltyDegradeRate);
        return true;
    }

    // NPC faction fire sfx list.
    /* IJB
    for (int i = 0; i < (factions_manager::s_FactionList.GetCount() - 1); i++) {
        if (PropQuery.IsVar(xfs("Faction Fire Sound\\%s", factions_manager::s_FactionList.GetStringFromIndex(i)))) {
            if (PropQuery.IsRead()) {
                const factions& rFaction = factions_manager::s_FactionList[i];

                if (m_FactionFireSfx[factions_manager::GetFactionBitIndex(rFaction)] != -1) {
                    PropQuery.SetVarExternal(g_StringMgr.GetString(m_FactionFireSfx[factions_manager::GetFactionBitIndex(rFaction)]), 64);
                } else {
                    PropQuery.SetVarExternal("", 64);
                }
            } else {
                // Get the FileName
                xstring ExtString = PropQuery.GetVarExternal();
                if (!ExtString.IsEmpty()) {
                    xstring String(ExtString);

                    int PkgIndex = String.Find('\\', 0);

                    if (PkgIndex != -1) {
                        xstring Pkg = String.Left(PkgIndex);
                        String.Delete(0, PkgIndex + 1);
                    }

                    const factions& rFaction = factions_manager::s_FactionList[i];
                    m_FactionFireSfx[factions_manager::GetFactionBitIndex(rFaction)] = g_StringMgr.Add(String);
                }
            }
            return true;
        }
    }
*/
    return false;
}

//===========================================================================

void new_weapon::ammo::OnEnumProp(prop_enum& List)
{
    List.PropEnumInt("Max Ammunition", "The maximum ammount of ammo that can be carried", 0);
    List.PropEnumInt("Ammunition per clip", "Ammount of ammo per clip", 0);
    List.PropEnumInt("Ammo ammount", "Number of bullets in this weapon", 0);
    List.PropEnumExternal("BitmapResource", "Resource\0xbmp\0", "Bitmap resource.", PROP_TYPE_MUST_ENUM | PROP_TYPE_EXTERNAL);

    List.PropEnumFileName("Projectile Blueprint Path",
                          "Projectile object blueprints (*.bpx)|*.bpx|All Files (*.*)|*.*||",
                          "Resource for this item",
                          PROP_TYPE_MUST_ENUM);
}

//===========================================================================

bool new_weapon::ammo::OnProperty(prop_query& rPropQuery)
{
    if (rPropQuery.VarInt("Max Ammunition", m_AmmoMax)) {
        return true;
    }

    if (rPropQuery.VarInt("Ammunition per clip", m_AmmoPerClip)) {
        return true;
    }

    if (rPropQuery.VarInt("Ammo ammount", m_AmmoAmount)) {
        return true;
    }

    if (rPropQuery.IsVar("BitmapResource")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarExternal(m_Bitmap.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            const char* pStr = rPropQuery.GetVarExternal();
            (void)pStr;
            m_Bitmap.setName(pStr);
        }
        return true;
    }

    if (rPropQuery.IsVar("Projectile Blueprint")) {
        return true;
    }

    /* IJB
    if (rPropQuery.IsVar("Projectile Blueprint Path")) {
        if (rPropQuery.IsRead()) {
            if (m_ProjectileTemplateID < 0) {
                rPropQuery.SetVarFileName("", 256);
            } else {
                rPropQuery.SetVarFileName(g_TemplateStringMgr.GetString(m_ProjectileTemplateID), 256);
            }
        } else {
            if (strlen(rPropQuery.GetVarFileName()) > 0) {
                m_ProjectileTemplateID = g_TemplateStringMgr.Add(rPropQuery.GetVarFileName());
            }
        }
        return true;
    }
*/
    return false;
}

//==============================================================================

void new_weapon::ResetWeapon(void)
{
    ReleaseAudio();
    ClearZoom();
}

//==============================================================================

void new_weapon::ProcessSfx(void)
{
}

//==============================================================================

void new_weapon::BeginPrimaryFire(void)
{
}

//==============================================================================

void new_weapon::EndPrimaryFire(void)
{
}

//==============================================================================

void new_weapon::BeginAltFire(void)
{
}

//==============================================================================
void new_weapon::EndAltFire(void)
{
}

//==============================================================================

void new_weapon::ReleaseAudio(void)
{
}

//==============================================================================
// This creates bullets through the template manager.
base_projectile* new_weapon::CreateBullet(
    const char*               pWeaponLogicalName,
    const Vector3&            InitPos,
    const Radian3&            InitRot,
    const Vector3&            InheritedVelocity,
    const guid                OwnerGuid,
    const pain_handle&        PainHandle,
    new_weapon::ammo_priority Priority,
    bool                      bHitLiving,
    int                       iFirePoint)
{
    Object* pOwner = objectManager->GetObjectByGuid(OwnerGuid);
    assert(pOwner);

    uint16_t Zone1 = pOwner->GetZone1();
    uint16_t Zone2 = pOwner->GetZone2();

    //create Bullet
    /* IJB
    guid    BulletID = g_TemplateMgr.CreateSingleTemplate(g_TemplateStringMgr.GetString(m_WeaponAmmo[Priority].m_ProjectileTemplateID), InitPos, InitRot, Zone1, Zone2);
    Object* pObject = objectManager->GetObjectByGuid(BulletID);
    assert(pObject);
    assert(pObject->IsKindOf(base_projectile::GetRTTI()));

    base_projectile* pBullet = (base_projectile*)pObject;

    assert(pBullet);
    if (pBullet != nullptr) {
        // Lookup speed
        tweak_handle SpeedTweak(xfs("%s_SPEED", pWeaponLogicalName));
        pBullet->Initialize(InitPos, InitRot, InheritedVelocity, SpeedTweak.GetF32(), OwnerGuid, PainHandle, bHitLiving, iFirePoint);

        // Lookup pain degradation
        tweak_handle PainDropDistTweak(xfs("%s_PainDropDist", pWeaponLogicalName));
        tweak_handle PainDropScaleTweak(xfs("%s_PainDropScale", pWeaponLogicalName));
        pBullet->SetPainDegradation(PainDropDistTweak.GetF32(), PainDropScaleTweak.GetF32());
    }
    return pBullet;
    */
    assert(false);
    return nullptr;
}
//==============================================================================
//change the attribs flags...

bool new_weapon::GetFiringBonePosition(Vector3& Pos, int iBone)
{
    if (m_FiringPointBoneIndex[iBone] != -1) {
        if (m_CurrentRenderState == RENDER_STATE_NPC) {
            // Using a dual weapon?
            if ((m_Item == INVEN_WEAPON_DUAL_SMP) || (m_Item == INVEN_WEAPON_DUAL_SHT)) {
                // Get position from anim player since special controller splits same geometry
                // weapon into correct positions so we can't use the faster method below
                Pos = m_AnimPlayer[m_CurrentRenderState].GetBonePosition(m_FiringPointBoneIndex[iBone]);
            } else {
                // Use optimized method that doesn't require computing animation matrices
                Pos = m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_FiringPointBoneIndex[iBone]);
                Pos = GetL2W().Transform(Pos);
            }
        } else if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
            Pos = m_AnimPlayer[m_CurrentRenderState].GetBonePosition(m_FiringPointBoneIndex[iBone]);
        }

        return true;
    }

    return false;
}

//==============================================================================

bool new_weapon::GetAimBonePosition(Vector3& Pos, int iBone)
{
    if (m_AimPointBoneIndex[iBone] != -1) {
        if (m_CurrentRenderState == RENDER_STATE_NPC) {
            // Using a dual weapon?
            if ((m_Item == INVEN_WEAPON_DUAL_SMP) || (m_Item == INVEN_WEAPON_DUAL_SHT)) {
                // Get position from anim player since special controller splits same geometry
                // weapon into correct positions so we can't use the faster method below
                Pos = m_AnimPlayer[m_CurrentRenderState].GetBonePosition(m_AimPointBoneIndex[iBone]);
            } else {
                // Use optimized method that doesn't require computing animation matrices
                Pos = m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_AimPointBoneIndex[iBone]);
                Pos = GetL2W().Transform(Pos);
            }
        } else if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
            Pos = m_AnimPlayer[m_CurrentRenderState].GetBonePosition(m_AimPointBoneIndex[iBone]);
        }

        return true;
    }
    return false;
}

//==============================================================================

bool new_weapon::CheckFirePoint(void)
{
    return (m_FiringPointBoneIndex[FIRE_POINT_DEFAULT] != -1);
}

//==============================================================================

bool new_weapon::CheckFlashlightPoint(void)
{
    return (m_FlashlightBoneIndex != -1);
}

//==============================================================================

bool new_weapon::GetFlashlightTransformInfo(Matrix4& incMatrix, Vector3& incVect)
{
    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        // return the matrix and the bone vector
        incMatrix = m_AnimPlayer[m_CurrentRenderState].GetBoneL2W(m_FlashlightBoneIndex);
        incVect = m_AnimPlayer[m_CurrentRenderState].GetBindPosition(m_FlashlightBoneIndex);

        return true;
    }

    // not in the proper state i.e. RENDER_STATE_NPC
    return false;
}

void new_weapon::OnKill()
{
    UninstallCustomScope();
}

//===========================================================================

void new_weapon::AddAmmoToWeapon(int nAmmoPrimary, int nAmmoSecondary)
{
    m_WeaponAmmo[AMMO_PRIMARY].m_AmmoAmount = std::min(m_WeaponAmmo[AMMO_PRIMARY].m_AmmoAmount + nAmmoPrimary, m_WeaponAmmo[AMMO_PRIMARY].m_AmmoMax);
    m_WeaponAmmo[AMMO_SECONDARY].m_AmmoAmount = std::min(m_WeaponAmmo[AMMO_SECONDARY].m_AmmoAmount + nAmmoSecondary, m_WeaponAmmo[AMMO_SECONDARY].m_AmmoMax);

    Object* pOwner = objectManager->GetObjectByGuid(m_OwnerGuid);

    // owner SHOULD be an actor
    if (pOwner && pOwner->IsKindOf(actor::GetRTTI())) {
        actor*     pActor = (actor*)pOwner;
        inven_item DualItem = GetDualWeaponID(m_Item);

        // if we have a dual, add the ammo to it as well so that we display ammo correctly.
        if (DualItem != INVEN_NULL) {
            new_weapon* pDualWeapon = pActor->GetWeaponPtr(DualItem);
            if (pDualWeapon) {
                pDualWeapon->AddAmmoToWeapon(nAmmoPrimary, nAmmoSecondary);
            }
        }
    }
}

//===========================================================================
void new_weapon::RefillClip(ammo_priority Priority)
{
    int Amount = GetAmmoPerClip(Priority);

    // make sure we have full ammo in this weapon's clip
    m_WeaponAmmo[Priority].m_AmmoInCurrentClip = Amount;
}

//===========================================================================

void new_weapon::ClearAmmo(const ammo_priority& rAmmoPriority)
{
    // clear count of bullets in current clip
    if (m_WeaponAmmo[rAmmoPriority].m_ProjectileType != BULLET_MUTATION) {
        m_WeaponAmmo[rAmmoPriority].m_AmmoInCurrentClip = 0;

        // clear all bullets
        m_WeaponAmmo[rAmmoPriority].m_AmmoAmount = 0;
    }
}

//===========================================================================

void new_weapon::ClearClipAmmo(const ammo_priority& rAmmoPriority)
{
    // clear current weapon's clip ammo so that we'll have to reload once we dump the dual.
    // so if you had 30/270 when you picked up duals, you would have 0/270 when you discarded the dual.  (and 30/240 after reload).
    // this is basically just burning what is in the clip.
    {
        // how much ammo do we remove from the total?
        int nTotal = m_WeaponAmmo[rAmmoPriority].m_AmmoAmount;
        int nClip = m_WeaponAmmo[rAmmoPriority].m_AmmoInCurrentClip;

        // make sure we don't subtract more than we have.
        int nRemove = std::min(nTotal, nClip);

        // take it out
        m_WeaponAmmo[rAmmoPriority].m_AmmoAmount -= nRemove;
        m_WeaponAmmo[rAmmoPriority].m_AmmoInCurrentClip = 0;
    }
}

//===========================================================================

void new_weapon::DecrementAmmo(const ammo_priority& rAmmoPriority, const int& nAmt)
{
    // decrement count of bullets in current clip
    m_WeaponAmmo[rAmmoPriority].m_AmmoInCurrentClip -= nAmt;

    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

    /* IJB
    if (!pObj->IsKindOf(character::GetRTTI())) {
        // also, take away from total if we aren't unlimited ammo
        m_WeaponAmmo[rAmmoPriority].m_AmmoAmount -= nAmt;
    }
        */
}

void new_weapon::FillAmmo()
{
}

Bitmap* new_weapon::GetCenterReticleBmp()
{
    return m_ReticleCenter.getPointer();
}

//==============================================================================

Bitmap* new_weapon::GetEdgeReticleBmp()
{
    return m_ReticleEdge.getPointer();
}

//==============================================================================

bool new_weapon::CanIntereptPrimaryFire(int nFireAnimIndex)
{
    // If we are currently playing the fire animation then see if its at end before we interupt it.
    if (m_AnimPlayer[m_CurrentRenderState].GetAnimIndex() == nFireAnimIndex) {
        return m_AnimPlayer[m_CurrentRenderState].IsAtEnd();
    }

    return true;
}

//==============================================================================

bool new_weapon::CanIntereptSecondaryFire(int nFireAnimIndex)
{
    // If we are currently playing the fire animation then see if its at end before we interupt it.
    if (m_AnimPlayer[m_CurrentRenderState].GetAnimIndex() == nFireAnimIndex) {
        return m_AnimPlayer[m_CurrentRenderState].IsAtEnd();
    }

    return true;
}
//==============================================================================

void new_weapon::SetupRenderInformation(void)
{
    for (int i = 0; i < AMMO_MAX; i++) {
        if (m_WeaponAmmo[i].m_AmmoInCurrentClip > m_WeaponAmmo[i].m_AmmoPerClip) {
            m_WeaponAmmo[i].m_AmmoAmount += m_WeaponAmmo[i].m_AmmoInCurrentClip - m_WeaponAmmo[i].m_AmmoPerClip;
        }

        m_WeaponAmmo[i].m_AmmoMax = m_OriginalWeaponAmmo[i].m_AmmoMax;
        m_WeaponAmmo[i].m_AmmoPerClip = m_OriginalWeaponAmmo[i].m_AmmoPerClip;
    }

    // If we have a valid anim group fill out the bone indecies.
    AnimGroup::handle& CurAnimGroup = m_AnimGroup[m_CurrentRenderState];
    if (CurAnimGroup.getPointer()) {
        m_FiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint");
        m_AltFiringPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint");
        m_AimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint");
        m_AltAimPointBoneIndex[FIRE_POINT_DEFAULT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint");

        m_FiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_left");
        m_AltFiringPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_left");
        m_AimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_left");
        m_AltAimPointBoneIndex[FIRE_POINT_LEFT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_left");

        m_FiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("firepoint_right");
        m_AltFiringPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altfirepoint_right");
        m_AimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("aimpoint_right");
        m_AltAimPointBoneIndex[FIRE_POINT_RIGHT] = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("altaimpoint_right");

        // flashlight bone
        m_FlashlightBoneIndex = m_AnimPlayer[m_CurrentRenderState].GetBoneIndex("lightpoint");

        // try and validate flashlight bone
        if (m_FlashlightBoneIndex == -1) {
            // no flashlight point, try to put it on the firing point
            if (CheckFirePoint()) {
                m_FlashlightBoneIndex = m_FiringPointBoneIndex[FIRE_POINT_DEFAULT];
            } else // try one more time to find a bone
                if (m_FiringPointBoneIndex[FIRE_POINT_RIGHT] != -1) {
                    m_FlashlightBoneIndex = m_FiringPointBoneIndex[FIRE_POINT_RIGHT];
                } else {
                    //x_DebugMsg( "new_weapon::SetupRenderInformation - No flashlight bone\n");
                }
        }
    }

    KillAllMuzzleFX();
}

void new_weapon::UpdateReticle(float DeltaTime)
{
    (void)DeltaTime;

    if (!ShouldUpdateReticle()) {
        return;
    }

    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

    if (pObj && pObj->IsKindOf(player::GetRTTI())) {
        player* pPlayer = (player*)pObj;

        // if this is a player and we're not running a cinema, see if we are over a target with our reticle
        if (pPlayer && (!pPlayer->IsCinemaRunning()) && CheckReticleLocked()) {
            // reticle was NOT already on
            if (!m_bLockedOn) {
                m_bLockedOn = true;
                // IJB g_AudioMgr.Play( "Reticule_Shift_Red" );
            }
        } else {
            m_bLockedOn = false;
        }
    } else {
        m_bLockedOn = false;
    }
}

//==============================================================================

void new_weapon::UpdateAmmoWarning(void)
{
    float fCurAmmo = (float)m_WeaponAmmo[AMMO_PRIMARY].m_AmmoInCurrentClip;
    float fMaxAmmo = (float)m_WeaponAmmo[AMMO_PRIMARY].m_AmmoPerClip;
    int   fTotalAmmo = m_WeaponAmmo[AMMO_PRIMARY].m_AmmoAmount;

    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);
    if (pObj) {
        player* pPlayer = (player*)pObj;

        HudObject* Hud = pPlayer->GetHud();
        /* IJB
                if (Hud && Hud->m_Initialized) {
                    // if we have low ammo, warn the player
                    if ((fCurAmmo / fMaxAmmo) <= s_LowAmmoPercent) {
                        // make sure we can warn the player (so we don't get constant annoyances, just warn once).
                        if (m_bCanWarnLowAmmo) {

                            // does this load use a warning message for low clip ammo?
                            if (pPlayer->LoadWarnsLowAmmo()) {
                                // set up message and sound and send to player
                                // IJB MsgMgr.Message( MSG_LOW_AMMO, pPlayer->net_GetSlot() );
                            }

                            m_bCanWarnLowAmmo = false;
                        }

                        // out of ammo, tell player
                        if (fCurAmmo == 0.0f) {
                            bool bShouldFlash = (fTotalAmmo <= 0);

                            // we can reload because we have more ammo, stop the flashing
                            Hud->SetElementPulseState(HUD_ELEMENT_AMMO_BAR, bShouldFlash);
                        } else {
                            // just low, clip not empty yet (NOTE: this will continue to flash if we are completely out of ammo)
                            Hud->SetElementPulseState(HUD_ELEMENT_AMMO_BAR, true);
                        }

                        Colour WarningColor(150, 15, 15, 255);
                        SetAmmoHudColor(pPlayer, Hud, WarningColor);
                    } else {
                        // reset warning flag
                        m_bCanWarnLowAmmo = true;

                        // turn off pulsing
                        Hud->SetElementPulseState(HUD_ELEMENT_AMMO_BAR, false);

                        // set normal color
                        SetAmmoHudColor(pPlayer, Hud, g_HudColor);
                    }
                }
                    */
    }
}

//==============================================================================
void new_weapon::SetAmmoHudColor(player* pPlayer, HudObject* Hud, Colour HudColor)
{
    assert(Hud);
    assert(pPlayer);
    /* IJB
    player_hud& PHud = Hud->GetPlayerHud(pPlayer->GetLocalSlot());
    hud_ammo*   pAHud = (hud_ammo*)(PHud.m_HudComponents[HUD_ELEMENT_AMMO_BAR]);
    pAHud->SetRenderColor(HudColor);
    */
}

//==============================================================================

bool new_weapon::CheckReticleLocked()
{
    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);
    if (pObj && pObj->IsKindOf(player::GetRTTI())) {
        player* pPlayer = (player*)pObj;

        // if we are over something with the reticle, make sure it's an enemy
        if (pPlayer->ReticleOnTarget() && (pPlayer->GetEnemyOnReticle() != NULL_GUID)) {
            return true;
        }
    }

    return false;
}

//==============================================================================
void new_weapon::SetRenderState(render_state RenderState)
{
    //set the render state
    m_CurrentRenderState = RenderState;
}

//==============================================================================

bool new_weapon::ContinueReload(void)
{
    return false;
}

//==============================================================================

float new_weapon::GetZoomLevel(void)
{
    //  assert( m_CurrentRenderState == RENDER_STATE_PLAYER );

    Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);
    if (pObj) {
        player* pPlayer = (player*)pObj;

        const player::view_info& ViewInfo = pPlayer->GetViewInfo();
        const player::view_info& OriginalViewInfo = pPlayer->GetOriginalViewInfo();

        float ZoomLevel = OriginalViewInfo.XFOV / ViewInfo.XFOV;

        return ZoomLevel;
    } else {
        return 0.0f;
    }
}

//==============================================================================

int new_weapon::GetZoomStep(void)
{
    return m_ZoomStep;
}

//==============================================================================

bool new_weapon::IsZoomEnabled(void)
{
    return (m_ZoomState != NO_ZOOM_WEAPON_STATE);
}

//==============================================================================

Radian new_weapon::GetXFOV(void)
{
    assert(m_CurrentRenderState == RENDER_STATE_PLAYER);
    assert(m_ZoomState != NO_ZOOM_WEAPON_STATE);

    return m_CurrentViewX;
}

//==============================================================================

float new_weapon::GetZoomMovementMod(void)
{
    assert(m_CurrentRenderState == RENDER_STATE_PLAYER);
    assert(m_ZoomState != NO_ZOOM_WEAPON_STATE);

    return m_ZoomMovementSpeed;
}

//==============================================================================

int new_weapon::IncrementZoom(void)
{
    assert(m_CurrentRenderState == RENDER_STATE_PLAYER);

    if (m_ZoomStep == 0) {
        // first time, start me up
        player&                  Player = player::GetSafeType(*objectManager->GetObjectByGuid(m_ParentGuid));
        const player::view_info& OriginalViewInfo = Player.GetOriginalViewInfo();

        m_CurrentViewX = OriginalViewInfo.XFOV;
        m_ZoomState = WEAPON_STATE_ZOOM_IN;
    }

    m_ZoomStep++;

    if (m_ZoomStep > m_nZoomSteps) {
        m_ZoomState = WEAPON_STATE_ZOOM_OUT;
        m_ZoomStep = 0;
    }

    return m_ZoomStep;
}

//==============================================================================

void new_weapon::ClearZoom(void)
{
    if (m_CurrentRenderState == RENDER_STATE_PLAYER) {
        Object* pParent = objectManager->GetObjectByGuid(m_ParentGuid);
        assert(pParent);
        player&                  Player = player::GetSafeType(*pParent);
        const player::view_info& OriginalViewInfo = Player.GetOriginalViewInfo();

        m_CurrentViewX = OriginalViewInfo.XFOV;
        m_ZoomState = NO_ZOOM_WEAPON_STATE;
        m_ZoomStep = 0;

        Player.ResetView();
    }
}

//==============================================================================

render_inst* new_weapon::GetRenderInstPtr(void)
{
    if (IN_RANGE(0, m_CurrentRenderState, RENDER_STATE_MAX)) {
        return &m_Skin[m_CurrentRenderState];
    } else {
        return nullptr;
    }
}

//==============================================================================

Geom* new_weapon::GetGeomPtr(void)
{
    render_inst* pRenderInst = GetRenderInstPtr();
    return (pRenderInst != nullptr) ? pRenderInst->GetGeom() : nullptr;
}

AnimGroup::handle* new_weapon::GetAnimGroupHandlePtr(void)
{
    if (IN_RANGE(0, m_CurrentRenderState, RENDER_STATE_MAX)) {
        return &m_AnimGroup[m_CurrentRenderState];
    } else {
        return nullptr;
    }
}

//==============================================================================

void new_weapon::GetAmmoState(ammo_priority Priority,
                              int&          AmmoAmount,
                              int&          AmmoMax,
                              int&          AmmoPerClip,
                              int&          AmmoInCurrentClip)
{
    AmmoAmount = m_WeaponAmmo[Priority].m_AmmoAmount;
    AmmoMax = m_WeaponAmmo[Priority].m_AmmoMax;
    AmmoPerClip = m_WeaponAmmo[Priority].m_AmmoPerClip;
    AmmoInCurrentClip = m_WeaponAmmo[Priority].m_AmmoInCurrentClip;
}

//==============================================================================

void new_weapon::SetAmmoState(ammo_priority Priority,
                              int           AmmoAmount,
                              int           AmmoMax,
                              int           AmmoPerClip,
                              int           AmmoInCurrentClip)
{
    m_WeaponAmmo[Priority].m_AmmoAmount = AmmoAmount;
    m_WeaponAmmo[Priority].m_AmmoMax = AmmoMax;
    m_WeaponAmmo[Priority].m_AmmoPerClip = AmmoPerClip;
    m_WeaponAmmo[Priority].m_AmmoInCurrentClip = AmmoInCurrentClip;
}

//==============================================================================

void new_weapon::SetAmmoState2(ammo_priority Priority,
                               int           AmmoAmount,
                               int           AmmoInCurrentClip)
{
    m_WeaponAmmo[Priority].m_AmmoAmount = AmmoAmount;
    m_WeaponAmmo[Priority].m_AmmoInCurrentClip = AmmoInCurrentClip;
}

//===========================================================================
// this function is used to orient a sprite to a collision plane to cover up the ugliness of two Oriented Quads intersecting
/* IJB
void new_weapon::DrawLaserFixupBitmap( Bitmap* pBitmap, float Radius, Colour cColor, collision_mgr::collision& Coll )
{
    Vector3 theNormal   = Coll.Plane.Normal;
    Vector3 CenterPoint = Coll.Point;

    // set fixup texture
    draw_SetTexture( *pBitmap );

    // pull back point a tad along Normal
    CenterPoint = CenterPoint + theNormal * 0.5f;

    //
    // Get the plane-parallel axiis and build the three world-space positions.
    // Because of floating-point error, the ortho vectors can get turned 90 degrees
    // when hitting a wall. Handle that as a special-case by forcing the "up"
    // vector to be the Y axis.
    //
    Vector3 AxisA, AxisB;
    if( theNormal.GetY() < 0.001f )
    {
        Vector3 Dir( 0.0f, 1.0f, 0.0f );
        AxisA = theNormal.Cross( Dir );
        AxisB = theNormal.Cross( AxisA );
        AxisA.Normalize();
        AxisB.Normalize();
    }
    else
    {
        Coll.Plane.GetOrthoVectors( AxisA, AxisB );
    }

    // Scale the axis
    AxisA *= Radius;
    AxisB *= Radius;

    // Vertices
    //  0,5 _____ 1
    //      |\  |
    //      | \ |
    //    4 |__\| 2,3

    // UVs
    //(0,0),(0,0)_____(1,0)
    //           |\  |
    //           | \ |
    //  (0,1)    |__\| (1,1), (1,1)

    // Build the quad points from the axis (Clock-wise)
    Vector3 Pos[6];
    Pos[0] = CenterPoint + AxisA + AxisB;
    Pos[1] = CenterPoint + AxisA - AxisB;
    Pos[2] = CenterPoint - AxisA - AxisB;
    Pos[3] = CenterPoint - AxisA - AxisB;
    Pos[4] = CenterPoint - AxisA + AxisB;
    Pos[5] = CenterPoint + AxisA + AxisB;

    // set up the draw color to the same color as the "laser"
    draw_Color(cColor);

    draw_UV(0.0f, 0.0f);
    draw_Vertex(Pos[0]);

    draw_UV(1.0f, 0.0f);
    draw_Vertex(Pos[1]);

    draw_UV(1.0f, 1.0f);
    draw_Vertex(Pos[2]);

    draw_UV(1.0f, 1.0f);
    draw_Vertex(Pos[3]);

    draw_UV(0.0f, 1.0f);
    draw_Vertex(Pos[4]);

    draw_UV(0.0f, 0.0f);
    draw_Vertex(Pos[5]);
}
*/
//=============================================================================

Bitmap* new_weapon::GetScopeTexture()
{
    if (m_ScopeMesh < 0) {
        return nullptr;
    }

    skin_inst& Skin = m_Skin[RENDER_STATE_PLAYER];
    Geom*      pGeom = Skin.GetGeom();
    if (pGeom == nullptr) {
        return nullptr;
    }

    ResourceHandle<texture> RTex;
    Geom::Mesh&             Mesh = pGeom->meshes[m_ScopeMesh];
    Geom::Submesh&          SubMesh = pGeom->subMeshes[Mesh.iSubMesh];
    Geom::Material&         Mat = pGeom->materials[SubMesh.iMaterial];

    RTex.setName(pGeom->getTextureFilename(Mat.iTexture));
    texture* pTexture = RTex.getPointer();
    if (!pTexture) {
        return nullptr;
    }

    return &pTexture->m_Bitmap;
}

//=============================================================================

void new_weapon::InstallCustomScope()
{
    // It is possible that OnProperty gets called multiple times, or we could
    // be changing the mesh in the editor, so first uninstall whatever we
    // already had
    UninstallCustomScope();

    /* IJB

    // no geometry == no scope texture
    skin_inst& Skin  = m_Skin[ RENDER_STATE_PLAYER ];
    geom*      pGeom = Skin.GetGeom();
    if ( pGeom == nullptr )
        return;

    // is there a mesh called SCOPE?
    m_ScopeMesh = pGeom->GetMeshIndex( "SCOPE" );
    if ( m_ScopeMesh >= 0 )
    {
        CLOG_MESSAGE( LOG_SCOPE_PATCHING, "SCOPE", "Install Ref(%d), Guid(%08X:%08X)", m_ScopeRefCount, GetGuid().GetHigh(), GetGuid().GetLow() );

        // set up the overlay, since we'll need to force that to render last
        m_ScopeLensMesh = pGeom->GetMeshIndex( "SCOPE_LENS" );

        // set up the custom scope texture
        xbitmap* pBitmap = GetScopeTexture();
        assert( pBitmap );
        if ( pBitmap  )
        {
            // patch the scope with our custom texture if no other
            // instance has done it yet
            int VramID = pBitmap->GetVRAMID();
            if( VramID != m_ScopeTextureVramId )
            {
                assert( m_OrigScopeVramId == -1 );
                m_OrigScopeVramId = VramID;
                pBitmap->SetVRAMID( m_ScopeTextureVramId );
                CLOG_MESSAGE( LOG_SCOPE_PATCHING, "SCOPE", "Patched OrigId=%d, ScopeId=%d", m_OrigScopeVramId, m_ScopeTextureVramId );
            }

            // let everyone know we're using the custom texture now
            m_ScopeRefCount++;
        }

        // Force render library to use top mip (we only have 1!) always since the render
        // library thinks the bitmap has mips since we just cheated and set the bitmap vram id
        // to use the pips!
        geom::mesh& Mesh = pGeom->m_pMesh[m_ScopeMesh];
        pGeom->m_pSubMesh[Mesh.iSubMesh].WorldPixelSize = 1000.0f;
    }
        */
}

//=============================================================================

void new_weapon::UninstallCustomScope()
{
    // handle any reference counting or custom scope texture cleanup
    if (m_ScopeMesh != -1) {
        //CLOG_MESSAGE( LOG_SCOPE_PATCHING, "SCOPE", "Uninstall Ref(%d), Guid(%08X:%08X)", m_ScopeRefCount, GetGuid().GetHigh(), GetGuid().GetLow() );

        Bitmap* pBitmap = GetScopeTexture();
        assert(pBitmap);
        if (pBitmap) {
            m_ScopeRefCount--;
            assert(m_ScopeRefCount >= 0);

            // if everyone is done using the custom texture, then restore the
            // geometry back to its original state
            if (m_ScopeRefCount == 0) {
                //CLOG_MESSAGE( LOG_SCOPE_PATCHING, "SCOPE", "Restore OrigId=%d, ScopeId=%d", m_OrigScopeVramId, m_ScopeTextureVramId );
                assert(m_OrigScopeVramId != -1);
                //pBitmap->SetVRAMID( m_OrigScopeVramId );
                m_OrigScopeVramId = -1;
            }
        }

        m_ScopeMesh = -1;
    }
}

#define SCISSOR_LEFT (2048 - (VRAM_FRAME_BUFFER_WIDTH / 2))
#define SCISSOR_TOP (2048 - (VRAM_FRAME_BUFFER_HEIGHT / 2))

void new_weapon::CreateScopeTexture()
{
    // IJB
    return;
}
