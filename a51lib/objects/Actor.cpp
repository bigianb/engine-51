#include "Actor.h"
#include "Object.h"
#include "../Guid.h"

#include "../characters/ActorEffects.h"

#include "Corpse.h"
//#include "Objects\NewWeapon.hpp"
//#include "Objects\Pickup.hpp"
#include "../characters/factions.h"
#include "../zoneManager/ZoneManager.h"
#include "../inventory/Inventory.h"
//#include "../characters/FloorProperties.h"
#include "../loco/LocoAnimController.h"
#include "../loco/Loco.h"
#include "../decals/DecalPackage.h"

#include <cstdlib>

#define ACTOR_DATA_VERSION 1000

const float k_CloakTransitionTime = 1.0f;
const float k_MinTimeBetweenBigStaggers = 0.25f;
const float k_MinTimeBetweenSmallStaggers = 0.75f;
const float k_MinTimeBetweenBigHits = 2.0f;
const float k_TimeBetweenContagionTicks = 1.0f;
const float k_ContagionSpreadDistanceSqr = 1000.0f * 1000.0f;
float       g_SpawnFadeTime = 1.5f;

static const float DISPLACE_PARTICLE_EXTENT = 20.0f;

//=========================================================================
// EXTERNALS
//=========================================================================

extern int         g_Difficulty;
extern const char* DifficultyText[];

extern bool g_bBloodEnabled;
extern bool g_RenderBoneBBoxes;

//=========================================================================
// TWEAKS
//=========================================================================

//static tweak_handle s_TWEAK_ContagionDuration("CONTAGION_Duration");
//static tweak_handle s_MPContagion_TouchDistance("ContagionRadius");
//static tweak_handle s_AlamoTurretDamagePct("PLAYER_AlamoTurretDamagePct");
//static tweak_handle s_ExcavationTurretDamagePct("PLAYER_ExcavationTurretDamagePct");

struct avatar_tweaks
{
    avatar_tweaks();
    float m_NormalingTime;
    float m_MutatingTime;
};

avatar_tweaks::avatar_tweaks()
    : m_NormalingTime(1.0f)
    , m_MutatingTime(1.0f)
{
}

avatar_tweaks g_AvatarTweaks;

//=========================================================================
// DATA
//=========================================================================

actor* actor::m_pFirstActive = nullptr;
int    actor::m_nActive = 0;

//=========================================================================
// Actor class
//=========================================================================

actor::actor(ObjectManager* pObjectManager) : Object(pObjectManager)
    , m_pNextActive(nullptr)
    , m_pPrevActive(nullptr)
    , m_bIsActive(false)
    , m_Faction(FACTION_NONE)
    , m_FriendFlags(0)
    , m_MaxHealth(100.0f)
    , m_bDead(false)
    , m_bCanDie(true)
    , m_DeathType(DEATH_BY_ANIM)
    , m_CorpseGuid(0)
    , m_bWantToSpawn(false)
    , m_SpawnFadeTime(0.0f)
    , m_SpawnNeutralTime(0.0f)
    , m_WeaponsCreated(false)
    , m_CurrentWeaponItem(INVEN_NULL)
    , m_LastAnimPainID(0)
    , m_LastMeleeEventID(0)
    , m_CurrentPainEventID(0)
    , m_TimeSinceLastPain(0.0f)
    , m_SafeFallAltitude(800.0f)
    , m_DeathFallAltitude(1400.0f)
    , m_FellFromAltitude(-(FLT_MAX / 2.0f))
    //, /* a REALLY negative number */
    // m_VoiceID(0)
    , m_PreferedVoiceActor(0)
    , m_PainSfxInterval(0.0f)
    , m_PainSfxDelay(0.0f)
    //  , m_CloakVoiceID(0)
    , m_CloakShieldPainTimer(0.0f)
    , m_CloakState(CLOAKING_OFF)
    , m_CollidedActor(0)
    , m_TimeActorColliding(0.0f)
    , m_TimeObjectColliding(0.0f)
    , m_BloodDecalGroup(0)
    , m_LocoAllocated(false)
    , m_pLoco(nullptr)
    , m_bIgnoreLocoInTransform(false)
    , m_LastTimeStaggered(0.0f)
    , m_TimeSinceLastRender(10.0f)
    , m_LeanAmount(0.0f)
    , m_LeanState(LEAN_NONE)
    , m_FriendlyGlowColor(50, 255, 0, 255)
    , m_EnemyGlowColor(255, 50, 0, 255)
    , m_PulseGlowDelta(0.0f)
    , m_CurrentGlowPulse(1.0f)
    , m_bAllowedToGlow(true)
    , m_ContagionTimer(0.0f)
    , m_ContagionDOTTimer(0.0f)
    , m_CanCloak(false)
    , m_MustCloak(false)
    , m_bIsCrouching(false)
    , m_bIsAirborn(false)
    , m_bIsMutated(false)
    , m_bCanToggleMutation(true)
    , m_bDumbAndFast(false)
    , m_bPrimaryFired(false)
    , m_bEndPrimaryFire(false)
    , m_bContagious(false)
    , m_FireState(0)
    , m_VoteCanStartKick(0)
    , m_VoteAction(0)
    , m_VoteArgument(0)
    , m_Pitch(R_0)
    , m_Yaw(R_0)
    , m_MutagenBurnMode(MBM_NORMAL_CAMPAIGN)
    , m_TargetNetSlot(-1)
    , m_AimOffset(Vector3(0.0f, 0.0f, 0.0f))
    , m_bLockedDoors(false)
    , m_PainThatKilledUs(pObjectManager)
{
    // Initialize the inventory data
    m_Inventory2.Init();

    // Clear the weapon guids
    memset(&m_WeaponGuids, 0, sizeof(m_WeaponGuids));

    // Not inactive.
    ClearInactiveTime();

    // m_FloorProperties.Init(100.0f, 0.5f);

    // m_BigPainTakenTime = (float)x_GetTimeSec();
    m_pEffects = nullptr;

    SetAvatarMutationState(AVATAR_NORMAL);
}

//=========================================================================

actor::~actor()
{
    // Make sure we are removed from the static actor list
    SetIsActive(false);

    if (m_pEffects) {
        //        delete m_pEffects;
        m_pEffects = nullptr;
    }

    if (m_LocoAllocated) {
        delete m_pLoco;
        m_pLoco = nullptr;
    }
    /*
        int c;
        for (c = 0; c < INVEN_NUM_WEAPONS; c++) {
            Object* weaponObject = objectManager->GetObjectByGuid(m_WeaponGuids[c]);
            if (weaponObject) {
                objectManager->DestroyObject(m_WeaponGuids[c]);
            }
        }
        */
}

//=========================================================================

void actor::OnInit()
{
}

//=========================================================================

void actor::OnKill()
{
    //netobj::OnKill();
    //  if (m_CloakVoiceID) {
    //    g_AudioMgr.Release(m_CloakVoiceID,0.0f);
    //      m_CloakVoiceID = 0;
    //  }
}

//=========================================================================

float actor::GetActorCollisionRadius()
{
    return GetLocoPointer()->GetActorCollisionRadius();
}

//=========================================================================

float actor::GetFloorIntensity()
{
    /*
    Colour  Color = GetFloorColor();
    Vector3 Vector(Color.R, Color.G, Color.B);
    return Vector.Length();
    */
    return 0.0f;
}

//=========================================================================

Radian actor::GetSightYaw() const
{
    if (m_pLoco) {
        return m_pLoco->GetSightYaw();
    } else {
        return R_0;
    }
}

//=========================================================================

Vector3 actor::GetPositionWithOffset(eOffsetPos offset)
{
    // SB: Fix meson debris crash in multi-player
    loco* pLoco = GetLocoPointer();
    if (pLoco == nullptr) {
        // Just use npc/player center
        return GetBBox().GetCenter();
    }

    switch (offset) {
    case OFFSET_NONE:
    {
        return GetPosition();
    }
    case OFFSET_CENTER:
    {
        Vector3 halfEyeOffset = pLoco->GetEyeOffset();
        halfEyeOffset.Scale(0.5f);
        return GetPosition() + halfEyeOffset;
    }
    case OFFSET_AIM_AT:
    case OFFSET_EYES:
    {
        return GetPosition() + pLoco->GetEyeOffset();
    }
    case OFFSET_TOP_OF_BBOX:
    {
        return GetPosition(); //+ Vector3(0.0f, pLoco->m_Physics.GetBBox().Max.GetY(), 0.0f);
    }
    default:
    {
        return GetPosition();
    }
    }
}

//=========================================================================

float actor::GetMovementNoiseLevel()
{
    //assert( GetLocoPointer() );

    if (IsMoving()) {
        switch (GetLocoPointer()->GetMoveStyle()) {
        case loco::MOVE_STYLE_PROWL:
            return 2.0f;
            break;
        case loco::MOVE_STYLE_WALK:
            return 4.0f;
            break;
        case loco::MOVE_STYLE_RUN:
            return 10.0f;
            break;
        default:
            break;
        }
    }
    return 0.0f;
}

//=============================================================================

void actor::GetHeadAndRootPosition(Vector3& HeadPos, Vector3& RootPos)
{
    loco* pLoco = GetLocoPointer();
    if (!pLoco) {
        RootPos = GetPosition();
        HeadPos = RootPos + Vector3(0, 200.0f, 0.0f);
    } else {
        RootPos = pLoco->m_Player.GetBonePosition(0);
        HeadPos = pLoco->GetEyePosition(); // Center of eyes is a good spot to aim for
    }
}

//=============================================================================

void actor::ReloadAllWeapons()
{
    /*
    for (int i = 0; i < INVEN_WEAPON_LAST; i++) {
        new_weapon* pWeapon = GetWeaponPtr(inventory2::WeaponIndexToItem(i));

        if (pWeapon) {
            pWeapon->Reload(new_weapon::AMMO_PRIMARY);
            pWeapon->Reload(new_weapon::AMMO_SECONDARY);
        }
    }
        */
}

//=============================================================================

bool actor::AddItemToInventory2(inven_item Item)
{
    // if we're dead, don't add this to our inventory.
    if (IsDead()) {
        return false;
    }

    if (m_Inventory2.CanHoldMore(Item)) {
        m_Inventory2.AddAmount(Item, 1.0f);
        return true;
    }

    return false;
}

//=============================================================================

bool actor::RemoveItemFromInventory2(inven_item Item, bool bRemoveAll)
{
    if (m_Inventory2.HasItem(Item)) {
        float NbrToRemove = 1.0f;
        if (bRemoveAll) {
            NbrToRemove = m_Inventory2.GetAmount(Item);
        }

        m_Inventory2.RemoveAmount(Item, NbrToRemove);
        return true;
    }

    return false;
}

//=========================================================================

void actor::AddAmmoToInventory2(inven_item Item, int Amount)
{
    m_Inventory2.AddAmount(Item, (float)Amount);
    /*
        if (IN_RANGE(INVEN_AMMO_FIRST, Item, INVEN_AMMO_LAST)) {
            new_weapon* pWeapon = GetWeaponPtr(inventory2::WeaponIndexToItem(m_Inventory2.AmmoToWeapon(Item)));
            if (pWeapon) {
                pWeapon->AddAmmoToWeapon(Amount, 0);
            }
        }
            */
}

//=========================================================================

void actor::ClearInventory2()
{
    m_Inventory2.Clear();
    /*
        for (int i = 0; i < INVEN_NUM_WEAPONS; i++) {
            new_weapon* pWeapon = GetWeaponPtr(inventory2::WeaponIndexToItem(i));
            if (pWeapon) {
                pWeapon->ClearAmmo();
            }
        }
            */
}

//=========================================================================

void actor::OnEnumProp(prop_enum& List)
{
    Object::OnEnumProp(List);
    List.PropEnumHeader("Actor", "Actor Info.", 0);

    // Locomotion
    if (m_pLoco) {
        m_pLoco->OnEnumProp(List);
    }

    // Fall damage
    List.PropEnumHeader("FallDamage", "Damage taken from falling a given altitude.", 0);
    List.PropEnumFloat("FallDamage\\SafeAltitude", "Falling from this altitude or less is safe -- no damage.", PROP_TYPE_MUST_ENUM);
    List.PropEnumFloat("FallDamage\\DeathAltitude", "Falling from this altitude or more is guaranteed fatal.", PROP_TYPE_MUST_ENUM);

    // Decals
    List.PropEnumHeader("BloodDecals", "Which blood decals this actor will leave.", 0);
    List.PropEnumExternal("BloodDecals\\Package", "Decal Resource\0decalpkg\0", "Which blood decal package this actor uses.", 0);
    List.PropEnumInt("BloodDecals\\Group", "Within the decal package, which group of bloud this actor uses.", 0);

    // mutation glow effect
    List.PropEnumHeader("MutantVision", "Properties for the mutation vision effect.", 0);
    List.PropEnumBool("MutantVision\\Allowed", "Allow this guy to glow?", 0);
    List.PropEnumColor("MutantVision\\FriendlyColor", "Ambient when you are a friendly in mutant vision. Alpha determines how much glow.", 0);
    List.PropEnumColor("MutantVision\\EnemyColor", "Ambient when you are an enemy in mutant vision. Alpha determines how much glow.", 0);

    List.PropEnumHeader("Effects", "Additional effects added to actors.", 0);
    //List.PropEnumBool    ( "Effects\\UseEffects",  "Turn on effects.",  PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    List.PropEnumBool("Effects\\FlamesOn", "Is Actor flaming?", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    List.PropEnumBool("Effects\\FlashlightOn", "Flashlight effect information", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
    List.PropEnumFloat("Effects\\Lifespan", "How long can actor remain alive?", PROP_TYPE_EXPOSE | PROP_TYPE_DONT_SAVE);
}

//=========================================================================

bool actor::OnProperty(prop_query& rPropQuery)
{
    // Call base class
    if (Object::OnProperty(rPropQuery)) {
        // Initialize zone tracker?
        if (rPropQuery.IsVar("Base\\Position")) {
            //       g_ZoneMgr.InitZoneTracking(*this, m_ZoneTracker);
        }

        // Initialize locomotion?
        if (rPropQuery.IsVar("Base\\Rotation") || rPropQuery.IsVar("Base\\Position")) {
            // Init the loco!
            if (GetLocoPointer()) {
                // Get transform info
                const Matrix4& L2W = GetL2W();
                Vector3        Pos = L2W.GetTranslation();
                Radian         Yaw = L2W.GetRotation().yaw;

                // Update loco
                GetLocoPointer()->SetPosition(Pos);
                GetLocoPointer()->SetYaw(Yaw + R_180); // SB +R_180 is legacy for old anim system
            }
        }

        return true;
    }

    // Skin?
    /*
    skin_inst& SkinInst = GetSkinInst();
    if (SkinInst.OnProperty(rPropQuery)) {
        return true;
    }

    // Animation?
    if (rPropQuery.IsVar("RenderInst\\Anim")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarExternal(m_hAnimGroup.GetName(), RESOURCE_NAME_SIZE);
        } else {
            // Anim changed?
            if (rPropQuery.GetVarExternal()[0]) {
                m_hAnimGroup.SetName(rPropQuery.GetVarExternal());
                if (m_hAnimGroup.GetPointer()) {
                    // Tell loco
                    if (m_pLoco) {
                        m_pLoco->OnInit(SkinInst.GetGeom(), m_hAnimGroup.GetName(), GetGuid());
                    }

                    InitLoco();
                }
            }
        }
        return true;
    }
*/
    // Loco?
    if ((m_pLoco) && (m_pLoco->OnProperty(rPropQuery))) {
        return true;
    }

    // Fall damage
    if (rPropQuery.VarFloat("FallDamage\\SafeAltitude", m_SafeFallAltitude, 0, FLT_MAX)) {
        if (!rPropQuery.IsRead() && (m_SafeFallAltitude > m_DeathFallAltitude)) {
            m_DeathFallAltitude = m_SafeFallAltitude + 1.0f;
        }
        return true;
    }

    if (rPropQuery.VarFloat("FallDamage\\DeathAltitude", m_DeathFallAltitude, 1.0f, FLT_MAX)) {
        if (!rPropQuery.IsRead() && (m_DeathFallAltitude < m_SafeFallAltitude)) {
            m_SafeFallAltitude = m_DeathFallAltitude - 1.0f;
        }
        return true;
    }

    // Decals

    if (rPropQuery.IsVar("BloodDecals\\Package")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarExternal(m_hBloodDecalPackage.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            m_hBloodDecalPackage.setName(rPropQuery.GetVarExternal());
        }

        return true;
    }

    if (rPropQuery.VarInt("BloodDecals\\Group", m_BloodDecalGroup)) {
        return true;
    }

    if (rPropQuery.VarBool("MutantVision\\Allowed", m_bAllowedToGlow)) {
        return true;
    }

    if (rPropQuery.VarColor("MutantVision\\FriendlyColor", m_FriendlyGlowColor)) {
        return true;
    }

    if (rPropQuery.VarColor("MutantVision\\EnemyColor", m_EnemyGlowColor)) {
        return true;
    }

    // Handle additional effects
    /*
    if (rPropQuery.IsBasePath("Effects")) {
        if (rPropQuery.IsVar("Effects\\FlamesOn")) {
            if (rPropQuery.IsRead()) {
                if (GetActorEffects()) {
                    rPropQuery.SetVarBool(GetActorEffects()->IsEffectOn(actor_effects::FX_FLAME));
                } else {
                    rPropQuery.SetVarBool(false);
                }
            } else {
                if (rPropQuery.GetVarBool()) {
                    GetActorEffects(true)->InitEffect(actor_effects::FX_FLAME, this);
                } else {
                    if (GetActorEffects()) {
                        GetActorEffects()->KillEffect(actor_effects::FX_FLAME);
                    }
                }
            }

            return true;
        }

        if (rPropQuery.IsVar("Effects\\FlashlightOn")) {
            if (rPropQuery.IsRead()) {
                if (GetActorEffects()) {
                    rPropQuery.SetVarBool(GetActorEffects()->IsEffectOn(actor_effects::FX_FLASHLIGHT));
                } else {
                    rPropQuery.SetVarBool(false);
                }
            } else {
                if (rPropQuery.GetVarBool()) {
                    GetActorEffects(true)->InitEffect(actor_effects::FX_FLASHLIGHT, this);
                } else {
                    if (GetActorEffects()) {
                        GetActorEffects()->KillEffect(actor_effects::FX_FLASHLIGHT);
                    }
                }
            }

            return true;
        }

        if (rPropQuery.IsVar("Effects\\Lifespan")) {
            if (rPropQuery.IsRead()) {
                if (GetActorEffects()) {
                    rPropQuery.SetVarFloat(GetActorEffects()->GetDeathTimer());
                } else {
                    rPropQuery.SetVarFloat(100000.0f);
                }
            } else {
                if (GetActorEffects()) {
                    GetActorEffects()->SetDeathTimer(rPropQuery.GetVarFloat());
                }
            }

            return true;
        }
    }
*/
    return false;
}

actor_effects* actor::GetActorEffects(bool bCreate)
{
    if (!m_pEffects && bCreate) {
        // create the actor effects
        m_pEffects = new actor_effects;
        m_pEffects->Init();
    }

    return m_pEffects;
}

void actor::OnAdvanceLogic(float DeltaTime)
{
    // update the skin instance so actors can fade based on a super-event
    //   m_SkinInst.OnAdvanceLogic(DeltaTime);

    m_TimeSinceLastPain = std::min(m_TimeSinceLastPain + DeltaTime, 1.0f);

    // Update pain sound effects.
    UpdatePainSfx(DeltaTime);

    // Safe to clear the actor effects?
    /*
    if (m_pEffects) {
        if (!m_pEffects->IsActive()) {
            delete m_pEffects;
            m_pEffects = nullptr;
        }
    }
*/
    // Update our spawn timers
    m_SpawnFadeTime = std::max(0.0f, m_SpawnFadeTime - DeltaTime);
    m_SpawnNeutralTime = std::max(0.0f, m_SpawnNeutralTime - DeltaTime);

    // Special case logic
    if (!m_bDead) {
        OnAliveLogic(DeltaTime);
    } else {
        OnDeathLogic(DeltaTime);
    }

    // contagion logic
    ContagionLogic(DeltaTime);

    // Track inactivity.
    UpdateInactive(DeltaTime);

    // Update avatar mutation
    UpdateAvatarMutation(DeltaTime);

    // Update staggering
    if (IsStaggering()) {
        m_LastTimeStaggered -= DeltaTime;
    }

    // Update rendering
    m_TimeSinceLastRender += DeltaTime;

    //
    // If actor is below some threshold then shut off his logic
    //
    /*
    if (!IsPlayer() && !IsNetGhost() && !objectManager->GetSafeBBox().Intersect(GetPosition())) {
        objectManager->DestroyObject(GetGuid());
        return;
    }

    // get floor information for ambient lighting and footfall sounds
    m_FloorProperties.Update(GetPosition(), DeltaTime);
*/
    UpdateFellFromAltitude();

    // Handled by player and characters
    //WakeUpDoors();

    // Update our current weapon
    UpdateWeapon(DeltaTime);
}

//=============================================================================
// This function should only be used when driving a ghost version of an actor.

void actor::OnAdvanceGhostLogic(float DeltaTime)
{
    // Update animation rate and move style to try match movement?
    loco* pLoco = GetLocoPointer();
    if (!pLoco) {
        return;
    }

    // Loco is controlled by ghost, not animations.
    pLoco->SetGhostMode(true);

    /*
    // If this is a player loco, set the weapon to update the anims...
    if (pLoco->IsKindOf(player_loco::GetRTTI())) {
        // Update player loco
        player_loco* pPlayerLoco = (player_loco*)pLoco;
        pPlayerLoco->SetWeapon(m_CurrentWeaponItem);
        pPlayerLoco->UpdateAnims(DeltaTime, m_bIsAirborn, m_bIsCrouching, m_LeanAmount);
    } else {
        // This is a non-player npc avatar.

        if (IsCrouching()) {
            pLoco->SetMoveStyle(loco::MOVE_STYLE_CROUCHAIM);
        } else {
            // TO DO: Send over current move style
            pLoco->SetMoveStyle(loco::MOVE_STYLE_RUNAIM);
        }
    }
*/
    // Update loco.
    pLoco->OnAdvance(DeltaTime);
}

//=============================================================================

void actor::OnAliveLogic(float DeltaTime)
{
    (void)DeltaTime;
}

//=============================================================================

void actor::OnDeathLogic(float DeltaTime)
{
    if (m_bWantToSpawn) {
        OnSpawn();
    }
}

//=============================================================================

void actor::OnRender()
{
    // Reset timer
    m_TimeSinceLastRender = 0.0f;
    /*
        // Lookup skin geometry
        skin_geom* pSkinGeom = GetSkinInst().GetSkinGeom();
        if (!pSkinGeom) {
            return;
        }

        // Compute LOD mask
        uint64_t LODMask = m_SkinInst.GetLODMask(GetL2W());
        if (m_CloakState == CLOAKING_ON) {
            int CloakVMesh = pSkinGeom->GetVMeshIndex("CLOAK");
            if (CloakVMesh != -1) {
                uint16_t ScreenSize = (uint16_t)eng_GetView()->CalcScreenSize(GetPosition(), m_SkinInst.GetBBox().GetRadius());
                LODMask = pSkinGeom->GetLODMask(1 << CloakVMesh, ScreenSize);
            }
        }

    if (LODMask == 0) {
        return;
    }

    // compute bones
    int            nActiveBones = 0;
    const Matrix4* pMatrices = GetBonesForRender(LODMask, nActiveBones);
    if (!pMatrices) {
        return;
    }
*/
    /*
    // Setup render flags and get ambient color
    Colour   Ambient;
    uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;
    if (g_RenderContext.m_bIsMutated && !g_RenderContext.m_bIsPipRender && m_bAllowedToGlow) {
        Flags |= render::GLOWING;

        Ambient = m_EnemyGlowColor;
        Ambient.A = (uint8_t)(Ambient.A * m_CurrentGlowPulse);
    } else {
        Ambient = GetFloorColor();
    }
*/
    // Render
    /*
    if ((m_CloakState == CLOAKING_ON) ||
        (m_CloakState == CLOAKING_TURNING_OFF)) {
        // TODO: Set up this pipeline so that we can use a lower poly distortion mesh
        // rather than the high poly one
        Radian3 NormalRot(R_0, R_0, R_0);
        if (m_CloakShieldPainTimer > 0.0f) {
            // if our cloaking shield needs to settle down a bit, add some randomness
            // to the normal rotations to show we're in an unstable state.
            Radian MaxRot = m_CloakShieldPainTimer * R_360;
            NormalRot.yaw += x_frand(R_0, MaxRot);
            NormalRot.pitch += x_frand(R_0, MaxRot);
            NormalRot.roll += x_frand(R_0, MaxRot);
        }
        GetSkinInst().RenderDistortion(&GetL2W(),
                                       pMatrices,
                                       nActiveBones,
                                       Flags,
                                       LODMask,
                                       NormalRot,
                                       Colour(128, 128, 128, Ambient.a));
    } else {
        // Are we fading in?
        if (m_SpawnFadeTime > 0.0f) {
            Flags |= render::FADING_ALPHA;
            float Alpha = 1.0f - (m_SpawnFadeTime / g_SpawnFadeTime);
            Alpha = std::min(Alpha, 1.0f);
            Alpha = std::max(Alpha, 0.0f);
            Ambient.a = (uint8_t)(Alpha * 255.0f);
        } else if (m_SkinInst.GetAlpha() != 255) {
            Flags |= render::FADING_ALPHA;
        }

        GetSkinInst().Render(&GetL2W(),
                             pMatrices,
                             nActiveBones,
                             Flags,
                             LODMask,
                             Ambient);
    }
*/
    // render our weapon if alive
    if (IsAlive()) {
        OnRenderWeapon();
    }

    // Render effects
    //if (m_pEffects) {
    //    m_pEffects->Render(this);
    // }
}

//=============================================================================

void actor::OnRenderShadowCast(uint64_t ProjMask)
{
    // Reset timer
    m_TimeSinceLastRender = 0;

    // don't render a shadow if we're cloaked
    if (m_CloakState == CLOAKING_ON) {
        return;
    }
    /*    // Lookup skin geometry
        skin_geom* pSkinGeom = GetSkinInst().GetSkinGeom();
        if (!pSkinGeom) {
            return;
        }

        // Compute LOD mask for the normal render
        uint64_t LODMask = GetSkinInst().GetLODMask(GetL2W());
        if (LODMask == 0) {
            return;
        }

        // Compute LOD mask for the shadow render (by forcing 0 for the screen size
        // we are sure to get the lowest LOD)
        uint64_t ShadLODMask = GetSkinInst().GetLODMask(0);
        if (ShadLODMask == 0) {
            return;
        }

        // Setup render flags
        uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;

        // compute bones
        int            nActiveBones = 0;
        const Matrix4* pMatrices = GetBonesForRender(LODMask | ShadLODMask, nActiveBones);

        // Render
        GetSkinInst().RenderShadowCast(&GetL2W(),
                                       pMatrices,
                                       nActiveBones,
                                       Flags,
                                       ShadLODMask,
                                       ProjMask);
    */
}

//=============================================================================

void actor::OnRenderTransparent()
{
    // if (m_pEffects) {
    //     m_pEffects->RenderTransparent(this);
    // }
}

//=============================================================================

void actor::OnRenderWeapon()
{
    //render weapon
    // new_weapon* pWeapon = GetCurrentWeaponPtr();
    // if (!pWeapon) {
    //     return;
    // }

    // // Always 3rd person for actor
    // pWeapon->SetRenderState(new_weapon::RENDER_STATE_NPC);

    // pWeapon->RenderWeapon(false, GetFloorColor(), (m_CloakState == CLOAKING_ON));
}

//==============================================================================

bool ALLOW_COLCHECK_BBOX = true;

void actor::OnColCheck()
{
    // Skip if dead - so that player does not get in the way of corpse ragdoll in MP
    if (IsDead()) {
        return;
    }

    // If not loco pointer then just bail.
    if (!GetLocoPointer()) {
        return;
    }
    /*
        // Get the Object that's trying to collide with us.
        Object* pObject = objectManager->GetObjectByGuid(g_CollisionMgr.GetMovingObjGuid());

        //
        // If this is a ray of some sort then give it special consideration.
        //
        if ((g_CollisionMgr.GetDynamicPrimitive() == PRIMITIVE_DYNAMIC_RAY) ||
            (g_CollisionMgr.GetDynamicPrimitive() == PRIMITIVE_DYNAMIC_LOS)) {
            // Do trivial reject with tighter BBox
            float T;

            // inflate the collision BBox a bit to encompass all of the head and extremeties
            BBox ColBBox = GetColBBox();
            ColBBox.Inflate(50.0f, 50.0f, 50.0f);

            const collision_mgr::dynamic_ray& Ray = g_CollisionMgr.GetDynamicRay();
            if (ColBBox.Intersect(T, Ray.Start, Ray.End) == false) {
                return;
            }

            bool bProvideBBoxAsCollision = false;

            //
            // Check if this is a bullet.
            //
            if (pObject && pObject->IsKindOf(base_projectile::GetRTTI())) {
                base_projectile* pProjectile = (base_projectile*)pObject;
                guid             OwnerGuid = pProjectile->GetOwnerID();
                Object*          pOwner = objectManager->GetObjectByGuid(OwnerGuid);

                // Check if we are being shot at.
                if (pOwner) {
                    if (IsEnemyFaction(GetFactionForGuid(OwnerGuid))) {
                        loco* pLoco = GetLocoPointer();
                        BBox  Box = pLoco->m_Physics.GetBBox();
                        Box.Translate(GetPosition());
                        Box.Inflate(50, 50, 50);

                        if (Box.Intersect(T, Ray.Start, Ray.End)) {
                            if (T >= 0 && T <= 1) {
                                // Handle shot at stuff here.
                                OnBeingShotAt(pProjectile->GetType(), pProjectile->GetOwnerID());
                            }
                        }
                    }
                }

                // Check if shooter is a character
                if (pOwner && pOwner->IsKindOf(character::GetRTTI())) {
                    bProvideBBoxAsCollision = true;
                }
            }

            //
            // Check if caller is a character
            //
            if (pObject && pObject->IsKindOf(character::GetRTTI())) {
                bProvideBBoxAsCollision = true;
            }

            //
            // Should we just send our BBox?
            //
            if (bProvideBBoxAsCollision && ALLOW_COLCHECK_BBOX) {
                g_CollisionMgr.StartApply(GetGuid());
                BBox BBox = GetLocoPointer()->m_Physics.GetBBox();
                BBox.Transform(GetL2W());
                g_CollisionMgr.ApplyAABBox(BBox, Object::MAT_TYPE_FLESH, (Bone::HIT_LOCATION_TORSO) << 24);
                g_CollisionMgr.EndApply();
                return;
            }
        }

        //
        // Apply individual bone locations if requesting high poly
        // else feed in BBox
        //
        {
            //  If hit locations have been added then let's use those
            if (g_CollisionMgr.IsUsingHighPoly()) {
                // Get the juicy info about the mesh and the skeleton
                Geom* pGeom = GetSkinInst().GetGeom();
                //assert( pGeom );
                if (!pGeom) {
                    return;
                }

                // Get loco
                loco* pLoco = GetLocoPointer();
                if (!pLoco) {
                    return;
                }

                // Must have an animation
                if (!pLoco->IsAnimLoaded()) {
                    return;
                }

                // Lookup # of bones since LOD system may not be using all of them
                int nBones = std::min(pLoco->GetNActiveBones(), (int)pGeom->m_nBones);

                // Lookup animation matrices
                const Matrix4* pMatrices = pLoco->ComputeL2W();
                if (!pMatrices) {
                    return;
                }

                int i;
                int nStoredCollisions = g_CollisionMgr.m_nCollisions;

                // Loop over all bones
                //assert( nBones <= pGeom->m_nBones);

                // look for headshots first
                g_CollisionMgr.StartApply(GetGuid());
                for (i = 0; i < nBones; i++) {
                    // Lookup bone and skip if no hit location specified
                    const Geom::bone& Bone = pGeom->m_pBone[i];

                    // unknown, skip it
                    if (Bone.HitLocation == Bone::HIT_LOCATION_UNKNOWN) {
                        continue;
                    }

                    // not a head bone, skip it
                    if (Bone.HitLocation != Bone::HIT_LOCATION_HEAD) {
                        continue;
                    }

                    // Lookup bone info
                    const BBox&    LocalBBox = Bone.BBox;
                    const Matrix4& L2W = pMatrices[i];

                    // Apply oriented BBox
                    g_CollisionMgr.ApplyOOBBox(LocalBBox,
                                               L2W,
                                               Object::MAT_TYPE_FLESH,
                                               (Bone.HitLocation << 24) | i); // Type | Index
                }
                g_CollisionMgr.EndApply();

                // hit a head bone, this is a headshot get out
                if (g_CollisionMgr.m_nCollisions != nStoredCollisions) {
                    // x_printfxy(2,2, "*** HEAD SHOT ****" );

                    return;
                }

                g_CollisionMgr.StartApply(GetGuid());
                for (i = 0; i < nBones; i++) {
                    // Lookup bone and skip if no hit location specified
                    const Geom::bone& Bone = pGeom->m_pBone[i];

                    if (Bone.HitLocation == Bone::HIT_LOCATION_UNKNOWN) {
                        continue;
                    }

                    // head bone, skip it
                    if (Bone.HitLocation == Bone::HIT_LOCATION_HEAD) {
                        continue;
                    }

                    // Lookup bone info
                    const BBox&    LocalBBox = Bone.BBox;
                    const Matrix4& L2W = pMatrices[i];

                    // Apply oriented BBox
                    g_CollisionMgr.ApplyOOBBox(LocalBBox,
                                               L2W,
                                               Object::MAT_TYPE_FLESH,
                                               (Bone.HitLocation << 24) | i); // Type | Index
                }
                g_CollisionMgr.EndApply();
            } else {
                // Let physics do it's stuff
                GetLocoPointer()->m_Physics.ApplyCollision();
            }
        }
            */
}

//=============================================================================

void actor::OnMove(const Vector3& NewPos)
{
    // Skip?
    if (GetAttrBits() & Object::ATTR_DESTROY) {
        return;
    }

    // Tell loco (this needs to happen event if the position hasn't changed so
    // that ghost locos work correctly)
    if (GetLocoPointer()) {
        GetLocoPointer()->SetPosition(NewPos);
    }

    // Call base class
    Object::OnMove(NewPos);

    // move our weapon
    MoveWeapon(true);

    // Update zone tracker
    UpdateZoneTrack();
}

//=========================================================================

void actor::OnTransform(const Matrix4& L2W)
{
    // Skip?
    if (GetAttrBits() & Object::ATTR_DESTROY) {
        return;
    }

    // Call base class
    Object::OnTransform(L2W);

    // Setup loco?
    if ((GetLocoPointer()) && (!m_bIgnoreLocoInTransform)) {
        // Get info
        Vector3 Pos = L2W.GetTranslation();
        Radian  Yaw = L2W.GetRotation().yaw;

        // Update loco
        GetLocoPointer()->SetPosition(Pos);
        GetLocoPointer()->SetYaw(Yaw + R_180); // SB +R_180 is legacy for old anim system
    }

    // move our weapon
    MoveWeapon(true);

    // Update zone tracker
    UpdateZoneTrack();
}

//===========================================================================

BBox actor::GetColBBox()
{
    /*
    if (m_pLoco) {
        // Start with physics BBox
        BBox BBox = m_pLoco->m_Physics.GetBBox();

        // Take lean into account so leaning ghosts/players can be hit in MP
        float LeanDist = abs(GetLeanAmount() * 100.0f);
        BBox.Inflate(LeanDist, 0.0f, LeanDist);

        // Convert into world space
        BBox.Transform(GetL2W());
        return BBox;
    } else {
     */
    return Object::GetColBBox();
    //}
}

//===========================================================================

BBox actor::GetLocalBBox() const
{
    /*
    if (m_pLoco) {
        // Start with physics BBox
        BBox BBox = m_pLoco->m_Physics.GetBBox();

        // Take lean into account so leaning ghosts/players can be hit in MP
        float LeanDist = abs(GetLeanAmount() * 100.0f);
        BBox.Inflate(LeanDist, 0.0f, LeanDist);

        return BBox;
    } else {
     */
    BBox BBox;
    BBox.min.set(-50.0f, 0.0f, -50.0f);
    BBox.max.set(50.0f, 200.0f, 50.0f);
    return BBox;
    // }
}

//===========================================================================
// HEALTH FUNCTIONS
//===========================================================================

bool actor::AddHealth(float DeltaHealth)
{
    // If player can not die, leave his health otherwise it screws with the AI targeting!
    if ((DeltaHealth < 0.0f) && (!m_bCanDie)) {
        return false;
    }

    float OldHealth = m_Health.GetHealth();

    m_Health.Add(DeltaHealth, IsCharacter());

    return (true);
}

//===========================================================================
// FACTION/FRIEND FUNCTIONS
//===========================================================================

factions actor::GetFactionForGuid(guid Guid) const
{
    /*
    Object* tempObject = objectManager->GetObjectByGuid(Guid);

    if (tempObject && tempObject->IsKindOf(actor::GetRTTI())) {
        actor& actorObject = actor::GetSafeType(*tempObject);
        return actorObject.GetFaction();
    } else if (tempObject && tempObject->IsKindOf(turret::GetRTTI())) {
        turret& turretObject = turret::GetSafeType(*tempObject);
        return turretObject.GetFaction();
    }
*/
    return FACTION_NOT_SET;
}

//===========================================================================
// INVENTORY FUNCTIONS
//===========================================================================

void actor::InitInventory()
{
}

//===========================================================================

void actor::ReInitInventory()
{
}

//=============================================================================

bool actor::HasItemInInventory2(inven_item Item)
{
    return m_Inventory2.HasItem(Item);
}

//=============================================================================

bool actor::OnPickup(pickup&)
{
    return (false);
}

//===========================================================================
// KILL/DAMAGE FUNCTIONS
//===========================================================================

bool actor::OnPlayFullBodyImpactAnim(loco::anim_type AnimType, float BlendTime, uint32_t Flags)
{
    // Lookup loco
    loco* pLoco = GetLocoPointer();
    if (!pLoco) {
        return false;
    }

    // Try play anim
    return pLoco->PlayAnim(AnimType, BlendTime, Flags);
}

//===========================================================================

void actor::OnDeath()
{
    m_Health.Dead();
    m_bDead = true;
    m_bWantToSpawn = false; // Clear it for now just to be safe.
    m_LeanAmount = 0.0f;

    // Stop any weapon looping sfx
    /*
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if (pWeapon) {
        pWeapon->ReleaseAudio();
        pWeapon->KillAllMuzzleFX();
        pWeapon->EndPrimaryFire();
        pWeapon->EndAltFire();
    }
*/
    // Create the corpse
    bool bCreateCorpse = true;
    // Do not create player corpse in single player games!
    if ((GetType() == Object::TYPE_PLAYER)) {
        bCreateCorpse = false;
    }

    // Create corpse?
    if (bCreateCorpse) {
        CreateCorpse();
    }

    // Turn off collision when you die...
    SetAttrBits((GetAttrBits() & ~((uint32_t)0 | ATTR_COLLIDABLE | ATTR_LIVING)));
}

//===========================================================================

void actor::OnSpawn()
{

    m_Health.Reset();
    m_bDead = false;
    m_bWantToSpawn = false;

    m_bIsCrouching = false;
    m_bIsAirborn = false;

    if (m_CorpseGuid) {
        corpse* pCorpse = (corpse*)objectManager->GetObjectByGuid(m_CorpseGuid);
        if (pCorpse) {
            pCorpse->SetPermanent(false);
        }
        m_CorpseGuid = 0; // NET HACK
    }

    // Bring loco back alive
    if (GetLocoPointer()) {
        GetLocoPointer()->SetDead(false);
    }

    // This assumes that the player is about 2.5 meters tall.  Then is about 2
    // meters around.  The BBox is center around the eyes of the player.
    m_ZoneTracker.SetBBox(BBox(Vector3(-100, -200, -100), Vector3(100, 50, 100)));

    // Turn on collision when you spawn.
    SetAttrBits(GetAttrBits() | ATTR_COLLIDABLE | ATTR_LIVING);

    // Spawning is NOT a sign of activity.
    m_RecentPosition = GetPosition();

    // Shut down any actor effects
    if (GetActorEffects()) {
        GetActorEffects()->Kill();
    }
}

//==============================================================================

corpse* actor::CreateCorpseObject(bool BodyFades)
{
    // create a dead body Object
    m_CorpseGuid = objectManager->CreateObject(corpse::GetObjectType());
    assert(m_CorpseGuid != 0);

    // initialize the dead body Object
    corpse* pCorpse = (corpse*)objectManager->GetObjectByGuid(m_CorpseGuid);
    assert(pCorpse);
    pCorpse->Initialize(*this, BodyFades, GetActorEffects());

    // Because we've passed the actor effects over to the dead body, it will
    // be responsible for destroying it. So set our pointer back to nullptr.
    m_pEffects = nullptr;

    return pCorpse;
}

//==============================================================================

void actor::CreateCorpse()
{
    // Must have loco
    if (!m_pLoco) {
        return;
    }

    // Skip if already created a corpse
    if (m_CorpseGuid) {
        return;
    }

    //  LOG_MESSAGE( "actor::CreateCorpse", "DEATH_BY_ANIM" );

    // Should body fade?
    /*
    bool BodyFades = true;
    if (IsKindOf(player::GetRTTI())) {
        BodyFades = false;
    }
*/
    // Create dead body
    /*
    corpse* pCorpse = CreateCorpseObject(BodyFades);
    if (!pCorpse) {
        return;
    }
*/
    // Only do pain to the corpse if the m_PainThatKilledUs
    // is recent enough to be valid.
    /*
    if (m_TimeSinceLastPain < 0.12f) {
        pCorpse->OnPain(m_PainThatKilledUs);
    } else {
        // Apply pain from net?
        if (pCorpse) {
            m_CorpseDeathPain.Apply(*pCorpse);
        }
    }
        */
}

//=============================================================================
//  GetHitLocation
//
//      returns hit location based on the last collision check.
//
//=============================================================================
Geom::Bone::HitLocation actor::GetHitLocation(const pain& Pain)
{
    // Be sure this is a direct hit and we have collision info
    if (Pain.HasCollision() == false) {
        return Geom::Bone::HIT_LOCATION_UNKNOWN_WRONG_GUID;
    }
    /*
        const collision_mgr::collision& Coll = Pain.GetCollision();
        if (Coll.ObjectHitGuid != GetGuid()) {
            return Bone::HIT_LOCATION_UNKNOWN_WRONG_GUID;
        }

        int Key = (Coll.PrimitiveKey);
        Key >>= 24;

        Bone::HitLocation Loc = (Bone::HitLocation)Key;

        if (Loc < Bone::HIT_LOCATION_START) {
            return Bone::HIT_LOCATION_UNKNOWN;
        }

        if (Loc >= Bone::HIT_LOCATION_COUNT) {
            return Bone::HIT_LOCATION_UNKNOWN;
        }

        return Loc;
        */
    return Geom::Bone::HIT_LOCATION_UNKNOWN_WRONG_GUID;
}

//=============================================================================
//  GetHitLocationName
//
//  Converts hit location enum into a string
//
//=============================================================================

const char* actor::GetHitLocationName(Geom::Bone::HitLocation Loc)
{
    switch (Loc) {
    case Geom::Bone::HIT_LOCATION_HEAD:
        return "HIT_LOCATION_HEAD";
    case Geom::Bone::HIT_LOCATION_SHOULDER_LEFT:
        return "HIT_LOCATION_SHOULDER_LEFT";
    case Geom::Bone::HIT_LOCATION_SHOULDER_RIGHT:
        return "HIT_LOCATION_SHOULDER_RIGHT";
    case Geom::Bone::HIT_LOCATION_TORSO:
        return "HIT_LOCATION_TORSO";
    case Geom::Bone::HIT_LOCATION_LEGS:
        return "HIT_LOCATION_LEGS";
    case Geom::Bone::HIT_LOCATION_COUNT:
        return "HIT_LOCATION_COUNT";
    case Geom::Bone::HIT_LOCATION_UNKNOWN:
        return "HIT_LOCATION_UNKNOWN";
    case Geom::Bone::HIT_LOCATION_UNKNOWN_WRONG_GUID:
        return "HIT_LOCATION_UNKNOWN_WRONG_GUID";
    default:
        return "INVALID_HIT_LOCATION";
    }
}

//=============================================================================

void actor::PlayImpactAnim(const pain& Pain, eHitType hitType)
{
    Geom::Bone::HitLocation hitLocation = GetHitLocation(Pain);

    //  LOG_MESSAGE( "actor::PlayImpactAnim", "Hit location: %s", GetHitLocationName(hitLocation) );
#if 0
    Radian FaceYaw = GetLocoPointer()->m_Player.GetFacingYaw();
    Radian PainYaw = Pain.GetDirection().GetYaw();
    Radian PainDeltaYaw = x_MinAngleDiff(FaceYaw, PainYaw);

    bool bFromFront = false;
    if (abs(PainDeltaYaw) > R_90) {
        bFromFront = true;
    }

    if (IsAlive()) {
        if (HandleSpecialImpactAnim(hitType)) {
            //did special handling for impact anim
            return;
        }
    }

    if (hitType == HITTYPE_PLAYER_MELEE_1) //light hit
    {
        OnPlayFullBodyImpactAnim(loco::ANIM_DAMAGE_PLAYER_MELEE_0, // AnimType
                                 DEFAULT_BLEND_TIME,               // Blend time
                                 loco::ANIM_FLAG_INTERRUPT_BLEND | // Flags
                                     loco::ANIM_FLAG_TURN_OFF_AIMER |
                                     loco::ANIM_FLAG_RESTART_IF_SAME_ANIM);
        //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_PLAYER_MELEE_0");
    } else if (hitType == HITTYPE_LIGHT) //light hit
    {
        // first figure out the additive impact anim to play
        loco::anim_type ImpactAnimType = loco::ANIM_NULL;
        switch (hitLocation) {
        case Bone::HIT_LOCATION_HEAD:
            if (bFromFront) {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_HEAD_FRONT;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_HEAD_FRONT");
            } else {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_HEAD_BACK;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_HEAD_BACK");
            }
            break;

        case Bone::HIT_LOCATION_SHOULDER_RIGHT:
            if (bFromFront) {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_SHOULDER_RIGHT_FRONT;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_SHOULDER_RIGHT_FRONT");
            } else {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_SHOULDER_RIGHT_BACK;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_SHOULDER_RIGHT_BACK");
            }
            break;

        case Bone::HIT_LOCATION_SHOULDER_LEFT:
            if (bFromFront) {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_SHOULDER_LEFT_FRONT;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_SHOULDER_LEFT_FRONT");
            } else {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_SHOULDER_LEFT_BACK;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_SHOULDER_LEFT_BACK");
            }
            break;

        case Bone::HIT_LOCATION_TORSO:
        case Bone::HIT_LOCATION_LEGS:
            if (bFromFront) {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_TORSO_FRONT;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_TORSO_FRONT");
            } else {
                ImpactAnimType = loco::ANIM_ADD_IMPACT_TORSO_BACK;
                //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_ADD_IMPACT_TORSO_BACK");
            }
            break;
        default:
            break;
        }

        if (ImpactAnimType != loco::ANIM_NULL) {
            m_LastTimeStaggered = k_MinTimeBetweenSmallStaggers;

            GetLocoPointer()->PlayAdditiveAnim(ImpactAnimType,               // AnimType
                                               0.1f,                         // BlendInTime
                                               0.1f,                         // BlendOutTime
                                               ANIM_FLAG_IMPACT_CONTROLLER); // Flags
        }
    } else if (hitType == HITTYPE_IDLE && !IgnoreFullBodyFlinches()) {
        loco::anim_type ImpactAnimType = loco::ANIM_NULL;
        if (bFromFront) {
            ImpactAnimType = loco::ANIM_PAIN_IDLE_FRONT;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_PAIN_IDLE_FRONT");
        } else {
            ImpactAnimType = loco::ANIM_PAIN_IDLE_BACK;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_PAIN_IDLE_BACK");
        }

        //lets also apply a random yaw change
        // messing with someone's yaw can do nasty things like screw up anim playback.
        // removing this part.
        /*        if (x_irand(0,1))
                    GetLocoPointer()->SetYawFacingTarget( GetLocoPointer()->GetYaw() + x_frand(R_20, R_40), R_360 );
                else
                    GetLocoPointer()->SetYawFacingTarget( GetLocoPointer()->GetYaw() - x_frand(R_20, R_40), R_360 );
        */
        if (OnPlayFullBodyImpactAnim(ImpactAnimType,                   // AnimType
                                     DEFAULT_BLEND_TIME,               // Blend time
                                     loco::ANIM_FLAG_INTERRUPT_BLEND | // Flags
                                         loco::ANIM_FLAG_TURN_OFF_AIMER |
                                         loco::ANIM_FLAG_RESTART_IF_SAME_ANIM)) {
            m_BigPainTakenTime = (float)x_GetTimeSec();
            m_LastTimeStaggered = k_MinTimeBetweenBigStaggers;
        }
    } else if (!IgnoreFullBodyFlinches()) // hard hit
    {
        // next figure out our stagger anim, if any
        loco::anim_type StagerAnimType = loco::ANIM_NULL;
/*
        Object* originObject = objectManager->GetObjectByGuid(Pain.GetOriginGuid());
        if (originObject &&
            originObject->IsKindOf(genericNPC::GetRTTI()) &&
            HasAnim(loco::ANIM_DAMAGE_PARASITE)) {
            StagerAnimType = loco::ANIM_DAMAGE_PARASITE;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_PARASITE");
        } else if ((PainDeltaYaw >= R_45) && (PainDeltaYaw <= R_135)) {
            StagerAnimType = loco::ANIM_DAMAGE_STEP_RIGHT;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_STEP_RIGHT");
        } else if ((PainDeltaYaw <= -R_45) && (PainDeltaYaw >= -R_135)) {
            StagerAnimType = loco::ANIM_DAMAGE_STEP_LEFT;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_STEP_LEFT+");
        } else if ((PainDeltaYaw >= -R_45) && (PainDeltaYaw <= R_45)) {
            StagerAnimType = loco::ANIM_DAMAGE_STEP_FORWARD;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_STEP_FORWARD");
        } else {
            StagerAnimType = loco::ANIM_DAMAGE_STEP_BACK;
            //  LOG_MESSAGE("actor::PlayImpactAnim","ANIM_DAMAGE_STEP_BACK");
        }
*/
        if (StagerAnimType != loco::ANIM_NULL) {
            //lets also apply a random yaw change
            // messing with someone's yaw can do nasty things like screw up anim playback.
            // removing this part.
            /*            if (x_irand(0,1))
                            GetLocoPointer()->SetYawFacingTarget( GetLocoPointer()->GetYaw() + x_frand(R_20, R_40), R_360 );
                        else
                            GetLocoPointer()->SetYawFacingTarget( GetLocoPointer()->GetYaw() - x_frand(R_20, R_40), R_360 );*/

            if (OnPlayFullBodyImpactAnim(StagerAnimType,                   // AnimType
                                         DEFAULT_BLEND_TIME,               // Blend time
                                         loco::ANIM_FLAG_INTERRUPT_BLEND | // Flags
                                             loco::ANIM_FLAG_TURN_OFF_AIMER |
                                             loco::ANIM_FLAG_RESTART_IF_SAME_ANIM)) {
                m_BigPainTakenTime = (float)x_GetTimeSec();
                m_LastTimeStaggered = k_MinTimeBetweenBigStaggers;
            }
        }
    }
#endif
}

//==============================================================================
// Locomotion functions
//==============================================================================

void actor::InitLoco()
{
    // Setup loco transform?
    if (m_pLoco) {
        // Get transform info
        const Matrix4& L2W = GetL2W();
        Vector3        Pos = L2W.GetTranslation();
        Radian         Yaw = L2W.GetRotation().yaw;

        // Update loco
        m_pLoco->SetPosition(Pos);
        m_pLoco->SetYaw(Yaw + R_180); // SB +R_180 is legacy for old anim system
        m_pLoco->SetArriveDistSqr(1.0f);
        m_pLoco->SetUseAimMoveStyles(true);
    }
}

//=============================================================================

bool actor::IsRunning()
{
    // Get loco
    loco* pLoco = GetLocoPointer();
    if (!pLoco) {
        return false;
    }

    // Must be running
    if (pLoco->GetState() != loco::STATE_MOVE) {
        return false;
    }

    // In a run style?
    switch (pLoco->GetMoveStyle()) {
    case loco::MOVE_STYLE_RUN:
    case loco::MOVE_STYLE_RUNAIM:
    case loco::MOVE_STYLE_CHARGE:
        return true;
    default:
        return false;
    }
}

//=============================================================================

bool actor::IsMoving()
{
    return !GetLocoPointer()->IsAtDestination();
}

//=============================================================================

bool actor::IsStaggering()
{
    return (m_LastTimeStaggered > 0.0f);
}

//=============================================================================

float actor::GetPitch()
{
    return m_Pitch;
}

//=============================================================================

float actor::GetYaw()
{
    return m_Yaw;
}

//=============================================================================

void actor::SetPitch(Radian Pitch)
{
    if (m_pLoco) {
        // Update loco
        m_pLoco->SetPitch(Pitch);

        // Update weapon
        MoveWeapon(true);
    }

    m_Pitch = Pitch;
}

//=============================================================================

void actor::SetYaw(Radian Yaw)
{
    if (m_pLoco) {
        // Update loco
        m_pLoco->SetYaw(Yaw);

        // Update weapon
        MoveWeapon(true);
    }

    m_Yaw = Yaw;
}

//==============================================================================
// ANIMATION FUNCTIONS
//=============================================================================

bool actor::HasAnim(loco::anim_type animType)
{
    return (GetLocoPointer() && (GetLocoPointer()->GetAnimIndex(animType) >= 0));
}

//=============================================================================

bool actor::IsAnimInPackage(const char* pAnimGroup, const char* pName)
{
    if (!GetLocoPointer() ||
        strlen(pName) <= 0) {
        return false;
    }

    AnimGroup::handle hAnimGroup;
    int               AnimIndex = -1;
    if (strlen(pAnimGroup) > 0) {
        hAnimGroup.setName(pAnimGroup);
        if (!hAnimGroup.getPointer()) {
            return false;
        }
        AnimIndex = hAnimGroup.getPointer()->GetAnimIndex(pName);
    } else {
        return false;
    }
    if (AnimIndex == -1) {
        return false;
    }
    return true;
}

//==============================================================================

bool actor::IsPlayingFullBodyLipSync()
{
    // Loco?
    loco* pLoco = GetLocoPointer();
    if (pLoco) {
        // Is full body lip sync anim playing?
        //loco_lip_sync_controller& LipSyncCont = pLoco->GetLipSyncController();
        //if ((LipSyncCont.IsPlaying()) && (LipSyncCont.IsFullBody())) {
        //    return true;
        // }
    }

    return false;
}

//==============================================================================

bool actor::IsPlayingFullBodyCinema()
{
    loco* pLoco = GetLocoPointer();
    if (pLoco) {
        // Is full body lip sync anim playing?
        /*
        loco_lip_sync_controller& LipSyncCont = pLoco->GetLipSyncController();
        if ((LipSyncCont.IsPlaying()) &&
            (LipSyncCont.IsFullBody()) &&
            (LipSyncCont.GetAnimFlags() & loco::ANIM_FLAG_CINEMA)) {
            return true;
        }
            */
    }
    return false;
}

//==============================================================================

bool actor::IsPlayingLipSync()
{
    // Loco?
    loco* pLoco = GetLocoPointer();
    if (pLoco) {
        /*
        // Is lip sync anim playing?
        loco_lip_sync_controller& LipSyncCont = pLoco->GetLipSyncController();
        if (LipSyncCont.IsPlaying()) {
            return true;
        }
            */
    }

    return false;
}

//==============================================================================

bool actor::IsPlayingCinema()
{
    loco* pLoco = GetLocoPointer();
    if (pLoco) {
        // Is lip sync cinema anim playing?
        /*
        loco_lip_sync_controller& LipSyncCont = pLoco->GetLipSyncController();
        if ((LipSyncCont.IsPlaying()) &&
            (LipSyncCont.GetAnimFlags() & loco::ANIM_FLAG_CINEMA)) {
            return true;
        }
            */
    }

    return false;
}

//==============================================================================
// ZONE FUNCTIONS
//==============================================================================

void actor::Teleport(const Vector3& Position, bool DoBlend, bool DoEffect)
{
    /*
    LOG_MESSAGE( "actor::Teleport", "Before:%.3f,%.3f,%.3f",
                                    GetPosition().GetX(),
                                    GetPosition().GetY(),
                                    GetPosition().GetZ() );
    */
#if 0
    if (!DoBlend) {
        m_WayPointFlags |= WAYPOINT_TELEPORT;
        m_WayPointTimeOut = 0;
    }

    if (DoEffect) {
        float   Best = FLT_MAX;
        Object* pBest = nullptr;
        slot_id Slot = objectManager->GetFirst(TYPE_TELEPORTER);
        while (Slot != SLOT_NULL) {
            Object* pObject = objectManager->GetObjectBySlot(Slot);
            assert(pObject);
            assert(pObject->GetType() == TYPE_TELEPORTER);
            Vector3 Gap = pObject->GetPosition() - GetPosition();
            float   L2 = Gap.LengthSquared();
            if (L2 < Best) {
                Best = L2;
                pBest = pObject;
            }
            Slot = objectManager->GetNext(Slot);
        }
        if (pBest) {
            teleporter* pTeleporter = (teleporter*)pBest;
            pTeleporter->PlayTeleportOut();
        }

        m_WayPointFlags |= WAYPOINT_TELEPORT_FX;
        m_WayPointTimeOut = 0;
    }

    // Move into position.
    OnMove(Position);

    if (DoEffect) {
        slot_id Slot = objectManager->GetFirst(TYPE_TELEPORTER);
        while (Slot != SLOT_NULL) {
            Object* pObject = objectManager->GetObjectBySlot(Slot);
            assert(pObject);
            assert(pObject->GetType() == TYPE_TELEPORTER);
            Vector3 Gap = pObject->GetPosition() - Position;
            if (Gap.LengthSquared() < 250.0f) {
                teleporter* pTeleporter = (teleporter*)pObject;
                pTeleporter->PlayTeleportIn();
                break;
            }
            Slot = objectManager->GetNext(Slot);
        }
    }

    // Fire a ray down from the given position.  Use the zone information from
    // the play surface hit by the ray.

    // HACK HACK HACK
    //
    // As fate would have it, the player often likes to spawn directly
    // over the seam between two floor pieces.  There is a slight chance
    // that a ray fired directly down from such a location may MISS all
    // of the floor pieces due to edge conditions in the math.
    //
    // Hack solution: Do not fire a ray straight down.  We move the start
    // position 5cm on X, and we angle the shot slightly as well.  This is
    // designed to minimize the chance of such math surprises.
    //
    // HACK HACK HACK

    Vector3 Start = Position + Vector3(5.0f, 10.0f, 0.0f); // HACK - Offset start postion
    Vector3 Ray(10.0f, -500.0f, 11.0f);                    // HACK - Don't fire straight down.

    g_CollisionMgr.RaySetup(GetGuid(), Start, Start + Ray);
    g_CollisionMgr.SetMaxCollisions(5);
    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES,
                                   Object::ATTR_COLLIDABLE,
                                   (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE |
                                                         Object::ATTR_LIVING));

    // Loop over collisions
    for (int i = 0; i < g_CollisionMgr.m_nCollisions; i++) {
        // Found a surface below the player.
        guid    HitGuid = g_CollisionMgr.m_Collisions[i].ObjectHitGuid;
        Object* pObject = objectManager->GetObjectByGuid(HitGuid);
        assert(pObject);

        // Skip invisible walls because they can span more than one zone
        if (pObject->GetType() == Object::TYPE_INVISIBLE_WALL_OBJ) {
            continue;
        }

        // Copy zones from Object (position has already been updated)
        SetZone1(pObject->GetZone1());
        SetZone2(pObject->GetZone2());

        // Update the zone tracker
        g_ZoneMgr.InitZoneTracking(*pObject, m_ZoneTracker);

        /*
        LOG_MESSAGE( "actor::Teleport", "Zone1:%d - Zone2:%d",
                    pObject->GetZone1(), pObject->GetZone2() );
        */

        // Exit for loop
        break;
    }

    /*
    LOG_MESSAGE( "actor::Teleport", "After:%.3f,%.3f,%.3f",
                                    GetPosition().GetX(),
                                    GetPosition().GetY(),
                                    GetPosition().GetZ() );
    */
#endif
}

//==============================================================================

void actor::Teleport(const Vector3& Position,
                     Radian         Pitch,
                     Radian         Yaw,
                     bool           DoBlend,
                     bool           DoEffect)
{
    Teleport(Position, DoBlend, DoEffect);
    SetPitch(Pitch);
    SetYaw(Yaw);
}

//==============================================================================

void actor::InitZoneTracking()
{
    // Update the zone tracker
    //g_ZoneMgr.InitZoneTracking(*this, m_ZoneTracker);
}

//==============================================================================

void actor::UpdateZoneTrack()
{
    // Update tracker
    //g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, GetPosition());
}

//==============================================================================

void actor::UpdateZone(uint8_t Zone)
{
    SetZone1(Zone);
    SetZone2(0);
}

//==============================================================================
// Contagion FUNCTIONS
//==============================================================================

void actor::ContagionLogic(float DeltaTime)
{
    if (!IsContagious()) {
        // Nothing to see here.  Move along.
        return;
    }

    // Update the timers.
    m_ContagionDOTTimer -= DeltaTime;
    m_ContagionTimer -= DeltaTime;

    // End of contagion?
    if (IsDead() || (m_ContagionTimer <= 0.0f)) {
        if (IsAlive() && IsCharacter()) {
            pain Pain(objectManager);
            Pain.Setup("CONTAGION_FINAL", 0, GetPosition());
            Pain.SetDirectHitGuid(GetGuid());
            Pain.ApplyToObject(GetGuid());
        }

        m_bContagious = false;
        if (m_pEffects) {
            //m_pEffects->KillEffect(actor_effects::FX_CONTAIGON);
        }

        // Show's over, people.  Go on home.
        return;
    }

    // Time for contagion DOT?
    if (m_ContagionDOTTimer <= 0.0f) {
        m_ContagionDOTTimer += k_TimeBetweenContagionTicks;
        ContagionDOT();
    }
}

//==============================================================================

void actor::ContagionDOT()
{
    //  LOG_MESSAGE( "actor::ContagionDOT", "No effect." );
}

//==============================================================================

void actor::InitContagion(int Origin)
{
    (void)Origin;

    //  LOG_MESSAGE( "actor::InitContagion", "Origin:%d", Origin );
    /*
        float Duration = s_TWEAK_ContagionDuration.GetF32();
        if (Duration == 0.0f) {
            Duration = 7.5f;
        }

        GetActorEffects(true)->InitEffect(actor_effects::FX_CONTAIGON, this);

        m_bContagious = true;
        m_ContagionTimer = Duration;
        m_ContagionDOTTimer = k_TimeBetweenContagionTicks;
        */
}

//==============================================================================

void actor::KillMPContagion()
{
    //  LOG_MESSAGE( "actor::KillMPContagion", "" );

    assert(!m_bContagious);
}

//=============================================================================

void actor::RenderContagion()
{
    if (!m_bContagious) {
        return;
    }
}

//=============================================================================
// WEAPON FUNCTIONS
//=============================================================================

new_weapon* actor::GetCurrentWeaponPtr()
{
    int Index = inventory2::ItemToWeaponIndex(m_CurrentWeaponItem);
    return (new_weapon*)objectManager->GetObjectByGuid(m_WeaponGuids[Index]);
}

//==============================================================================

new_weapon* actor::GetWeaponPtr(inven_item WeaponItem)
{
    int Index = inventory2::ItemToWeaponIndex(WeaponItem);
    return (new_weapon*)objectManager->GetObjectByGuid(m_WeaponGuids[Index]);
}

//==============================================================================

void actor::MoveWeapon(bool UpdateWeaponRenderState)
{
    //move the weapon using the prop points position... from the character animiation player

    //
    // move our 3rd person weapon?
    // This is a necessary test to that we don't mess up the player's weapon
    // pullback calculation by putting the weapon in an avatar's hands. So
    // we move the weapon if either we aren't a player, or we're playing split
    // screen.
    //
    const bool bIsPlayer = (GetType() == Object::TYPE_PLAYER);

    const bool bIsSplitScreen = false;
    const bool bShouldMove = (!bIsPlayer || (bIsPlayer && bIsSplitScreen));

    if (!bShouldMove || !m_pLoco) {
        return;
    }
    /*
        new_weapon* pWeapon = GetCurrentWeaponPtr();

        if (!pWeapon || !GetLocoPointer()->IsAnimLoaded()) {
            return;
        }

        // Update weapons NPC (3rd person) state, but keep original state afterwards so split screen players work
        new_weapon::render_state RenderState = pWeapon->GetRenderState();
        pWeapon->SetRenderState(new_weapon::RENDER_STATE_NPC);
        pWeapon->OnTransform(GetLocoPointer()->GetWeaponL2W());
        pWeapon->SetZone1(GetZone1());
        pWeapon->SetRenderState(RenderState);

        if (UpdateWeaponRenderState) {
            pWeapon->SetRenderState((new_weapon::render_state)GetWeaponRenderState());
        }
    */
}

//=============================================================================

void actor::UpdateWeapon(float DeltaTime)
{
    if (!IsAlive()) {
        return;
    }
    /*
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon && (pWeapon->GetRenderState() != new_weapon::RENDER_STATE_PLAYER)) {
            //advance currently held weapon's logic.
            pWeapon->OnAdvanceLogic(DeltaTime);
        }
            */
}

//=============================================================================

bool actor::EquipWeapon2(inven_item WeaponItem)
{
    assert((WeaponItem >= INVEN_WEAPON_FIRST) && (WeaponItem <= INVEN_WEAPON_LAST));
    /*
        new_weapon* pNewWeapon = GetWeaponPtr(WeaponItem);
        if (pNewWeapon) {
            pNewWeapon->InitWeapon(GetPosition(), new_weapon::RENDER_STATE_NPC, GetGuid());
            m_CurrentWeaponItem = WeaponItem;
            return true;
        }
    */
    return false;
}

//=============================================================================

void actor::UnequipCurrentWeapon()
{
    /*
        new_weapon* pWeapon = GetCurrentWeaponPtr();
        if (pWeapon) {
            pWeapon->SetVisible(false);
        }
            */
    m_CurrentWeaponItem = INVEN_NULL;
}

//=============================================================================

int actor::GetWeaponRenderState()
{
    return 0; //new_weapon::RENDER_STATE_NPC;
}

//=============================================================================

bool actor::IsFlashlightOn()
{
    return false; //(m_pEffects && m_pEffects->IsEffectOn(actor_effects::FX_FLASHLIGHT));
}

//=============================================================================

bool actor::IsBloodEnabled() const
{
    return true; //g_bBloodEnabled;
}

//=============================================================================

void actor::CreateDamageEffects(const pain& Pain, bool bDoLargeEffects, bool bDoDebris)
{
    (void)bDoDebris;

    // Skip?
    if (!IsBloodEnabled()) {
        return;
    }

    // Create Large Blood Splats
    if (bDoLargeEffects) {
        CreateSplatDecalOnWall(Pain);

        if (IsDead()) {
            CreateSplatDecalOnGround();
        }
    }
    /*
        // If pain is a direct hit then use pain position for blood effect
        if (Pain.IsDirectHit()) {
            // Create blood impact if blood decals are assigned
            const decal_package* pBloodDecalPackage = m_hBloodDecalPackage.getPointer();
            if (pBloodDecalPackage) {
                // Create blood based on pain type and use color of assigned blood decal group
                particle_emitter::CreateOnPainEffect(Pain,
                                                     0.0f,
                                                     particle_emitter::UNINITIALIZED_PARTICLE,
                                                     pBloodDecalPackage->GetGroupColor(m_BloodDecalGroup));
            }
        }
     */
}

//===========================================================================

void actor::CreateSplatDecalOnGround()
{
    // Skip?
    if (!IsBloodEnabled()) {
        return;
    }
    /*
        decal_package* pPackage = m_hBloodDecalPackage.GetPointer();
        if (!pPackage) {
            return;
        }

        if ((m_BloodDecalGroup < 0) || (m_BloodDecalGroup >= pPackage->GetNGroups())) {
            return;
        }

        int nDecalDefs = pPackage->GetNDecalDefs(m_BloodDecalGroup);
        if (nDecalDefs == 0) {
            return;
        }
            */
    /*
        // choose a random decal to paste
        int               DecalIndex = (nDecalDefs == 1) ? 0 : x_rand() % (nDecalDefs - 1);
        decal_definition& DecalDef = pPackage->GetDecalDef(m_BloodDecalGroup, DecalIndex);

        // create a ray that is biased towards the ground
        Radian3 BloodOrient;
        BloodOrient.pitch = x_frand(R_80, R_100);
        BloodOrient.yaw = x_frand(R_0, R_360);
        BloodOrient.roll = R_0;

        Vector3 RayStart = GetPosition();
        RayStart.GetY() += 10.0f;

        Vector3 RayEnd(0.0f, 0.0f, 1.0f);
        RayEnd.Rotate(BloodOrient);
        RayEnd = 500.0f * RayEnd;
        RayEnd += RayStart;

        // generate the splat decal
        g_DecalMgr.CreateDecalFromRayCast(DecalDef,
                                          RayStart,
                                          RayEnd);
                                          */
}

//===========================================================================

void actor::CreateSplatDecalOnWall(const pain& Pain)
{
    // Skip?
    if (!IsBloodEnabled()) {
        return;
    }
    /*
        decal_package* pPackage = m_hBloodDecalPackage.GetPointer();
        if (!pPackage) {
            return;
        }

        if ((m_BloodDecalGroup < 0) || (m_BloodDecalGroup >= pPackage->GetNGroups())) {
            return;
        }

        int nDecalDefs = pPackage->GetNDecalDefs(m_BloodDecalGroup);
        if (nDecalDefs == 0) {
            return;
        }

        // choose a random decal to paste
        int               DecalIndex = (nDecalDefs == 1) ? 0 : x_rand() % (nDecalDefs - 1);
        decal_definition& DecalDef = pPackage->GetDecalDef(m_BloodDecalGroup, DecalIndex);

        // create a ray that is biased towards the ground
        Radian3 BloodOrient;
        Pain.GetDirection().GetPitchYaw(BloodOrient.Pitch, BloodOrient.Yaw);
        BloodOrient.pitch += x_frand(R_0, R_45);
        BloodOrient.yaw += x_frand(-R_30, R_30);
        BloodOrient.roll = R_0;

        Vector3 RayEnd(0.0f, 0.0f, 1.0f);
        RayEnd.Rotate(BloodOrient);
        RayEnd = 500.0f * RayEnd;
        RayEnd += Pain.GetImpactPoint();

        // generate the splat decal
        g_DecalMgr.CreateDecalFromRayCast(DecalDef,
                                          Pain.GetImpactPoint(),
                                          RayEnd);
                                          */
}

//=============================================================================

loco::anim_type actor::GetDeathAnim(const pain& painThatKilledUs)
{
    return loco::ANIM_NULL;
#if 0 
    general_hit_location   GenLocation = HIT_INVALID;
    death_motion_direction MoveDir = DEATH_MOVE_INVALID;
    Bone::HitLocation      hitLocation = GetHitLocation(painThatKilledUs);

    //  LOG_MESSAGE( "actor::GetDeathAnim", "Hit location:%s", GetHitLocationName(hitLocation) );

    // How high did we get hit?
    switch (hitLocation) {
    case Bone::HIT_LOCATION_HEAD:
        GenLocation = HIT_HIGH;
        break;

    case Bone::HIT_LOCATION_SHOULDER_RIGHT:
    case Bone::HIT_LOCATION_SHOULDER_LEFT:
    case Bone::HIT_LOCATION_TORSO:
        GenLocation = HIT_MIDDLE;
        break;

    case Bone::HIT_LOCATION_LEGS:
        GenLocation = HIT_LOW;
        break;

    default:
        GenLocation = HIT_MIDDLE;
        break;
    }

    // Since middle is so common do a random chance of high or low
    if (GenLocation == HIT_MIDDLE) {
        int I = irand(0, 100);
        if (I < 50) {
            if (I < 25) {
                GenLocation = HIT_LOW;
            } else {
                GenLocation = HIT_HIGH;
            }
        }
    }

    // Get the facing and force direction
    Radian FaceYaw = GetLocoPointer()->m_Player.GetFacingYaw();
    Radian ForceYaw = painThatKilledUs.GetForceDirection().GetYaw();
    Radian DeltaYaw = MinAngleDiff(FaceYaw, ForceYaw);
    if (fabs(DeltaYaw) > R_90) {
        MoveDir = DEATH_MOVE_BACK;
    } else {
        MoveDir = DEATH_MOVE_FORWARD;
    }

    // What animation do we play?
    loco::anim_type  AnimType = loco::ANIM_NULL;
    eHitType         HitType = GetHitType(painThatKilledUs);
    loco::move_style MoveStyle = GetLocoPointer()->GetMoveStyle();

    if ((HitType == HITTYPE_LIGHT) &&
        ((MoveStyle == loco::MOVE_STYLE_CROUCH) || (MoveStyle == loco::MOVE_STYLE_CROUCHAIM)) &&
        (GetLocoPointer()->GetAnimIndex(loco::ANIM_DEATH_CROUCH) != -1)) {
        //light hits while crouching cause us to play crouching death
        AnimType = loco::ANIM_DEATH_CROUCH;
    } else {
        switch (GenLocation) {
        case HIT_HIGH:
        {
            switch (MoveDir) {
            case DEATH_MOVE_FORWARD:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_BACK_HIGH;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_BACK_HIGH;
                    break;
                }
                break;
            case DEATH_MOVE_BACK:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_FRONT_HIGH;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_FRONT_HIGH;
                    break;
                }
                break;
            default:
                break;
            }
        } break;

        case HIT_MIDDLE:
        {
            switch (MoveDir) {
            case DEATH_MOVE_FORWARD:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_BACK_MED;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_BACK_MED;
                    break;
                }
                break;
            case DEATH_MOVE_BACK:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_FRONT_MED;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_FRONT_MED;
                    break;
                }
                break;
            default:
                break;
            }
        } break;

        case HIT_LOW:
        {
            switch (MoveDir) {
            case DEATH_MOVE_FORWARD:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_BACK_LOW;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_BACK_LOW;
                    break;
                }
                break;
            case DEATH_MOVE_BACK:
                switch (HitType) {
                case HITTYPE_HARD:
                    AnimType = loco::ANIM_DEATH_HARD_SHOT_IN_FRONT_LOW;
                    break;
                case HITTYPE_LIGHT:
                    AnimType = loco::ANIM_DEATH_LIGHT_SHOT_IN_FRONT_LOW;
                    break;
                }
                break;
            default:
                break;
            }
        } break;
        }
    }
    return AnimType;
#endif
}

//=============================================================================
// ANIMATION RELATED FUNCTIONS
//=============================================================================

// Takes care of all anim events
void actor::SendAnimEvents()
{
    int i;

    // Any loco?
    if (!GetLocoPointer()) {
        return;
    }
    /*
        g_EventMgr.HandleSuperEvents(GetLocoPointer()->m_Player, this);
        g_EventMgr.HandleSuperEvents(GetLocoPointer()->m_Player, GetLocoPointer()->GetMaskController(), this);
        g_EventMgr.HandleSuperEvents(GetLocoPointer()->m_Player, GetLocoPointer()->GetAdditiveController(ANIM_FLAG_IMPACT_CONTROLLER), this);
        g_EventMgr.HandleSuperEvents(GetLocoPointer()->m_Player, GetLocoPointer()->GetAdditiveController(ANIM_FLAG_SHOOT_CONTROLLER), this);

        // Check main anim controller events
        for (i = 0; i < GetLocoPointer()->m_Player.GetNEvents(); i++) {
            // Send this event?
            if (GetLocoPointer()->m_Player.IsEventActive(i)) {
                // Lookup event and world position
                const anim_event& Event = GetLocoPointer()->m_Player.GetEvent(i);
                Vector3           Pos = GetLocoPointer()->m_Player.GetEventPosition(i);

                // Call event handler
                OnAnimEvent(Event, Pos);
            }
        }

        // OLD EVENT SYSTEM:
        // TO DO - Remove when all events are updated to super events!
        // Check reload shoot anim controller events
        for (i = 0; i < GetLocoPointer()->GetMaskController().GetNEvents(); i++) {
            // Send this event?
            if (GetLocoPointer()->GetMaskController().IsEventActive(i)) {
                // Lookup event
                const anim_event& Event = GetLocoPointer()->GetMaskController().GetEvent(i);

                // Compute world pos
                const Matrix4& BoneM = GetLocoPointer()->m_Player.GetBoneL2W(Event.GetInt(anim_event::INT_IDX_BONE));
                Vector3        Pos = BoneM * Event.GetPoint(anim_event::POINT_IDX_OFFSET);

                // Call event handler
                OnAnimEvent(Event, Pos);
            }
        }

        // OLD EVENT SYSTEM:
        // TO DO - Remove when all events are updated to super events!
        // Check reload shoot anim controller events
        for (i = 0; i < GetLocoPointer()->GetAdditiveController(ANIM_FLAG_IMPACT_CONTROLLER).GetNEvents(); i++) {
            // Send this event?
            if (GetLocoPointer()->GetAdditiveController(ANIM_FLAG_IMPACT_CONTROLLER).IsEventActive(i)) {
                // Lookup event
                const anim_event& Event = GetLocoPointer()->GetAdditiveController(ANIM_FLAG_IMPACT_CONTROLLER).GetEvent(i);

                // Compute world pos
                const Matrix4& BoneM = GetLocoPointer()->m_Player.GetBoneL2W(Event.GetInt(anim_event::INT_IDX_BONE));
                Vector3        Pos = BoneM * Event.GetPoint(anim_event::POINT_IDX_OFFSET);

                // Call event handler
                OnAnimEvent(Event, Pos);
            }
        }

        // OLD EVENT SYSTEM:
        // TO DO - Remove when all events are updated to super events!
        // Check reload shoot anim controller events
        for (i = 0; i < GetLocoPointer()->GetAdditiveController(ANIM_FLAG_SHOOT_CONTROLLER).GetNEvents(); i++) {
            // Send this event?
            if (GetLocoPointer()->GetAdditiveController(ANIM_FLAG_SHOOT_CONTROLLER).IsEventActive(i)) {
                // Lookup event
                const anim_event& Event = GetLocoPointer()->GetAdditiveController(ANIM_FLAG_SHOOT_CONTROLLER).GetEvent(i);

                // Compute world pos
                const Matrix4& BoneM = GetLocoPointer()->m_Player.GetBoneL2W(Event.GetInt(anim_event::INT_IDX_BONE));
                Vector3        Pos = BoneM * Event.GetPoint(anim_event::POINT_IDX_OFFSET);

                // Call event handler
                OnAnimEvent(Event, Pos);
            }
        }
    */
}

//=============================================================================

bool actor::OnAnimEvent(const anim_event& Event, const Vector3& WorldPos)
{
    // TO DO - Move weapon firing into here?

    return false;
}

//=============================================================================

void actor::PlayFlinch(const pain& Pain)
{
    // get the hit type.
    /*
        eHitType HitType = GetHitType(Pain);

        bool    isIdle = (GetLocoPointer()->GetState() == loco::STATE_IDLE);
        Object* painSource = objectManager->GetObjectByGuid(Pain.GetOriginGuid());

        // If playing a full body lip sync cinema, then ignore pain
        if (IsPlayingFullBodyCinema()) {
            HitType = HITTYPE_LIGHT;
        }
        // first player melee always plays special stun
        else if (Pain.GetHitType() == 6 && HasAnim(loco::ANIM_DAMAGE_PLAYER_MELEE_0)) {
            //  LOG_MESSAGE( "actor::PlayFlinch", "HITTYPE_PLAYER_MELEE_1" );
            HitType = HITTYPE_PLAYER_MELEE_1;
        }
        // damage from a friendly player is always light
        else if (painSource &&
                 painSource->IsKindOf(player::GetRTTI()) &&
                 IsFriendlyFaction(GetFactionForGuid(Pain.GetOriginGuid()))) {
            HitType = HITTYPE_LIGHT;
        }
        // lights can become full body idles if we are idle.
        else if (HitType == HITTYPE_LIGHT) {
            // convert up to a full body hit
            if (isIdle &&
                x_GetTimeSec() - m_BigPainTakenTime >= k_MinTimeBetweenBigHits &&
                HasAnim(loco::ANIM_PAIN_IDLE_FRONT)) {
                //  LOG_MESSAGE( "actor::PlayFlinch", "HITTYPE_LIGHT->HITTYPE_IDLE" );
                HitType = HITTYPE_IDLE;
            } else {
                //  LOG_MESSAGE( "actor::PlayFlinch", "HITTYPE_LIGHT" );
            }
        } else {
            // downgrade to light if timer to low.
            if ((x_GetTimeSec() - m_BigPainTakenTime < k_MinTimeBetweenBigHits) ||
                IsDead()) {
                //  LOG_MESSAGE("actor::PlayFlinch","HITTYPE_BIG->HITTYPE_LIGHT");
                HitType = HITTYPE_LIGHT;
            } else {
                //  LOG_MESSAGE("actor::PlayFlinch","HITTYPE_BIG");
            }
        }

        // place to put any special hit_type override code.
        HitType = OverrideFlinchType(HitType);

        // play an impact anim.
        if (!IgnoreFlinches()) {
            PlayImpactAnim(Pain, HitType);
        } else {
            //  LOG_MESSAGE("actor::PlayFlinch","IgnoreFlinches()==true");
        }
            */
}

//=============================================================================

void actor::PlayPainSfx()
{
    /*
    // Trigger impact sfx immediately
    if (IsDead()) {
        // Play after this delay
        m_PainSfxDelay = x_frand(0.2f, 0.25f);
    } else {
        // Only play grunt if interval has finished
        if (m_PainSfxInterval == 0.0f) {
            // Play after this delay
            m_PainSfxDelay = x_frand(0.1f, 0.2f);

            // Wait this time before playing another grunt
            m_PainSfxInterval = x_frand(0.2f, 0.4f);
        }
    }
        */
}

//=============================================================================

void actor::UpdatePainSfx(float DeltaTime)
{
    // Update interval
    m_PainSfxInterval = std::max(m_PainSfxInterval - DeltaTime, 0.0f);
    /*
        // Waiting to play a pain?
        if (m_PainSfxDelay > 0.0f) {
            // Update delay
            m_PainSfxDelay = std::max(m_PainSfxDelay - DeltaTime, 0.0f);

            // Trigger the sfx?
            if (m_PainSfxDelay == 0.0f) {
                // Stop current pain grunt
                g_AudioMgr.Release(m_VoiceID, 0.0f);

                // Lookup audio strings
                const char* pPrefix = GameMgr.GetSkinAudioPrefix(m_Net.Skin, m_PreferedVoiceActor, m_bIsMutated);
                const char* pType = IsDead() ? "DEATH" : "PAINGRUNT";

                // Is this player local?
                if (g_NetworkMgr.IsLocal(net_GetSlot())) {
                    // 2d sound
                    m_VoiceID = g_AudioMgr.Play(xfs("%s_%s", pPrefix, pType));
                } else {
                    // 3d sound
                    m_VoiceID = g_AudioMgr.PlayVolumeClipped(xfs("%s_%s", pPrefix, pType), GetPosition(), GetZone1(), true);
                }
            }
        }
            */
}

//=============================================================================

bool actor::TakeDamage(const pain& Pain)
{
    /*
    assert(Pain.ComputeDamageAndForceCalled());

    bool bIsPlayer = IsKindOf(player::GetRTTI());

    // Invincible?
    if (m_bCanDie == false) {
        // if this isn't a player, just don't let them take damage at all
        if (!bIsPlayer) {
            return (false);
        }
    }

    float Damage = Pain.GetDamage();

    Damage = ModifyDamageByDifficulty(Damage);

    if (InTurret()) {
        Damage = ModifyDamageByTurret(Damage);
    }

    // Update health.
    m_Health.Sub(Damage, !bIsPlayer);

    // Dead?
    if (m_Health.GetHealth() <= 0.0f) {
        // if we are a player and are invulnerable and we are about to die... refill health
        if ((m_bCanDie == false) && bIsPlayer) {
            m_Health.Add(GetMaxHealth());
        } else {
            m_TimeSinceLastPain = 0.0f;
            m_PainThatKilledUs = Pain;
            OnDeath();        // You are DEAD!
            Damage = 1000.0f; // "Spike" the pain for the net kill.
        }
    }
*/
    // Damage taken
    return true;
}

//==============================================================================

actor::eHitType actor::GetHitType(const pain& Pain)
{
    // Get HitType as specified in the tweak tables
    int HT = Pain.GetHitType();
    switch (HT) {
    case 0:
        return HITTYPE_LIGHT;
    case 1:
        return HITTYPE_HARD;
    case 3:
        return HITTYPE_HARD;
    case 4:
        return HITTYPE_HARD;
    case 2:
    {
        /*
        if (Pain.GetOriginGuid()) {
            Object* pOriginOfPain = objectManager->GetObjectByGuid(Pain.GetOriginGuid());
            if (pOriginOfPain) {
                tweak_handle DistTweak("DistForCharacterHitType2");
                Vector3      vToTarget = pOriginOfPain->GetPosition() - GetPosition();
                if (vToTarget.LengthSquared() <= x_sqr(DistTweak.GetF32())) {
                    return character::HITTYPE_HARD;
                }
            }
        }
            */
        return HITTYPE_LIGHT;
    }
    default:
        return HITTYPE_LIGHT;
    }
}

//=============================================================================

void actor::UpdateCloak(float DeltaTime)
{
    // are we allowed to cloak?
    if (!m_CanCloak) {
        Decloak();
        return;
    }

    // are we forced to be cloaked all the time?
    if (m_MustCloak) {
        Cloak();
        return;
    }

    // update the shield status for a cloaked guy
    if ((m_CloakState == CLOAKING_ON) && (m_CloakShieldPainTimer > 0.0f)) {
        m_CloakShieldPainTimer -= DeltaTime;
        if (m_CloakShieldPainTimer < 0.0f) {
            m_CloakShieldPainTimer = 0.0f;
        }
    }

    // update the transition state
    if ((m_CloakState == CLOAKING_TURNING_ON) ||
        (m_CloakState == CLOAKING_TURNING_OFF)) {
        m_CloakTransitionTime += DeltaTime;
        if (m_CloakTransitionTime > k_CloakTransitionTime) {
            if (m_CloakState == CLOAKING_TURNING_ON) {
                m_CloakState = CLOAKING_ON;
            } else {
                m_CloakState = CLOAKING_OFF;
            }
        } else {
            // add a light flash to draw the user's attention
            // g_LightMgr.AddFadingLight(GetPosition(), Colour(57, 230, 246), 200.0f, 3.0f * m_CloakTransitionTime / k_CloakTransitionTime, 0.1f);
        }
    }
}

//=============================================================================

void actor::Cloak()
{
    // if we are already cloaked, then nothing to do
    if ((m_CloakState == CLOAKING_ON) ||
        (m_CloakState == CLOAKING_TURNING_ON)) {
        return;
    }

    // we need to transition into cloaked
    m_CloakTransitionTime = 0.0f;
    m_CloakState = CLOAKING_TURNING_ON;
    /*
        // play a cloaking sound effect
        g_AudioMgr.Play("Cloak_On", GetPosition(), GetZone1(), true);
        m_CloakVoiceID = g_AudioMgr.Play("Cloak_Loop");

        // kick off the particle effect
        actor_effects* pActorEffects = GetActorEffects(true);
        if (pActorEffects) {
            pActorEffects->InitEffect(actor_effects::FX_CLOAK, this);
        }
            */
}

//=============================================================================

void actor::Decloak()
{
    // if we are already uncloaked, then nothing to do
    if ((m_CloakState == CLOAKING_OFF) ||
        (m_CloakState == CLOAKING_TURNING_OFF)) {
        return;
    }

    // we need to transition into not cloaked
    m_CloakTransitionTime = 0.0f;
    m_CloakState = CLOAKING_TURNING_OFF;
    /*
        // play a decloaking sound effect
        g_AudioMgr.Play("Cloak_Off", GetPosition(), GetZone1(), true);
        g_AudioMgr.Release(m_CloakVoiceID, 0.0f);
        m_CloakVoiceID = 0;

        // kick off the particle effect
        actor_effects* pActorEffects = GetActorEffects(true);
        if (pActorEffects) {
            pActorEffects->InitEffect(actor_effects::FX_DECLOAK, this);
        }
            */
}

//=============================================================================

void actor::WakeUpDoors()
{
    bool bLockedDoors = false;

    // Dead men wake no doors.
    if (IsDead()) {
        return;
    }

    //
    // Doors.
    //
    /*
        // Scan the box for objects.
        for (int ObjectSlot = objectManager->GetFirst(Object::TYPE_DOOR);
             ObjectSlot != SLOT_NULL;
             ObjectSlot = objectManager->GetNext(ObjectSlot)) {
            Object* pObj = objectManager->GetObjectBySlot(ObjectSlot);
            if (pObj) {
                door* pDoor = (door*)pObj;

                BBox BBox = GetBBox();

                if (!pDoor->UsesProxBox()) {
                    BBox.Inflate(350.0f, 50.0f, 350.0f);
                }

                if (pDoor->GetDoorBBox().Intersect(BBox)) {
                    uint32_t CircuitBits = pDoor->GetCircuit().GetTeamBits();
                    // Wake up the door if it's open so it
                    // doesn't close on somebody's head.
                    if (pDoor->GetState() != door::CLOSED) {
                        pDoor->WakeUp();
                        pDoor->WakeUp();
                    }
                }
            }

            //
            // Forcefields.
            //
            {
                // Scan the box for objects.
                for (int ObjectSlot = objectManager->GetFirst(Object::TYPE_FORCE_FIELD);
                     ObjectSlot != SLOT_NULL;
                     ObjectSlot = objectManager->GetNext(ObjectSlot)) {
                    Object* pObj = objectManager->GetObjectBySlot(ObjectSlot);

                    if (pObj) {
                        force_field* pField = (force_field*)pObj;

                        BBox BBox = GetBBox();

                        BBox.Inflate(350.0f, 50.0f, 350.0f);

                        if (pField->GetBBox().Intersect(BBox)) {
                            pField->Open();
                        }
                    }
                }
            }
    */
    m_bLockedDoors = bLockedDoors;
}

//=============================================================================

void actor::Push(const Vector3& PushVector)
{
    /*
    GetLocoPointer()->m_Physics.SetPosition(GetPosition());
    GetLocoPointer()->m_Physics.Push(PushVector);
    OnMove(GetLocoPointer()->m_Physics.GetPosition());
    */
}

//========================================================================

float actor::GetCollisionHeight()
{
    return 0.0f;
    /*
    if (m_pLoco) {
        return m_pLoco->m_Physics.GetColHeight();
    } else {
        return 0.0f;
    }
        */
}

//========================================================================

float actor::GetCollisionRadius()
{
    return 0.0f;
    /*
    if (m_pLoco) {
        return m_pLoco->m_Physics.GetColRadius();
    } else {
        return 0.0f;
    }
        */
}

//=============================================================================

void actor::SetIsActive(bool bIsActive)
{
    if (m_bIsActive == bIsActive) {
        return;
    }

    if (bIsActive) {
        m_bIsActive = true;
        m_nActive++;

        m_pNextActive = m_pFirstActive;
        m_pPrevActive = nullptr;
        if (m_pFirstActive) {
            m_pFirstActive->m_pPrevActive = this;
        }
        m_pFirstActive = this;
    } else {
        m_bIsActive = false;
        m_nActive--;

        if (m_pNextActive) {
            m_pNextActive->m_pPrevActive = m_pPrevActive;
        }
        if (m_pPrevActive) {
            m_pPrevActive->m_pNextActive = m_pNextActive;
        }
        if (m_pFirstActive == this) {
            m_pFirstActive = m_pNextActive;
        }

        m_pNextActive = nullptr;
        m_pPrevActive = nullptr;
    }
}

//=============================================================================

//=============================================================================

void actor::EnumAttachPoints(std::string& String) const
{
    String = "BaseObject~";

    int i;
    /*
        if (m_pLoco) {
            int nBones = m_pLoco->m_Player.GetNBones();

            for (i = 0; i < nBones; i++) {
                const anim_bone& Bone = m_pLoco->m_Player.GetBone(i);

                String += Bone.Name;
                String += "~";
            }
        }

        for (i = 0; String[i]; i++) {
            if (String[i] == '~') {
                String[i] = 0;
            }
        }
            */
}

//=============================================================================

int actor::GetAttachPointIDByName(const char* pName) const
{
    /*
    if (stricmp(pName, "BaseObject") == 0) {
        return 0;
    }

    if (m_pLoco) {
        int nBones = m_pLoco->m_Player.GetNBones();
        int i;
        for (i = 0; i < nBones; i++) {
            const anim_bone& Bone = m_pLoco->m_Player.GetBone(i);

            if (stricmp(pName, Bone.Name) == 0) {
                return i + 1;
            }
        }
    }
*/
    return -1;
}

//=============================================================================

std::string actor::GetAttachPointNameByID(int iAttachPt) const
{
    if (iAttachPt == 0) {
        return "BaseObject";
    }

    if (m_pLoco) {
        // Decrement by one to bring it into the range [0,nbones)
        iAttachPt -= 1;
        int nBones = m_pLoco->m_Player.GetNBones();
        if ((iAttachPt >= 0) &&
            (iAttachPt < nBones)) {
            const AnimBone& Bone = m_pLoco->m_Player.GetBone(iAttachPt);

            return Bone.name;
        }
    }

    return "INVALID";
}

//=============================================================================

void actor::OnAttachedMove(int            iAttachPt,
                           const Matrix4& L2W)
{
    if (iAttachPt == 0) {
        OnTransform(L2W);
    } else if (m_pLoco) {
        // Decrement by one to bring it into the range [0,nbones)
        iAttachPt -= 1;
        int nBones = m_pLoco->m_Player.GetNBones();
        if ((iAttachPt >= 0) &&
            (iAttachPt < nBones)) {
            Matrix4 BoneL2W = m_pLoco->m_Player.GetBoneL2W(iAttachPt);
            BoneL2W.PreTranslate(m_pLoco->m_Player.GetBoneBindPosition(iAttachPt));

            Vector3 BonePos = BoneL2W.GetTranslation();
            BonePos -= GetPosition();

            Matrix4 NewL2W = L2W;
            NewL2W.Translate(-BonePos);

            OnTransform(NewL2W);
        }
    }
}

//=============================================================================

bool actor::GetAttachPointData(int      iAttachPt,
                               Matrix4& L2W,
                               uint32_t Flags)
{
    if (iAttachPt == 0) {
        L2W = GetL2W();
        return true;
    }
    if (m_pLoco) {
        // Decrement by one to bring it into the range [0,nbones)
        iAttachPt -= 1;
        int nBones = m_pLoco->m_Player.GetNBones();
        if ((iAttachPt >= 0) &&
            (iAttachPt < nBones)) {
            L2W = m_pLoco->m_Player.GetBoneL2W(iAttachPt);
            if (Flags & ATTACH_USE_WORLDSPACE) {
                L2W.PreTranslate(m_pLoco->m_Player.GetBoneBindPosition(iAttachPt));
            }

            return true;
        }
    }

    return false;
}

//=============================================================================
// RENDER FUNCTIONS
//=============================================================================

const Matrix4* actor::GetBonesForRender(uint64_t LODMask, int& nActiveBones)
{
    int i;

    // Lookup loco
    loco* pLoco = GetLocoPointer();
    if (!pLoco) {
        return nullptr;
    }
    /*
        // SB 6/26/03 -
        // Multi-view optimization: Compute the nActiveBones for all views and keep the max count

        // Compute # of bones that the skin rendering will need
        skin_inst& SkinInst = GetSkinInst();
        nActiveBones = SkinInst.GetNActiveBones(LODMask);

        // SB: 2/22/05
        // Always include the weapon bone for correct positioning
        nActiveBones = x_max(nActiveBones, pLoco->GetWeaponBoneIndex() + 1);

        // No bones?!
        if (nActiveBones == 0) {
            return nullptr;
        }

        // Use loco to compute bones?
        const Matrix4* pMatrices = nullptr;
        if (pLoco->IsAnimLoaded()) {
            // Tell the animation player the # of bones to compute
            pLoco->m_Player.SetNActiveBones(nActiveBones);

            // Compute matrices
            assert(pLoco->GetNActiveBones() == nActiveBones);
            pMatrices = pLoco->ComputeL2W();
        }

        // If animation not loaded, just render with bind pose
        // (this can only happen in the editor when an anim is not yet assigned to a new actor)
        if (pMatrices == nullptr) {
            // Allocate matrices
            Matrix4* pMat = (Matrix4*)smem_BufferAlloc(nActiveBones * sizeof(Matrix4));
            if (!pMat) {
                return nullptr;
            }

            // Get Object transform
            const Matrix4& L2W = GetL2W();

            // Use bind pose at current transform
            for (i = 0; i < nActiveBones; i++) {
                pMat[i] = L2W;
            }

            pMatrices = pMat;
        }

        return pMatrices;
        */
    return nullptr;
}

void actor::ResetRidingPlatforms()
{
    //if (GetLocoPointer()) {
    //    GetLocoPointer()->m_Physics.ResetRidingPlatforms();
    // }
}

//=============================================================================

void actor::SetSkinVMesh(bool Mutant)
{
    // multi player only
    return;
}

//=============================================================================

void actor::SetAvatarMutationState(avatar_mutation_state State)
{
    if (State != m_AvatarMutationState) {
        m_AvatarMutationState = State;

        if (!m_pEffects) {
            GetActorEffects(true);
        }

        switch (m_AvatarMutationState) {
        case AVATAR_NORMAL:
            m_TimeLeftInAvatarMutationState = FLT_MAX;
            break;

        case AVATAR_NORMALING:
        {
            m_TimeLeftInAvatarMutationState = g_AvatarTweaks.m_NormalingTime;
            if (!m_bDead) {
                //      m_pEffects->InitEffect(actor_effects::FX_UNMUTATE, this);
            }
        } break;

        case AVATAR_MUTATING:
        {
            m_TimeLeftInAvatarMutationState = g_AvatarTweaks.m_MutatingTime;
            if (!m_bDead) {
                //       m_pEffects->InitEffect(actor_effects::FX_MUTATE, this);
            }
        } break;

        case AVATAR_MUTANT:
            m_TimeLeftInAvatarMutationState = FLT_MAX;
            break;

        default:
            assert(false);
            break;
        }
    }
}

//=============================================================================

static const float TIME_IN_MUTATING_STATE = 0.0f;
static const float TIME_IN_NORMALING_STATE = 0.0f;

void actor::UpdateAvatarMutation(float DeltaTime)
{
    m_TimeLeftInAvatarMutationState -= DeltaTime;

    switch (m_AvatarMutationState) {
    case AVATAR_NORMAL:

        // Show normal skin
        SetSkinVMesh(false);

        // Switch to mutate?
        if (m_bIsMutated) {
            SetAvatarMutationState(AVATAR_MUTATING);
        }
        break;

    case AVATAR_MUTATING:
        if (m_TimeLeftInAvatarMutationState <= 0.0f) {
            m_AvatarMutationState = AVATAR_MUTANT;
            m_TimeLeftInAvatarMutationState = TIME_IN_MUTATING_STATE;
        }
        break;

    case AVATAR_MUTANT:

        // Show mutant skin
        SetSkinVMesh(true);

        // Switch to normal?
        if (!m_bIsMutated) {
            SetAvatarMutationState(AVATAR_NORMALING);
        }
        break;

    case AVATAR_NORMALING:
        if (m_TimeLeftInAvatarMutationState <= 0.0f) {
            m_AvatarMutationState = AVATAR_NORMAL;
            m_TimeLeftInAvatarMutationState = TIME_IN_NORMALING_STATE;
        }
        break;

    default:
        //ASSERTS( false, xfs( "Invalid avatar mutation state: %d", m_AvatarMutationState ) );
        break;
    }
}

//=============================================================================

void actor::PrepPlayerAvatar()
{
    /*
    m_hAudioPackage.SetName("AI_Multiplayer_Player.audiopkg");

    m_SkinInst.SetUpSkinGeom(PRELOAD_MP_FILE("MP_AVATAR_BIND.skingeom"));

    m_hAnimGroup.SetName(PRELOAD_MP_FILE("MP_AVATAR.anim"));

    m_hBloodDecalPackage.SetName(PRELOAD_MP_FILE("Blood.decalpkg"));
    m_BloodDecalGroup = 0; // 0 = Human, 1 = Gray, 2 = Blackops

    if ((m_SkinInst.GetGeom()) && (m_hAnimGroup.GetPointer()) && (m_pLoco)) {
        // Tell loco
        m_pLoco->OnInit(m_SkinInst.GetGeom(), m_hAnimGroup.GetName(), GetGuid());

        InitLoco();
    }
        */
}

//=============================================================================

bool actor::IsAlly(const actor* pActor) const
{
    return (IsFriendlyFaction(pActor->GetFaction()));
}

//=============================================================================

bool actor::IsEnemy(const actor* pActor) const
{
    assert(false); //(0, "OBSOLETE - Please use IsEnemyFaction()");
    return true;
}

//=============================================================================

float actor::ModifyDamageByDifficulty(float Damage)
{
    /*
        // make sure the one taking damage is a player before we scale the damage
        if (IsKindOf(player::GetRTTI())) {
            tweak_handle DamageScalarTweak(xfs("%s_Damage_%s",
                                               GetLogicalName(),
                                               DifficultyText[g_Difficulty]));

            float DamageScalar = 0.0f;

            if (DamageScalarTweak.Exists()) {
                DamageScalar = DamageScalarTweak.GetF32();
            } else {
                // missing tweak for this type, just load in default/generic damage modifier
                tweak_handle DefaultDamageScalarTweak(xfs("GENERIC_Damage_%s",
                                                          DifficultyText[g_Difficulty]));

                DamageScalar = DefaultDamageScalarTweak.GetF32();
            }

            // Scale damage based on difficulty level
            // the scalar could be +/- and is a whole percentage i.e. -20
            float NewDamage = Damage + (Damage * (DamageScalar / 100.0f));

            return NewDamage;
        }
    */
    return Damage;
}

float actor::ModifyDamageByTurret(float Damage)
{
    /*
    s_AlamoTurretDamagePct.GetF32();

    int   LevelID = g_ActiveConfig.GetLevelID();
    float DamageModifier = 1.0f;
    if (LevelID == LEVELID_THE_LAST_STAND) {
        DamageModifier = s_AlamoTurretDamagePct.GetF32();
    } else if ((LevelID == LEVELID_BURIED_SECRETS) || (LevelID == LEVELID_NOW_BOARDING)) {
        DamageModifier = s_ExcavationTurretDamagePct.GetF32();
    }

    Damage *= DamageModifier;
*/
    return Damage;
}

//=============================================================================

bool actor::SetMutated(bool bMutate)
{
    //  LOG_MESSAGE( "actor::SetMutated", "Mutate:%d", bMutate );

    m_bIsMutated = bMutate;

    return true;
}

//=============================================================================

void actor::SetupMutationChange(bool bMutate)
{
    //  LOG_MESSAGE( "actor::SetupMutationChange", "Mutate:%d", bMutate );
    /*
        const bool bWasMutated = m_bIsMutated;
        SetMutated(bMutate);
        const bool bIsMutated = m_bIsMutated;

        if ((bWasMutated != bIsMutated) && GetActorEffects(true) && m_pEffects) {
            if (bMutate) {
                m_pEffects->InitEffect(actor_effects::FX_MUTATE, this);
            } else {
                m_pEffects->InitEffect(actor_effects::FX_UNMUTATE, this);
            }
        }
            */
}

//=============================================================================

void actor::ForceMutationChange(bool bMutate)
{

    SetMutated(bMutate);
    SetSkinVMesh(bMutate);
}

//=============================================================================

void actor::SetCanToggleMutation(bool bCanToggleMutation)
{
    m_bCanToggleMutation = bCanToggleMutation;
}

//=============================================================================

void actor::SetMutagenBurnMode(mutagen_burn_mode MutagenBurnMode)
{
    m_MutagenBurnMode = MutagenBurnMode;
}

//=============================================================================

Vector3 actor::GetBonePos(int BoneIndex)
{
    return m_pLoco->m_Player.GetBonePosition(BoneIndex);
}

//=============================================================================

void actor::UpdateInactive(float DeltaTime)
{
    m_InactiveTime += DeltaTime;

    Vector3 Movement = GetPosition() - m_RecentPosition;
    if (Movement.LengthSquared() > 250000.0f) {
        m_RecentPosition = GetPosition();
        m_InactiveTime = 0.0f;
    }
}
