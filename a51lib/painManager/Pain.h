#pragma once

#include "PainMgr.h"
#include "../collisionMgr/CollisionMgr.h"
#include "../Guid.h"
#include "../VectorMath.h"
#include "../RTTI.h"

class Object;

class pain
{
public:
    pain(ObjectManager* om);
    ~pain();

    // This routine clears the pain class and initializes it with the basics
    void Setup(pain_handle PainID, guid OriginGuid, const Vector3& Position);
    void Setup(const char* PainDesc, guid OriginGuid, const Vector3& Position);

    // Status routines
    bool SetupCalled() const;
    bool ComputeDamageAndForceCalled() const;

    // These routines provide additional information (if available) to later inquiries
    void SetDirection(const Vector3& Direction);
    void SetDirectHitGuid(guid DirectHitGuid);
    void SetCustomScalar(float CustomScalar);
    void SetCollisionInfo(const collision_mgr::collision& CollisionInfo);
    void SetAnimEventID(int AnimEventID);

    // Distribute pain to victims.  Returns TRUE if any Object's OnPain() was called.
    bool ApplyToWorld(bool bIgnoreOrigin = false);
    bool ApplyToObject(guid VictimGuid);
    bool ApplyToObject(Object* pVictimObject);

    // Queries available after Setup()
    pain_handle        GetPainHandle() const;
    pain_health_handle GetPainHealthHandle() const;
    guid               GetOriginGuid() const;
    const rtti&        GetOriginRTTI() const;
    const Vector3&     GetPosition() const;
    const Vector3&     GetDirection() const;
    float              GetCustomScalar() const;
    guid               GetDirectHitGuid() const;
    int                GetAnimEventID() const;
    const Vector3&     GetImpactPoint() const;
    const Vector3&     GetImpactNormal() const;
    bool               IsDamageComputed() const;

    // The collision info provides a number of useful values
    // - ObjectHit guid
    // - PrimitiveKey for triangle index or character bone index
    // - Flags for triangle material type
    // - collision point and plane.  These area mirrored in ImpactPoint/Normal

    bool                            HasCollision() const; // Defaults to FALSE
    const collision_mgr::collision& GetCollision() const; // Asserts if not available

    //==========================================================================
    // PAIN RESOLUTION
    //==========================================================================

    // Returns TRUE if any damage or force should be applied
    bool ComputeDamageAndForce(const char* HealthDesc, guid VictimGuid, const Vector3& VictimPainPosition) const;
    bool ComputeDamageAndForce(health_handle HealthID, guid VictimGuid, const Vector3& VictimPainPosition) const;

    // Queries available after ComputeDamageAndForce().
    float          GetDamage() const;         // Health to subtract from Object
    float          GetForce() const;          // Change in speed after pain is applied (cm/sec)
    const Vector3& GetForceDirection() const; // Normalized direction
    Vector3        GetForceVelocity() const;  // ForceDirection * Force
    bool           IsDirectHit() const;
    guid           GetVictimGuid() const;
    const rtti&    GetVictimRTTI() const;
    health_handle  GetHealthHandle() const;
    const Vector3& GetVictimPainPosition() const;
    int            GetHitType() const;     // Custom hit-type as listed in HitTypeTable
    bool           IsFriendlyFire() const; // Were the Origin & Victim friends?
    float          GetLOSCoverage() const; // 1.0-FullCovered, 0.0-ClearLOS

private:
    void Clear();
    void SetupDefaults();
    void ComputeLOSCoverage() const;
    bool HandleFriendlyFire(Object* pKiller, Object* pVictim, float& DamageModifier, float& ForceModifier) const;

private:
    // Values set by Setup()
    pain_handle              m_PainHandle;
    Vector3                  m_Position;
    BBox                     m_PainBBox;
    guid                     m_OriginGuid;
    Vector3                  m_Direction;
    guid                     m_DirectHitGuid;
    float                    m_CustomScalar;
    int                      m_AnimEventID;
    Vector3                  m_ImpactPoint;
    Vector3                  m_ImpactNormal;
    const rtti*              m_pOriginRTTI;
    collision_mgr::collision m_Collision;

    ObjectManager* objectManager;

    // Values set by ComputeDamageAndForce()
    mutable float              m_Force;
    mutable float              m_Damage;
    mutable Vector3            m_ForceDirection;
    mutable health_handle      m_HealthHandle;
    mutable pain_health_handle m_PainHealthHandle;
    mutable guid               m_VictimGuid;
    mutable Vector3            m_VictimPainPosition;
    mutable int                m_HitType;
    mutable const rtti*        m_pVictimRTTI;
    mutable float              m_LOSCoverage;

    // General flags
    mutable uint32_t m_bSetupCalled : 1,
        m_bComputeDamageAndForceCalled : 1,
        m_bCollisionAvailable : 1,
        m_bIsFriendlyFire : 1,
        m_bDirectHit : 1;
};
