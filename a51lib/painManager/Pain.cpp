#define DO_LOG_SETUP 1
#define DO_LOG_COMPUTE_DAMAGE 1

#include "Pain.h"
#include "PainMgr.h"
#include "../objectManager/ObjectManager.h"

// Need this to handle factions.
// Too bad base class object can't handle it.
#include "../objects/Actor.h"
#include "../objects/Player.h"

#include "../tweakManager/TweakMgr.h"

pain::pain(ObjectManager* om): objectManager(om)
{
    Clear();
}

//==============================================================================

pain::~pain()
{
    Clear();
}

//==============================================================================

void pain::Clear()
{
    m_bSetupCalled = false;
    m_bComputeDamageAndForceCalled = false;

    SetupDefaults();
}

//==============================================================================

void pain::SetupDefaults()
{
    m_OriginGuid = 0;
    m_DirectHitGuid = 0;
    m_CustomScalar = 1.0f;
    m_AnimEventID = -1;
    m_pOriginRTTI = nullptr;
    m_Position.Zero();
    m_Direction.Zero();

    m_Force = 0.0f;
    m_Damage = 0.0f;
    m_ForceDirection.set(0.0f, 1.0f, 0.0f);
    m_VictimGuid = 0;
    m_VictimPainPosition.Zero();
    m_HitType = -1;
    m_pVictimRTTI = nullptr;
    m_LOSCoverage = 0.0f;

    m_bCollisionAvailable = false;
    m_bDirectHit = false;
}

//==============================================================================

void pain::Setup(const char* PainDesc, guid OriginGuid, const Vector3& Position)
{
    pain_handle PainHandle(PainDesc);
    Setup(PainHandle, OriginGuid, Position);
}

//==============================================================================

void pain::Setup(pain_handle PainHandle, guid OriginGuid, const Vector3& Position)
{
    m_bSetupCalled = true;
    m_bComputeDamageAndForceCalled = false;

    SetupDefaults();

    m_PainHandle = PainHandle;
    m_OriginGuid = OriginGuid;
    m_Position = Position;
    m_ImpactPoint = Position;
    m_ImpactNormal.set(0, 0, 0);

    // Compute area of effect.  If PainProfile is not available
    // then it is reported through the logging system.
    const pain_profile* pPainProfile = PainHandle.GetPainProfile();
    if (pPainProfile) {
        m_PainBBox = pPainProfile->m_BBox;
    } else {
        m_PainBBox = BBox(Vector3(0, 0, 0), 10.0f);
    }
    m_PainBBox.Translate(m_Position);

    // Grab a ptr to the rtti of the origin object
    Object* pObj = objectManager->GetObjectByGuid(OriginGuid);
    if (pObj) {
        m_pOriginRTTI = &pObj->GetRTTI();
    }
}

//==============================================================================

bool pain::SetupCalled() const
{
    return m_bSetupCalled;
}

//==============================================================================

bool pain::ComputeDamageAndForceCalled() const
{
    return m_bComputeDamageAndForceCalled;
}

//==============================================================================

void pain::SetDirection(const Vector3& Direction)
{
    assert(m_bSetupCalled);
    m_Direction = Direction;
    m_Direction.Normalize();
    if (HasCollision() == false) {
        m_ImpactNormal = -m_Direction;
    }
}

//==============================================================================

void pain::SetDirectHitGuid(guid DirectHitGuid)
{
    assert(m_bSetupCalled);
    m_DirectHitGuid = DirectHitGuid;
    m_bDirectHit = (DirectHitGuid != 0);
}

//==============================================================================

void pain::SetCustomScalar(float CustomScalar)
{
    assert(m_bSetupCalled);
    assert(CustomScalar >= 0);
    m_CustomScalar = CustomScalar;
}

//==============================================================================

void pain::SetCollisionInfo(const collision_mgr::collision& CollisionInfo)
{
    assert(m_bSetupCalled);
    m_Collision = CollisionInfo;
    m_ImpactPoint = CollisionInfo.Point;
    m_ImpactNormal = CollisionInfo.Plane.Normal;
    m_bCollisionAvailable = true;
}

//==============================================================================

void pain::SetAnimEventID(int AnimEventID)
{
    assert(m_bSetupCalled);
    assert(AnimEventID >= 0);
    m_AnimEventID = AnimEventID;
}

//==============================================================================

pain_handle pain::GetPainHandle() const
{
    assert(m_bSetupCalled);
    return m_PainHandle;
}

//==============================================================================

pain_health_handle pain::GetPainHealthHandle() const
{
    assert(m_bSetupCalled);
    assert(m_bComputeDamageAndForceCalled);
    return m_PainHealthHandle;
}

//==============================================================================

guid pain::GetOriginGuid() const
{
    return m_OriginGuid;
}

//==============================================================================

const Vector3& pain::GetPosition() const
{
    assert(m_bSetupCalled);
    return m_Position;
}

//==============================================================================

const Vector3& pain::GetImpactPoint() const
{
    assert(m_bSetupCalled);
    return m_ImpactPoint;
}

//==============================================================================

const Vector3& pain::GetImpactNormal() const
{
    assert(m_bSetupCalled);
    return m_ImpactNormal;
}

//==============================================================================

const rtti& pain::GetOriginRTTI() const
{
    if (m_pOriginRTTI) {
        return *m_pOriginRTTI;
    } else {
        return Object::GetRTTI();
    }
}

//==============================================================================

const Vector3& pain::GetDirection() const
{
    assert(m_bSetupCalled);
    return m_Direction;
}

//==============================================================================

float pain::GetCustomScalar() const
{
    assert(m_bSetupCalled);
    return m_CustomScalar;
}

//==============================================================================

guid pain::GetDirectHitGuid() const
{
    assert(m_bSetupCalled);
    return m_DirectHitGuid;
}

//==============================================================================

int pain::GetAnimEventID() const
{
    assert(m_bSetupCalled);
    return m_AnimEventID;
}

//==============================================================================

bool pain::IsDamageComputed() const
{
    return m_bComputeDamageAndForceCalled;
}

//==============================================================================

bool pain::HasCollision() const
{
    return m_bCollisionAvailable;
}

//==============================================================================

const collision_mgr::collision& pain::GetCollision() const
{
    assert(m_bSetupCalled);
    assert(m_bCollisionAvailable);
    return m_Collision;
}

//==============================================================================

float pain::GetDamage() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_Damage;
}

//==============================================================================

float pain::GetForce() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_Force;
}

//==============================================================================

const Vector3& pain::GetForceDirection() const
{
    return m_ForceDirection;
}

//==============================================================================

Vector3 pain::GetForceVelocity() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_ForceDirection * m_Force;
}

//==============================================================================

bool pain::IsDirectHit() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_bDirectHit;
}

//==============================================================================

bool pain::IsFriendlyFire() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_bIsFriendlyFire;
}

//==============================================================================

guid pain::GetVictimGuid() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_VictimGuid;
}

//==============================================================================

health_handle pain::GetHealthHandle() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_HealthHandle;
}

//==============================================================================

const Vector3& pain::GetVictimPainPosition() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_VictimPainPosition;
}

//==============================================================================

int pain::GetHitType() const
{
    // SB: 2/23/05
    // NOTE: "m_HitType" is always valid - it defaults to -1 if
    //       "ComputeDamageAndForceCalled" has not been called
    return m_HitType;
}

//==============================================================================

float pain::GetLOSCoverage() const
{
    assert(m_bComputeDamageAndForceCalled);
    return m_LOSCoverage;
}

//==============================================================================

const rtti& pain::GetVictimRTTI() const
{
    if (m_pVictimRTTI) {
        return *m_pVictimRTTI;
    } else {
        return Object::GetRTTI();
    }
}

//==============================================================================

bool pain::ComputeDamageAndForce(const char* HealthDesc, guid VictimGuid, const Vector3& VictimPainPosition) const
{
    health_handle HealthHandle(HealthDesc);
    return ComputeDamageAndForce(HealthHandle, VictimGuid, VictimPainPosition);
}

//==============================================================================

bool pain::ComputeDamageAndForce(health_handle HealthHandle, guid VictimGuid, const Vector3& VictimPainPosition) const
{
    // Copy parameters into pain class
    assert(HealthHandle.IsValid());
    m_HealthHandle = HealthHandle;
    m_VictimGuid = VictimGuid;
    m_VictimPainPosition = VictimPainPosition;

    // Get ptr to the killer and victim
    Object* pKillerObj = objectManager->GetObjectByGuid(m_OriginGuid);
    Object* pVictimObj = objectManager->GetObjectByGuid(m_VictimGuid);

    // Grab a ptr to the rtti of the victim object
    if (pVictimObj) {
        m_pVictimRTTI = &pVictimObj->GetRTTI();
    }

    // Clear return values
    m_Damage = 0;
    m_Force = 0;
    m_ForceDirection.Zero();
    m_bComputeDamageAndForceCalled = true;
    m_bIsFriendlyFire = true;
    m_LOSCoverage = 0;

    // Lookup the different data structures from the data_vault.
    // If they are not available it will be reported via the
    // logging system by the individual handles.  We just need to survive.

    // Lookup pain_profile
    const pain_profile* pPainProfile = m_PainHandle.GetPainProfile();
    if (!pPainProfile) {
        return false;
    }
    const pain_profile& PainProfile = *pPainProfile;

    // Do LOSCoverage check
    if (PainProfile.m_bCheckLOS) {
        ComputeLOSCoverage();

        // If we have full coverage then no damage or force can touch us.
        if (m_LOSCoverage == 1.0f) {
            return false;
        }
    }

    // Lookup health_profile
    const health_profile* pHealthProfile = m_HealthHandle.GetHealthProfile();
    if (!pHealthProfile) {
        return false;
    }

    // Lookup pain_health_profile
    m_PainHealthHandle = m_PainHandle.BuildPainHealthProfileHandle(m_HealthHandle);
    const pain_health_profile* pPainHealthProfile = m_PainHealthHandle.GetPainHealthProfile();
    if (!pPainHealthProfile) {
        return false;
    }
    const pain_health_profile& PainHealthProfile = *pPainHealthProfile;

    // Copy hit-type out of profile
    m_HitType = PainHealthProfile.m_HitType;

    // Compute Scalars.  These are used to scale the final table-based
    // Damage and Force values.
    float DamageScalar;
    float ForceScalar;

    // Check if pain does splash damage or not
    if (PainProfile.m_bSplash) {
        // Handle splash-based pain.
        assert(PainProfile.m_bSplash == true);

        // Compute falloff distance to decay pain by.
        Vector3 PainVictimDelta = m_VictimPainPosition - m_Position;
        float   FalloffDist = PainVictimDelta.Length();

        // Trivially reject pain if too far away.
        if ((FalloffDist > PainProfile.m_DamageFarDist) && (FalloffDist > PainProfile.m_ForceFarDist)) {
            return false;
        }

        // Compute linear, parametric, clamped, falloff scalar
        DamageScalar = x_parametric(FalloffDist, PainProfile.m_DamageFarDist, PainProfile.m_DamageNearDist, true);
        ForceScalar = x_parametric(FalloffDist, PainProfile.m_ForceFarDist, PainProfile.m_ForceNearDist, true);

        // If a direct hit was provided then apply full pain
        if (m_DirectHitGuid && (m_DirectHitGuid == m_VictimGuid)) {
            DamageScalar = 1.0f;
            ForceScalar = 1.0f;
        }

        // Compute force direction
        m_ForceDirection = PainVictimDelta;
        m_ForceDirection.Normalize();
    } else {
        // Handle non-splash-based pain.
        assert(PainProfile.m_bSplash == false);

        // Set scalars to exactly what's in the tables
        DamageScalar = 1.0f;
        ForceScalar = 1.0f;

        // Use pain direction to build force direction
        m_ForceDirection = m_Direction;
    }

    // Apply team damage scalars
    m_bIsFriendlyFire = HandleFriendlyFire(pKillerObj, pVictimObj, DamageScalar, ForceScalar);

    // Apply LOS coverage
    DamageScalar *= (1 - m_LOSCoverage);
    ForceScalar *= (1 - m_LOSCoverage);

    // Force final scalars within range
    DamageScalar = std::clamp(DamageScalar, 0.0f, 1.0f);
    ForceScalar = std::clamp(ForceScalar, 0.0f, 1.0f);

    // Apply custom scalar.  It is applied here since it is allowed to be > 1.0f
    DamageScalar *= m_CustomScalar;
    ForceScalar *= m_CustomScalar;

    // Apply global scalar.  It is applied here since it is allowed to be > 1.0f
    DamageScalar *= GetTweakF32("Global_Damage_Scalar", 1.0f);

    // Apply scalars to table values
    m_Damage = PainHealthProfile.m_Damage * DamageScalar;
    m_Force = PainHealthProfile.m_Force * ForceScalar;

    // All trivial rejects have already happened.  Even if there is no
    // damage or force we still need to return true to objects can
    // react to being hit.
    return true;
}

void pain::ComputeLOSCoverage() const
{
    // This could be more intelligent and handle partial coverage.

    // Decide victim position
    Vector3 VictimPos = m_VictimPainPosition;
    {
        Object* VictimObject = objectManager->GetObjectByGuid(m_VictimGuid);
        if (VictimObject && VictimObject->IsKindOf(actor::GetRTTI())) {
            actor& actorVictim = actor::GetSafeType(*VictimObject);
            VictimPos = actorVictim.GetPositionWithOffset(actor::OFFSET_CENTER);
        }
    }

    Vector3 Distance = (VictimPos - m_Position);

    // if the distance is very small, no LOS coverage
    if (Distance.LengthSquared() < 1.0f) {
        m_LOSCoverage = 0.0f;
    } else {
        // Test ray from pain position to victim pain position
        g_CollisionMgr.LineOfSightSetup(0, m_Position, VictimPos);
        g_CollisionMgr.AddToIgnoreList(m_OriginGuid);
        g_CollisionMgr.AddToIgnoreList(m_VictimGuid);
        g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PAIN_LOS, (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));

        // set LOS coverage
        if (g_CollisionMgr.m_nCollisions) {
            m_LOSCoverage = 1.0f;
        } else {
            m_LOSCoverage = 0.0f;
        }
    }
}

bool pain::HandleFriendlyFire(Object* pKiller, Object* pVictim, float& DamageModifier, float& ForceModifier) const
{
    // If we don't have ptrs to the two parties we can't decide anything.
    if (!pKiller || !pVictim) {
        return false;
    }

    // Both need to be descendents of actor or we can't determine factions.
    if (!pKiller->IsKindOf(actor::GetRTTI()) ||
        !pVictim->IsKindOf(actor::GetRTTI())) {
        return false;
    }

    // Alright, treat these as actors
    if (((actor*)pVictim)->IsAlly((actor*)pKiller)) {
        //
        // They are friendly!
        //
        if ((pVictim == pKiller) && pKiller->IsKindOf(player::GetRTTI())) {
            // We are the player hurting ourselves, so we're not /really/ friendly
            return false;
        }

        // Scale damage down by whatever scale tweak we have
        tweak_handle ScaleTweak("FriendlyFireScalar");
        DamageModifier *= ScaleTweak.GetF32();

        return true;
    } else {
        //
        // They are enemies or neutral!
        //

        return false;
    }
}

bool pain::ApplyToWorld(bool bIgnoreOrigin)
{
    // Clear the return result
    bool bHitAnObject = false;

    //
    // Get reference to pain profile or just return if not available.
    //
    const pain_profile* pPainProfile = m_PainHandle.GetPainProfile();
    if (!pPainProfile) {
        return false;
    }
    const pain_profile& PainProfile = *pPainProfile;

    //
    // If ApplyToWorld() is called but this type of pain does not have splash
    // then apply the pain only to the DirectHitGuid if available.
    //
    if (PainProfile.m_bSplash == false) {
        if (m_DirectHitGuid) {
            return ApplyToObject(m_DirectHitGuid);
        }
        return false;
    }

    //
    // Setup a slot to ignore if we should ignore the origin object
    //
    slot_id IgnoreSlot = SLOT_NULL;
    if (bIgnoreOrigin) {
        IgnoreSlot = objectManager->GetSlotFromGuid(GetOriginGuid());
    }

    //
    // Collect all damagable objects in range of pain.  We are putting them into
    // an array because we can't handle nested SelectBBox and the OnPain response
    // of the object might need it.
    //
    const int MAX_DAMAGEABLE_OBJECTS = 64;
    slot_id   iSlot[MAX_DAMAGEABLE_OBJECTS];
    int       nSlots = 0;
    {
        objectManager->SelectBBox(Object::ATTR_DAMAGEABLE, m_PainBBox, Object::TYPE_ALL_TYPES);
        slot_id ID = objectManager->StartLoop();
        while ((ID != SLOT_NULL) && (nSlots < MAX_DAMAGEABLE_OBJECTS)) {
            if (ID != IgnoreSlot) {
                iSlot[nSlots] = ID;
                nSlots++;
            }
            ID = objectManager->GetNextResult(ID);
        }
        objectManager->EndLoop();
    }

    //
    // Loop through the objects and apply the pain
    //
    int i;
    for (i = 0; i < nSlots; i++) {
        Object* pObject = objectManager->GetObjectBySlot(iSlot[i]);
        if (pObject) {
            bHitAnObject = ApplyToObject(pObject) || bHitAnObject;
        }
    }

    return bHitAnObject;
}

bool pain::ApplyToObject(guid VictimGuid)
{
    Object* pObject = objectManager->GetObjectByGuid(VictimGuid);
    return ApplyToObject(pObject);
}

bool pain::ApplyToObject(Object* pObject)
{
    if (pObject) {
        // Does object have a parent?
        Object* pParentObject = objectManager->GetObjectByGuid(pObject->GetParentGuid());
        if (pParentObject) {
            // Give parent a chance to handle the pain
            if (pParentObject->OnChildPain(pObject->GetGuid(), *this)) {
                return true;
            }
        }

        // Parent didn't handle the pain, so apply to object
        pObject->OnPain(*this);
        return true;
    }
    return false;
}
