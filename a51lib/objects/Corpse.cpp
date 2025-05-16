

// #include "Entropy\Entropy.hpp"
// #include "Characters\Character.hpp"
#include "../loco/Loco.h"
// #include "Objects\BaseProjectile.hpp"
// #include "NetworkMgr\NetObj.hpp"
#include "../decals/DecalMgr.h"
// #include "Dictionary\global_dictionary.hpp"
#include "../characters/ActorEffects.h"
#include "Corpse.h"
#include "CorpsePain.h"
#include "../physics/PhysicsMgr.h"
// #include "Objects\ParticleEmiter.hpp"
#include "Player.h"
#include "../objectManager/ObjectManager.h"
#include "../objectManager/ObjectPtr.h"
// #include "Objects\DamageField.hpp"
#include "Actor.h"

//=========================================================================
// DEFINES
//=========================================================================

static int   CORPSE_MAX_DYNAMIC_COUNT = 3;
static int   CORPSE_MAX_ACTIVE_COUNT = 4;
static float CORPSE_FADEOUT_START_TIME = 8.0f;
static float CORPSE_FADEOUT_TIME = 1.0f;
static float CORPSE_IMPACT_SFX_SPEED_SQR = 300.0f * 300.0f;
static float CORPSE_IMPACT_SFX_INTERVAL_TIME = 0.1f;
static float CORPSE_BLEND_CONSTRAINTS_TIME = 1.0f;

//=========================================================================
// DATA
//=========================================================================

int corpse::m_ActiveCount = 0; // # of active (moving) corpses

// Workspace data for corpses for initialization. Using scratchmem would
// be preferable, but this needs to be used in the loading screen where
// smem isn't available to us.
static Matrix4 s_InitMatrices[MAX_ANIM_BONES];
static AnimKey s_InitAnimKeys[MAX_ANIM_BONES];

// Material type enum
typedef enum_pair<int>           corpse_material_enum_pair;
static corpse_material_enum_pair s_CorpseMaterialEnumPair[] =
    {
        // Available materials
        corpse_material_enum_pair("FLESH", Object::MAT_TYPE_FLESH),
        corpse_material_enum_pair("FABRIC", Object::MAT_TYPE_FABRIC),

        //**MUST BE LAST**//
        corpse_material_enum_pair(k_EnumEndStringConst, Object::MAT_TYPE_NULL)};
enum_table<int> s_CorpseMaterialEnum(s_CorpseMaterialEnumPair);

extern bool g_level_loading;
extern bool g_bBloodEnabled;
extern bool g_bRagdollsEnabled;

struct corpse_table_entry
{
    int         Item;
    const char* pName;
    const char* pIdentifier;
};

static corpse_table_entry s_CorpseTable[] =
    {
        {CORPSE_GENERIC, "Generic Corpse", "GENERIC"},
        {CORPSE_BRIDGES, "Bridges Corpse", "BRIDGES"},
        {CORPSE_CARSON, "Carson Corpse", "CARSON"},
        {CORPSE_CRISPY, "Crispy Corpse", "CRISPY"},
        {CORPSE_CRISPY_MUTATED, "Crispy-MUTATED Corpse", "CRISPY_MUT"},
        {CORPSE_CHEW, "Chew Corpse", "CHEW"},
        {CORPSE_DRCRAY, "Dr. Cray Corpse", "DRCRAY"},
        {CORPSE_FERRI, "Ferri Corpse", "FERRI"},
        {CORPSE_LEONARD, "Leonard Corpse", "LEONARD"},
        {CORPSE_MCCANN, "McCann Corpse", "MCCANN"},
        {CORPSE_MRWHITE, "Mr. White Corpse", "MRWHITE"},
        {CORPSE_RAMIREZ, "Ramirez Corpse", "RAMIREZ"},
        {CORPSE_VICTOR, "Victor Corpse", "VICTOR"},
};

#define NUM_CORPSE_TABLE_ENTRIES ((int)(sizeof(s_CorpseTable) / sizeof(corpse_table_entry)))

static struct corpse_desc : public object_desc
{
    corpse_desc()
        : object_desc(Object::TYPE_CORPSE,
                      "Dead Body", //!!!!
                      "AI",

                      Object::ATTR_SPACIAL_ENTRY |
                          Object::ATTR_NEEDS_LOGIC_TIME |
                          Object::ATTR_SOUND_SOURCE |
                          Object::ATTR_COLLIDABLE |
                          Object::ATTR_BLOCKS_ALL_PROJECTILES |
                          Object::ATTR_BLOCKS_RAGDOLL |
                          Object::ATTR_BLOCKS_SMALL_DEBRIS |
                          Object::ATTR_DAMAGEABLE |
                          Object::ATTR_NO_RUNTIME_SAVE |
                          Object::ATTR_RENDERABLE |
                          Object::ATTR_TRANSPARENT,

                      FLAGS_GENERIC_EDITOR_CREATE | FLAGS_NO_ICON |
                          FLAGS_IS_DYNAMIC)
    {
    }

    virtual Object* Create(ObjectManager* om, collision_mgr*) { return new corpse(om); }

} s_CorpseDesc;

//=========================================================================

const object_desc& corpse::GetTypeDesc() const
{
    return s_CorpseDesc;
}

//=========================================================================

const object_desc& corpse::GetObjectType()
{
    return s_CorpseDesc;
}

//=========================================================================
// FUNCTIONS
//=========================================================================

//=========================================================================

corpse::corpse(ObjectManager* om)
    : Object(om)
    , m_OriginGuid(0)
    , m_bActive(false)
    , m_bCreatedBlood(false)
    , m_bCanDelete(false)
    , m_bPermanent(false)
    , m_bDestroy(false)
    , m_bDrainable(false)
    , m_bActorCollision(false)
    , m_bWorldCollision(true)
    , m_bActiveWhenVisible(false)
    , m_TimeAlive(0.0f)
    , m_pActorEffects(nullptr)
    , m_AnimGroupName(-1)
    , m_AnimName(-1)
    , m_AnimFrame(0)
    , m_SimulationTime(0.0f)
    , m_Material(Object::MAT_TYPE_FLESH)
    , m_BloodDecalGroup(-1)
    , m_CorpseName(CORPSE_GENERIC)
    , m_ImpactSfxTimer(0.0f)
{
    m_FloorProperties.Init(100.0f, 0.5f);

    // This assumes that the dead body is about 2.5 meters tall.  Then is about 2
    // meters around.  The BBox is center around the eyes of the player.
    m_ZoneTracker.SetBBox(BBox(Vector3(-100, -200, -100),
                               Vector3(100, 50, 100)));

    m_FadeOutTime = CORPSE_FADEOUT_TIME;
}

//=========================================================================

corpse::~corpse()
{
    if (m_pActorEffects) {
        delete m_pActorEffects;
        m_pActorEffects = nullptr;
    }
}

//=========================================================================

void corpse::OnKill(void)
{
    if (m_FlamingDamageField != NULL_GUID) {
        objectManager->DestroyObject(m_FlamingDamageField);
    }
}

//=========================================================================

void corpse::OnMove(const Vector3& NewPos)
{
    // Call base class
    Object::OnMove(NewPos);

    // Update zone tracking
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, NewPos);
    // IJB m_PhysicsInst.SetZone(m_ZoneTracker.GetMainZone());

    // Move flaming damage field
    if (m_FlamingDamageField != NULL_GUID) {
        Object* pObject = objectManager->GetObjectByGuid(m_FlamingDamageField);

        if (pObject) {
            pObject->OnMove(GetBBox().GetCenter());
        }
    }
}

//=========================================================================

void corpse::OnTransform(const Matrix4& L2W)
{
    // Call base class
    Object::OnTransform(L2W);

    // Update zone tracking
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, L2W.GetTranslation());
    // IJB m_PhysicsInst.SetZone(m_ZoneTracker.GetMainZone());
}

//=========================================================================

BBox corpse::GetLocalBBox(void) const
{
    // Get BBox from Geom? (Fixes punch bag collision since it's so tall!)
    /*  IJB
    const Geom* pGeom = GetSkinInst().GetGeom();
    if (pGeom) {
        BBox BBox(Vector3(0.0f, 0.0f, 0.0f), pGeom->bbox.GetRadius());
        return BBox;
    } else {
     */
        // Use generic BBox
        return BBox(Vector3(-150, -150, -150), Vector3(150, 150, 150));
    //}
}

//===========================================================================

// This function gets called once per game loop from god::OnAdvance
void corpse::LimitCount(ObjectManager* om)
{
    // Keep looping until active count is reduced
    bool bActiveCountValid;
    do {
        // Default to valid count
        bActiveCountValid = true;

        // Clear global active count
        m_ActiveCount = 0;

        // Search for dynamically created corpses
        int DynamicCorpseCount = 0;

        // Search for slowest moving active corpse
        corpse* pActiveCorpse = nullptr;
        float   ActiveSpeedSqr = FLT_MAX;

        // Search for oldest inactive corpse
        corpse* pInactiveCorpse = nullptr;
        float   InactiveTimeAlive = -FLT_MAX;

        // Loop through all corpses in the world looking for oldest dynamic corpses
        for (slot_id Slot = om->GetFirst(Object::TYPE_CORPSE); Slot != SLOT_NULL; Slot = om->GetNext(Slot)) {
            // Lookup corpse
            corpse* pCorpse = (corpse*)om->GetObjectBySlot(Slot);

            // Skip if already flagged to be destroyed
            if (pCorpse->m_bDestroy) {
                continue;
            }

            // Update global active count
            /* IJB
            if (pCorpse->m_PhysicsInst.IsActive()) {
                m_ActiveCount++;
            }
*/
            // Skip if already fading out
            if (pCorpse->m_TimeAlive >= CORPSE_FADEOUT_START_TIME) {
                continue;
            }

            // Skip if permanent
            if (pCorpse->m_bPermanent) {
                continue;
            }

            // Update dynamic (created by killing an npc ie. not editor placed) count
            DynamicCorpseCount++;

            // Active?
            /* IJB
            if (pCorpse->m_PhysicsInst.IsActive()) {
                // Keep least moving active dynamic corpse
                float CorpseSpeedSqr = pCorpse->m_PhysicsInst.GetSpeedSqr();
                if (CorpseSpeedSqr < ActiveSpeedSqr) {
                    pActiveCorpse = pCorpse;
                    ActiveSpeedSqr = CorpseSpeedSqr;
                }
            } else {
                // Keep oldest inactive dynamic corpse
                float CorpseTimeAlive = pCorpse->m_TimeAlive;
                if (CorpseTimeAlive > InactiveTimeAlive) {
                    pInactiveCorpse = pCorpse;
                    InactiveTimeAlive = CorpseTimeAlive;
                }
            }
                */
        }

        // If there are too many dynamic corpses or too many active corpses, fade out the oldest
        if ((DynamicCorpseCount > CORPSE_MAX_DYNAMIC_COUNT) || (m_ActiveCount > CORPSE_MAX_ACTIVE_COUNT) /* IJB || (g_PhysicsMgr.GetAwakeInstanceCount() > CORPSE_MAX_ACTIVE_COUNT) */) {
            // Favor inactive corpses first (if there are any)
            corpse* pOldestCorpse = (pInactiveCorpse != nullptr) ? pInactiveCorpse : pActiveCorpse;

            // Corpse to fade out?
            if (pOldestCorpse) {
                // Should be valid
                assert(pOldestCorpse);
                assert(!pOldestCorpse->m_bPermanent);
                assert(!pOldestCorpse->m_bDestroy);
                assert(pOldestCorpse->m_TimeAlive < CORPSE_FADEOUT_START_TIME);

                // Instead of popping away, start the fade
                pOldestCorpse->m_TimeAlive = CORPSE_FADEOUT_START_TIME;

                // Put the rigid bodies to sleep so that the physics system does not run out of constraints
                /* IJB
                pOldestCorpse->m_PhysicsInst.Deactivate();
                pOldestCorpse->m_PhysicsInst.SetInstCollision(false);
*/
                // Keep checking
                bActiveCountValid = false;
            }
        }
    } while (!bActiveCountValid); // Keep limiting until count is acceptable
}

//===========================================================================

bool corpse::ReachedMaxActiveLimit(void)
{
    return (m_ActiveCount >= CORPSE_MAX_ACTIVE_COUNT)
            /* IJB || (g_PhysicsMgr.GetAwakeInstanceCount() > CORPSE_MAX_ACTIVE_COUNT)*/;
}

//===========================================================================

bool corpse::Initialize(actor& Actor, bool bDoBodyFade, actor_effects* pActorEffects)
{
    // Keep owner
    m_OriginGuid = Actor.GetGuid();

    // copy in the actor effects
    if (m_pActorEffects) {
        delete m_pActorEffects;
    }
    m_pActorEffects = pActorEffects;
    m_bPermanent = !bDoBodyFade;

    /* IJB
    // Initialize the ragdoll with pop fixing and blending in the constraints
    if (!m_PhysicsInst.Init(Actor.GetSkinInst(), true, CORPSE_BLEND_CONSTRAINTS_TIME)) {
        return false;
    }

    // Set the zone info
    m_ZoneTracker = Actor.GetZoneTracker();
    SetZone1(Actor.GetZone1());
    SetZone2(Actor.GetZone2());
*/
    // Copy the time when It was created
    m_TimeAlive = 0.0f;

    // Copy floor properties
    m_FloorProperties = Actor.GetFloorProperties();

    // Now setup the matrices of the ragdoll from the current animation to inherit velocities
    loco* pLoco = Actor.GetLocoPointer();
    if (pLoco) {
        // IJB m_PhysicsInst.SetMatrices(pLoco->m_Player, pLoco->GetDeltaPos());
    }

    // Now rigid body matrices have been setup, update the object transform, which will
    // correctly update the world bounding box and zone
    // IJB OnMove(m_PhysicsInst.GetPosition());

    // copy the decal data
    m_hBloodDecalPackage.setName(Actor.GetBloodDecalPackage());
    m_BloodDecalGroup = Actor.GetBloodDecalGroup();

    // If we're on fire, create a damage field
    if (m_pActorEffects && m_pActorEffects->IsEffectOn(actor_effects::FX_FLAME)) {
        if (m_FlamingDamageField != NULL_GUID) {
            Object* pObject = objectManager->GetObjectByGuid(m_FlamingDamageField);
            if (pObject) {
                objectManager->DestroyObject(pObject->GetSlot());
            }
        }
        /* IJB
        m_FlamingDamageField = objectManager->CreateObject(damage_field::GetObjectType());
        damage_field* pDamageField = (damage_field*)objectManager->GetObjectByGuid(m_FlamingDamageField);

        if (pDamageField) {
            damage_field& DF = *pDamageField;
            DF.SetSpatialType(damage_field::SPATIAL_TYPE_SPHERICAL);
            DF.SetSpatialTargets(damage_field::DF_TARGET_PLAYER);
            DF.SetActive(true);
            DF.SetDimension(0, 50.0f);
        }
            */
    }

    // Success
    return true;
}

//===============================================================================

// This function is only called when creating permanent dead bodies in the editor
bool corpse::Initialize(const char* pGeomName,
                        const char* pAnimGroupName,
                        const char* pAnimName,
                        int         AnimFrame)
{
    // Must be valid
    if ((!pGeomName) || (!pAnimGroupName) || (!pAnimName)) {
        return false;
    }

    /* IJB
    // Initialize the ragdoll (no need to fix popping)
    if (!m_PhysicsInst.Init(pGeomName, false, 0.0f)) {
        return false;
    }

    // Lookup Geom
    const SkinGeom* pGeom = GetSkinInst().GetSkinGeom();
    if (!pGeom) {
        return false;
    }
*/
    // Copy the time when It was created
    m_TimeAlive = 0.0f;

    // Lookup animation group
    /* IJB
    AnimGroup::handle& hAnimGroup = m_PhysicsInst.GetAnimGroupHandle();
    hAnimGroup.setName(pAnimGroupName);
    const AnimGroup* pAnimGroup = hAnimGroup.getPointer();
    if (!pAnimGroup) {
        return false;
    }

    // Lookup animation info
    int nBones = pAnimGroup->GetNBones();
    int iAnim = pAnimGroup->GetAnimIndex(pAnimName);
    if (iAnim == -1) {
        return false;
    }
    const AnimInfo& animInfo = pAnimGroup->GetAnimInfo(iAnim);

    // Clamp frame
    int LastFrame = std::max(0, animInfo.GetNFrames() - 2);
    if (AnimFrame > LastFrame) {
        AnimFrame = LastFrame;
    }

    // Setup ptrs
    Matrix4*  pMatrices = s_InitMatrices;
    anim_key* pKeys = s_InitAnimKeys;

    // Compute matrices
    AnimInfo.GetInterpKeys((float)AnimFrame, pKeys, nBones);
    pAnimGroup->ComputeBonesL2W(GetL2W(), pKeys, nBones, pMatrices, true);

    // Setup the ragdoll bodies
    m_PhysicsInst.SetMatrices(pMatrices, nBones, false);

    // Create body -> world constraints from events in animation
    for (int iEvent = 0; iEvent < AnimInfo.GetNEvents(); iEvent++) {
        // Lookup event
        const anim_event& Event = AnimInfo.GetEvent(iEvent);

        // Is this a generic super event?
        const char* pEventType = Event.GetType();
        if (x_stricmp(pEventType, "Generic") == 0) {
            // Is this a "PinToWorld" event?
            const char* pType = Event.GetString(anim_event::STRING_IDX_GENERIC_TYPE);
            if (x_stricmp(pType, "Pin_To_World") == 0) {
                // Walk up hierarchy to get a valid bone
                int iBone = Event.GetInt(anim_event::INT_IDX_BONE);
                while (iBone >= pGeom->m_nBones) {
                    iBone = pAnimGroup->GetBoneParent(iBone);
                }

                // Compute event world position
                Matrix4& BoneL2W = pMatrices[iBone];
                Vector3  WorldPos = BoneL2W * Event.GetPoint(anim_event::POINT_IDX_OFFSET);

                // Finally, create the constraint
                m_PhysicsInst.AddBodyWorldConstraint(pGeom->m_pBone[iBone].iRigidBody, WorldPos, 0.0f);
            }
        }
    }
*/
    // Success
    return true;
}

//===============================================================================

bool corpse::InitializeEditorPlaced(void)
{
    // Skip if loading since the static world is not yet loaded and corpses will fall forever! -
    // the main app or editor will call this function again after loading.
    if (g_level_loading) {
        return false;
    }
/* IJB
    // Valid?
    if ((!GetSkinInst().GetSkinGeomName()) || (!GetSkinInst().GetSkinGeom()) || (m_AnimGroupName == -1) || (m_AnimName == -1)) {
        return false;
    }

    // Never delete
    m_bPermanent = true;

    // Make sure properties are included in game save/load
    SetAttrBits(GetAttrBits() & ~Object::ATTR_NO_RUNTIME_SAVE);

    // Initialize from properties
    if (!Initialize(GetSkinInst().GetSkinGeomName(),
                    g_StringMgr.GetString(m_AnimGroupName),
                    g_StringMgr.GetString(m_AnimName),
                    m_AnimFrame)) {
        return false;
    }

    // Run the physics for the specified simulation time...
    float Step = 1.0f / 30.0f;
    for (float Time = 0; Time < m_SimulationTime; Time += Step) {
        // Force activation incase bodies deactivate
        m_PhysicsInst.Activate();

        // Advance simulation
        g_PhysicsMgr.Advance(Step);
    }
    g_PhysicsMgr.ClearDeltaTime();

    // Finally activate or deactivate physics instance (since sim time maybe zero)?
    if (m_bActive) {
        m_PhysicsInst.Activate();
    } else {
        m_PhysicsInst.Deactivate();
    }

    // Setup floor color
    m_FloorProperties.ForceUpdate(m_PhysicsInst.GetPosition());

#ifdef X_EDITOR
    // Only update position if game is running
    if (g_game_running) {
        OnMove(m_PhysicsInst.GetPosition());
    }
#else
    // Update position ready for correct rendering
    OnMove(m_PhysicsInst.GetPosition());
#endif
*/
    // Success
    return true;
}

//===============================================================================

void corpse::OnRenderShadowCast(uint64_t ProjMask)
{
    /* IJB
    // Compute LOD mask for the shadow render (by putting zero in for screen size
    // we force the lowest lod)
    uint64_t ShadLODMask = GetSkinInst().GetLODMask(0);
    if (ShadLODMask == 0) {
        return;
    }

    // Compute matrices
    uint64_t            LODMask;
    int            nActiveBones;
    const Matrix4* pMatrices = m_PhysicsInst.GetBoneL2Ws(LODMask, nActiveBones);
    if (!pMatrices) {
        return;
    }

    // Setup render flags
    uint32_t Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;

    // Render that puppy!
    GetSkinInst().RenderShadowCast(&GetL2W(),
                                   pMatrices,
                                   nActiveBones,
                                   Flags,
                                   ShadLODMask,
                                   ProjMask);
    */
}

//===============================================================================

void corpse::OnRender(void)
{
    // render any actor effects
    if (m_pActorEffects) {
        m_pActorEffects->Render(this);
    }

    // Setup render flags and get the floor color
    Colour Ambient = GetFloorColor();
    uint32_t    Flags = (GetFlagBits() & Object::FLAG_CHECK_PLANES) ? render::CLIPPED : 0;

    // Render fading out?
    if ((!m_bPermanent) && (m_TimeAlive > CORPSE_FADEOUT_START_TIME)) {
        Flags |= render::FADING_ALPHA;
        float Alpha = 1.0f - ((m_TimeAlive - CORPSE_FADEOUT_START_TIME) / m_FadeOutTime);
        Alpha = std::min(Alpha, 1.0f);
        Alpha = std::max(Alpha, 0.0f);
        Ambient.a = (uint8_t)(Alpha * 255.0f);
    }

    // Render that puppy!
    // IJB m_PhysicsInst.Render(Flags, Ambient);
}

void corpse::OnRenderTransparent(void)
{
    if (m_pActorEffects) {
        float Alpha = 1.0f;
        if ((!m_bPermanent) && (m_TimeAlive > CORPSE_FADEOUT_START_TIME)) {
            Alpha = 1.0f - ((m_TimeAlive - CORPSE_FADEOUT_START_TIME) / m_FadeOutTime);
            Alpha = std::min(Alpha, 1.0f);
            Alpha = std::max(Alpha, 0.0f);
        }
        m_pActorEffects->RenderTransparent(this, Alpha);
    }
}

//===============================================================================

void corpse::OnAdvanceLogic(float DeltaTime)
{
    // update any actor effects
    if (m_pActorEffects) {
        m_pActorEffects->Update(this, DeltaTime);
    }

    // Flagged to be destroyed?
    if (m_bDestroy) {
        assert(!m_bPermanent);
        objectManager->DestroyObject(GetGuid());
        return;
    }

    // Time out and delete his body?
    if ((m_bCanDelete) && (!m_bPermanent) && (m_TimeAlive >= (CORPSE_FADEOUT_START_TIME + m_FadeOutTime))) {
        objectManager->DestroyObject(GetGuid());
        return;
    }

    // Update the timer.
    if (m_bPermanent) {
        // Reset time
        m_TimeAlive = 0.0f;
    } else {
        // Update time
        m_TimeAlive += DeltaTime;

        // If fading out, put the rigid bodies to sleep so that the physics system does not run out of constraints
        if (m_TimeAlive >= CORPSE_FADEOUT_START_TIME) {
            /* IJB
            m_PhysicsInst.Deactivate();
            m_PhysicsInst.SetInstCollision(false);
            */
        }
    }

    // Okay lets assume next time we can be deleted
    m_bCanDelete = true;
/* IJB
    // Get current position from physics instance
    Vector3 NewPos = m_PhysicsInst.GetPosition();

    // Update base object position?
    if (NewPos != GetPosition()) {
        OnMove(NewPos);
    }

    // Update ambient color for rendering
    m_FloorProperties.Update(NewPos, DeltaTime);

    // Update instance house keeping
    m_PhysicsInst.Advance(DeltaTime);

    // Possibly play an impact?
    if (m_ImpactSfxTimer == 0.0f) {
        // Are rigid bodies active?
        physics_inst& PhysicsInst = GetPhysicsInst();
        if (PhysicsInst.IsActive()) {
            // Loop through all rigid bodies looking for fastest collision
            float ImpactSpeedSqr = 0.0f;
            int   iImpactBody = -1;
            for (int i = 0; i < PhysicsInst.GetNRigidBodies(); i++) {
                // Lookup body
                rigid_body& Body = PhysicsInst.GetRigidBody(i);

                // Collision occurred?
                if (Body.HasCollided()) {
                    // Biggest so far?
                    float CollisionSpeedSqr = Body.GetCollisionSpeedSqr();
                    if ((CollisionSpeedSqr > CORPSE_IMPACT_SFX_SPEED_SQR) && (CollisionSpeedSqr > ImpactSpeedSqr)) {
                        // Record
                        iImpactBody = i;
                        ImpactSpeedSqr = CollisionSpeedSqr;
                    }
                }
            }

            // Play impact?
            if (iImpactBody != -1) {
                // Play impact sfx
                g_AudioMgr.Play("TerroristA_Bodyfall",
                                PhysicsInst.GetRigidBody(iImpactBody).GetPosition(),
                                GetZone1(),
                                true);

                // Setup delay before next impact
                m_ImpactSfxTimer = CORPSE_IMPACT_SFX_INTERVAL_TIME;
            }
        }
    } else {
        // Update impact sfx timer
        m_ImpactSfxTimer = x_max(0.0f, m_ImpactSfxTimer - DeltaTime);
    }
        */
}

//===============================================================================

void corpse::OnActivate(bool Flag)
{
    // Wake up or put physics to sleep?
    /* IJB
    if (Flag) {
        m_PhysicsInst.Activate();
    } else {
        m_PhysicsInst.Deactivate();
    }
*/
    // Call base class
    Object::OnActivate(Flag);
}

//===============================================================================

bool corpse::IsBloodEnabled(void) const
{
    // TO DO: Add property and detect german version if this needs to be on
    //        a per character basis
    return g_bBloodEnabled;
}

//===============================================================================

bool corpse::IsRagdollEnabled(void) const
{
    // TO DO: Add property and detect german version if this needs to be on
    //        a per character basis
    return g_bRagdollsEnabled;
}

//===============================================================================

void corpse::CreateSplatDecalOnGround(void)
{
    // Skip?
    if (!IsBloodEnabled()) {
        return;
    }

    // Just create once to keep limit down
    if (m_bCreatedBlood) {
        return;
    }

    // Lookup package
    decal_package* pPackage = m_hBloodDecalPackage.getPointer();
    if (!pPackage) {
        return;
    }

    // Valid group?
    if ((m_BloodDecalGroup < 0) || (m_BloodDecalGroup >= pPackage->GetNGroups())) {
        return;
    }

    // Valid decal def?
    int nDecalDefs = pPackage->GetNDecalDefs(m_BloodDecalGroup);
    if (nDecalDefs == 0) {
        return;
    }

    // choose a random decal to paste
    int               DecalIndex = (nDecalDefs == 1) ? 0 : x_rand() % (nDecalDefs - 1);
    decal_definition& DecalDef = pPackage->GetDecalDef(m_BloodDecalGroup, DecalIndex);

    // create a ray that is biased towards the ground
    Radian3 BloodOrient;
    BloodOrient.pitch = x_frand(R_80, R_100);
    BloodOrient.yaw = x_frand(R_0, R_360);
    BloodOrient.roll = R_0;

    Vector3 RayStart = GetPosition();
    RayStart.y += 10.0f;

    Vector3 RayEnd(0.0f, 0.0f, 1.0f);
    RayEnd.Rotate(BloodOrient);
    RayEnd = 500.0f * RayEnd;
    RayEnd += RayStart;

    // generate the splat decal
    g_DecalMgr.CreateDecalFromRayCast(DecalDef, RayStart, RayEnd);

    // Don't do again...
    m_bCreatedBlood = true;
}

//===============================================================================

void corpse::CreateImpactEffect(const pain& Pain)
{
    // Skip if not a direct hit
    if (!Pain.IsDirectHit()) {
        return;
    }
/* IJB
    // Create blood impact?
    if (m_Material == Object::MAT_TYPE_FLESH) {
        // Only do if blood is on
        if (IsBloodEnabled()) {
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
    }
    // Create dust puff?
    else if (m_Material == Object::MAT_TYPE_FABRIC) {
        // Create fabric impact effect
        particle_emitter::CreateOnPainEffect(Pain,
                                             0.0f,
                                             particle_emitter::IMPACT_FABRIC_HIT,
                                             COLOR_WHITE);
    }
                                             */
}

//===============================================================================

void corpse::OnPain(const pain& Pain)
{
    // Skip?
    if (!IsRagdollEnabled()) {
        return;
    }

    // If we've already starting fading the body out, don't do any more splatting
    if (m_TimeAlive >= CORPSE_FADEOUT_START_TIME) {
        return;
    }
/* IJB
    // If corpse is not active and there are already too many active, leave alone
    if ((m_PhysicsInst.IsActive() == false) && (ReachedMaxActiveLimit())) {
        return;
    }
*/
    // Triggers can create npc ragdolls that do not have any pain setup, but still
    // go through through character death state which always calls this function upon
    // creating a ragdoll.
    if (Pain.SetupCalled() == false) {
        return;
    }

    // Setup corpse pain and apply to self
    corpse_pain CorpsePain;
    CorpsePain.Setup(Pain, *this);
    CorpsePain.Apply(*this);

    // Record pain in origin actor so it can be sent over the net in MP?
    object_ptr<actor> pActor(m_OriginGuid, objectManager);
    if (pActor) {
        // Keep
        pActor->GetCorpseDeathPain() = CorpsePain;

        // Only record once so death pain is not overwritten
        m_OriginGuid = 0;
    }

    // Create blood on ground?
    if (m_Material == Object::MAT_TYPE_FLESH) {
        CreateSplatDecalOnGround();
    }
}

//===============================================================================

void corpse::OnColCheck(void)
{
    // Get moving object
    guid    MovingGuid = g_CollisionMgr.GetMovingObjGuid();
    Object* pObject = objectManager->GetObjectByGuid(MovingGuid);

    // Only collide with bullets/grenades/player/character melee during game
    /* IJB
    if ((pObject) && ((pObject->IsKindOf(net_proj::GetRTTI())) || (pObject->IsKindOf(base_projectile::GetRTTI())) || (pObject->IsKindOf(character::GetRTTI())) || (pObject->IsKindOf(player::GetRTTI())))) {
        m_PhysicsInst.OnColCheck(GetGuid());
    }
        */
}

//===============================================================================

void corpse::ChangeObjectGuid(guid NewGuid)
{
    // Update dead body object
    objectManager->ChangeObjectGuid(GetGuid(), NewGuid);
}

//===============================================================================
// Editor functions
//===============================================================================

void corpse::OnEnumProp(prop_enum& List)
{
    // Call base class
    Object::OnEnumProp(List);

    // Character properties - lets trigger system check the health
    List.PropEnumHeader("Character", "This is here so you can check the health with triggers", 0);
    List.PropEnumFloat("Character\\Health", "This is here so you can check the health with triggers", PROP_TYPE_EXPOSE);

    // Dead body
    List.PropEnumHeader("DeadBody", "Editor placed ragdoll deadbody", 0);

    // Corpse's name/material type
    List.PropEnumEnum("DeadBody\\CorpseName", GetEnumStringCorpse().c_str(), "Which NPCs corpse is this (Generic for NPCs with no name).", PROP_TYPE_EXPOSE);
    List.PropEnumEnum("DeadBody\\Material", s_CorpseMaterialEnum.BuildString(), "Type of material of corpse - drives impact effect.", 0);

    // Ragdoll properties
    List.PropEnumBool("DeadBody\\Active", "Start level with physics active? (false = frozen, true = moving!)\nFor best performance try leaving this to false!", PROP_TYPE_MUST_ENUM);
    List.PropEnumBool("DeadBody\\ActorCollision", "Should player/npcs push ragdoll out of the way?", PROP_TYPE_MUST_ENUM);
    List.PropEnumBool("DeadBody\\WorldCollision", "Should corpse collide with the world?", PROP_TYPE_MUST_ENUM);
    List.PropEnumBool("DeadBody\\ActiveWhenVisible", "Should rigid bodies always be active when visible?", PROP_TYPE_MUST_ENUM);
    List.PropEnumFloat("DeadBody\\SimulationTime", "Physics time to simulate before level starts.", PROP_TYPE_MUST_ENUM);

    // Animation properties
    List.PropEnumExternal("DeadBody\\AnimGroupName", "Resource\0animexternal", "Select the animation group and animation.", PROP_TYPE_MUST_ENUM | PROP_TYPE_EXTERNAL);
    List.PropEnumString("DeadBody\\AnimName", "Name of the animation to play.", PROP_TYPE_MUST_ENUM);
    List.PropEnumInt("DeadBody\\AnimFrame", "Frame of animation to start at.", PROP_TYPE_MUST_ENUM);

    // Geometry properties
    // IJB GetSkinInst().OnEnumProp(List);

    // Decals
    List.PropEnumHeader("BloodDecals", "Which blood decals this actor will leave.", 0);
    List.PropEnumExternal("BloodDecals\\Package", "Decal Resource\0decalpkg\0", "Which blood decal package this actor uses.", 0);
    List.PropEnumInt("BloodDecals\\Group", "Within the decal package, which group of bloud this actor uses.", 0);
}

//===============================================================================

bool corpse::OnProperty(prop_query& I)
{
    // Call base class
    if (Object::OnProperty(I)) {
        // Initialize zone tracker?
        if (I.IsVar("Base\\Position")) {
            g_ZoneMgr.InitZoneTracking(*this, m_ZoneTracker);
        }

        // Update physics instance zone
        // IJB m_PhysicsInst.SetZone(m_ZoneTracker.GetMainZone());

        return true;
    }

    // Health?
    if (I.IsVar("Character\\Health")) {
        if (I.IsRead()) {
            I.SetVarFloat(0.0f);
        }

        return true;
    }

    if (I.IsVar("DeadBody\\CorpseName")) {
        if (I.IsRead()) {
            I.SetVarEnum(EnumToName(m_CorpseName));
        } else {
            m_CorpseName = (eCorpseName)NameToEnum(I.GetVarEnum());
        }

        return true;
    }

    // Material
    if (I.IsVar("DeadBody\\Material")) {
        // Updating UI?
        if (I.IsRead()) {
            if (s_CorpseMaterialEnum.DoesValueExist(m_Material)) {
                I.SetVarEnum(s_CorpseMaterialEnum.GetString(m_Material));
            } else {
                I.SetVarEnum("nullptr");
            }
        } else {
            // Reading from UI/File
            const char* pValue = I.GetVarEnum();

            // Found?
            if (!s_CorpseMaterialEnum.GetValue(pValue, m_Material)) {
                m_Material = Object::MAT_TYPE_NULL;
            }
        }

        return true;
    }

    // Active physics?
    if (I.IsVar("DeadBody\\Active")) {
        if (I.IsRead()) {
            I.SetVarBool(m_bActive);
        } else {
            m_bActive = I.GetVarBool();
        }

        return true;
    }

    // Actor collision?
    if (I.IsVar("DeadBody\\ActorCollision")) {
        if (I.IsRead()) {
            I.SetVarBool(m_bActorCollision);
        } else {
            m_bActorCollision = I.GetVarBool();
        }

        // IJB m_PhysicsInst.SetActorCollision(m_bActorCollision);

        return true;
    }

    // World collision?
    if (I.IsVar("DeadBody\\WorldCollision")) {
        if (I.IsRead()) {
            I.SetVarBool(m_bWorldCollision);
        } else {
            m_bWorldCollision = I.GetVarBool();
        }

        // IJB m_PhysicsInst.SetWorldCollision(m_bWorldCollision);

        return true;
    }

    // ActiveWhenVisible?
    if (I.IsVar("DeadBody\\ActiveWhenVisible")) {
        if (I.IsRead()) {
            I.SetVarBool(m_bActiveWhenVisible);
        } else {
            m_bActiveWhenVisible = I.GetVarBool();
        }

        // IJB m_PhysicsInst.SetActiveWhenVisible(m_bActiveWhenVisible);

        return true;
    }

    // SimulationTime
    if (I.VarFloat("DeadBody\\SimulationTime", m_SimulationTime, 0.0f, 5.0f)) {
        return true;
    }

    // AnimGroup, AnimName?
    /* IJB
    if (SMP_UTIL_IsAnimVar(I,
                           "DeadBody\\AnimGroupName",
                           "DeadBody\\AnimName",
                           m_PhysicsInst.GetAnimGroupHandle(),
                           m_AnimGroupName,
                           m_AnimName)) {
        return true;
    }
*/
    // AnimFrame
    if (I.VarInt("DeadBody\\AnimFrame", m_AnimFrame, 0)) {
        return true;
    }

    // Geometry
    /* IJB
    if (GetSkinInst().OnProperty(I)) {
        return true;
    }
*/
    // Decal package
    if (I.IsVar("BloodDecals\\Package")) {
        if (I.IsRead()) {
            I.SetVarExternal(m_hBloodDecalPackage.getName().c_str(), RESOURCE_NAME_SIZE);
        } else {
            m_hBloodDecalPackage.setName(I.GetVarExternal());
        }

        return true;
    }

    // Decal group
    if (I.VarInt("BloodDecals\\Group", m_BloodDecalGroup)) {
        return true;
    }

    return false;
}

//=========================================================================

const char* corpse::EnumToName(eCorpseName theCorpse)
{
    assert((theCorpse >= 0) && (theCorpse < CORPSE_NAME_MAX));
    const char* pName = s_CorpseTable[theCorpse].pName;
    //CLOG_MESSAGE( CORPSE_LOGGING, "corpse::EnumToName", "%d = '%s'", theCorpse, pName );
    return pName;
}

//=========================================================================

std::string corpse::GetEnumStringCorpse()
{
    std::string EnumString;

    for (int i = 0; i < NUM_CORPSE_TABLE_ENTRIES; i++) {
        EnumString += EnumToName((eCorpseName)i);
        EnumString += '\0';
    }

    return EnumString;
}

//=========================================================================

eCorpseName corpse::NameToEnum(const char* pName)
{
    // Search the table for the corpse name
    for (int i = 0; i < NUM_CORPSE_TABLE_ENTRIES; i++) {
        if (strcmp(pName, s_CorpseTable[i].pName) == 0) {
            assert(i == s_CorpseTable[i].Item);
            //CLOG_MESSAGE( CORPSE_LOGGING, "corpse::NameToEnum", "Found '%s' = %d", pName, i );
            return (eCorpseName)i;
        }
    }

    // Not found so nullptr
    //CLOG_ERROR( CORPSE_LOGGING, "corpse::NameToEnum", " Not Found '%s'", pName );
    return CORPSE_GENERIC;
}

//=========================================================================

const char* corpse::GetScanIdentifier(void)
{
    eCorpseName theCorpse = m_CorpseName;
    assert((theCorpse >= 0) && (theCorpse < CORPSE_NAME_MAX));
    const char* pIdentifier = s_CorpseTable[theCorpse].pIdentifier;
    //CLOG_MESSAGE( CORPSE_LOGGING, "corpse::GetScanIdentifier", "%d = '%s'", theCorpse, pIdentifier );
    return pIdentifier;
}

//=========================================================================

void corpse::StartFading()
{
    // Start the fade
    m_TimeAlive = CORPSE_FADEOUT_START_TIME;

    // Put the rigid bodies to sleep so that the physics system does not run out of constraints
    /* IJB
    m_PhysicsInst.Deactivate();
    m_PhysicsInst.SetInstCollision(false);
    */
}

void corpse::StartFading(float FadeOutTime)
{
    assert(FadeOutTime > 0.0f);
    m_FadeOutTime = FadeOutTime;
    StartFading();
}
