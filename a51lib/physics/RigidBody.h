#pragma once

#include "CollisionShape.h"

class constraint;

class rigid_body
{
public:
    // Flags
    enum flags
    {
        FLAG_WORLD_COLLISION = (1 << 0), // TRUE if body should collide with world
        FLAG_IS_ACTIVE = (1 << 1),       // TRUE if body is moving
        FLAG_HAS_COLLIDED = (1 << 2),    // TRUE if body has collided
    };

public:
    // State of rigid body
    struct state
    {
        Matrix4 m_L2W;
        Vector3 m_LinearVelocity;
        Vector3 m_AngularVelocity;
    };

    //==============================================================================
    // Functions
    //==============================================================================
public:
    rigid_body();
    ~rigid_body();

    // Property functions
    float GetMass() const;
    void  SetMass(float Mass);
    void  SetInfiniteMass();
    float GetInvMass() const;

    float GetElasticity() const;
    float GetStaticFriction() const;
    float GetDynamicFriction() const;

    void SetElasticity(float Elasticity);
    void SetStaticFriction(float StaticFriction);
    void SetDynamicFriction(float DynamicFriction);

    // Transform functions
    const Matrix4& GetL2W() const;
    const Matrix4& GetPrevL2W() const;
    Matrix4        GetW2L() const;
    void           SetPrevL2W(const Matrix4& L2W);
    void           SetL2W(const Matrix4& L2W);

    void    SetPosition(const Vector3& Position);
    Vector3 GetPosition() const;

    void ComputeWorldInvInertia();

    // Bounding box functions
    void        ComputeWorldBBox();
    const BBox& GetWorldBBox() const;

    // Inertia tensor functions
    void           SetBodyInertia(const Vector3& I);
    const Matrix4& GetWorldInvInertia() const;

    // Velocity functions
    void           ClampVelocities();
    void           SetLinearVelocity(const Vector3& Velocity);
    const Vector3& GetLinearVelocity() const;
    Vector3&       GetLinearVelocity();

    void           SetAngularVelocity(const Vector3& Velocity);
    const Vector3& GetAngularVelocity() const;
    Vector3&       GetAngularVelocity();

    void SetLinearDamping(float LinearDamping);
    void SetAngularDamping(float AngularDamping);

    void ZeroLinearVelocity();
    void ZeroAngularVelocity();
    void ZeroVelocity();

    // State backup functions
    void GetState(state& State) const;
    void SetPosition(const state& State);
    void SetVelocity(const state& State);

    // Force/torque functions
    void ClearForces();
    void ComputeForces(float DeltaTime);

    void AddWorldForce(const Vector3& Force);
    void AddWorldForce(const Vector3& Force, const Vector3& Position);

    void AddWorldTorque(const Vector3& Torque);
    void AddWorldTorque(const Vector3& Torque, const Vector3& Position);

    void ApplyWorldImpulse(const Vector3& Impulse);
    void ApplyWorldImpulse(const Vector3& Impulse, const Vector3& Position);
    void ApplyLocalImpulse(const Vector3& Impulse, const Vector3& Position);

    // Integration functions
    void IntegratePosition(float DeltaTime);
    void IntegrateVelocity(float DeltaTime);

    // Active functions
    void  UpdateActiveState(float DeltaTime);
    bool  HasActiveEnergy() const;
    bool  IsActive() const;
    float GetActiveBlend() const;
    void  Deactivate();
    void  Activate();

    // Collision functions
    void             SetCollisionShape(collision_shape* pCollisionShape, float Mass, float InertiaMax);
    collision_shape* GetCollisionShape() const;
    void             SetCollisionInfo(int Group, int ID, uint32_t Mask);
    int              GetCollisionID() const;
    bool             IsCollisionEnabled(rigid_body* pBody);
    void             SetWorldCollision(bool bEnable);
    void             CollisionWakeup();

    // Collision impact query functions
    void  ClearCollision();
    bool  HasCollided() const;
    float GetCollisionSpeedSqr() const;

    // Constraint functions
    void        SetPivotConstraint(constraint* pPivotConstraint);
    constraint* GetPivotConstraint() const;

public:
    std::vector<rigid_body> m_ActiveListNode; // Physics mgr active list node

protected:
    // Constant properties
    float            m_Mass;            // Mass
    float            m_InvMass;         // 1/Mass
    Vector3          m_BodyInvInertia;  // Body space inverse inertia (diagonal of matrix)
    float            m_Elasticity;      // Bouncyness
    float            m_StaticFriction;  // Friction when not moving
    float            m_DynamicFriction; // Friction when moving
    float            m_LinearDamping;   // Linear velocity damping
    float            m_AngularDamping;  // Angular velocity damping
    collision_shape* m_pCollisionShape; // Pointer to collision shape

    // Status information
    mutable uint32_t m_Flags;        // General flags
    float            m_InactiveTime; // Time body has had below active energy

    // State variables
    mutable Matrix4 m_L2W;       // Local to world transform
    mutable BBox    m_WorldBBox; // World space bounding box

    // Derived quantities (auxiliary variables)
    Matrix4 m_WorldInvInertia; // World space inverse inertia tensor
    Vector3 m_LinearVelocity;  // Linear (translation) velocity
    Vector3 m_AngularVelocity; // Angular (rotation) velocity

    // Computed quantities about center of mass
    Vector3 m_Force;  // Accumulated force to apply
    Vector3 m_Torque; // Accumulated torque to apply

    // Backup state
    state m_BackupState; // Back of world transform and velocity

    // Collision info
    int      m_CollisionGroup;    // Group ID (bodies in same instance have same)
    int      m_CollisionID;       // Collision index
    uint32_t m_CollisionBit;      // Collision bit mask ( 1 << m_CollisionID )
    uint32_t m_CollisionMask;     // Collision mask (Mask & Bit) = Collision?
    float    m_CollisionSpeedSqr; // Biggest collision speed squared (if collision happened)

    // Constraint info
    constraint* m_pPivotConstraint; // Ptr to pivot constraint (or NULL)

    friend class physics_mgr;
    friend class physics_inst;
    friend class collider;
};

// IJB typedef std::vector<rigid_body, offsetof(rigid_body, m_ActiveListNode)> rigid_body_active_list;

inline rigid_body::~rigid_body()
{
}

inline float rigid_body::GetMass() const
{
    return m_Mass;
}

//==============================================================================

inline void rigid_body::SetMass(float Mass)
{
    assert(Mass > 0.0f);

    // Record mass and inverse mass
    m_Mass = Mass;
    m_InvMass = 1.0f / Mass;
}

//==============================================================================

inline void rigid_body::SetInfiniteMass()
{
    // Set to non-movable and infinite mass
    m_Mass = 0.0f;
    m_InvMass = 0.0f;
    m_BodyInvInertia.Zero();
    m_WorldInvInertia.Zero();
}

//==============================================================================

inline float rigid_body::GetInvMass() const
{
    return m_InvMass;
}

//==============================================================================

inline float rigid_body::GetElasticity() const
{
    return m_Elasticity;
}

//==============================================================================

inline float rigid_body::GetStaticFriction() const
{
    return m_StaticFriction;
}

//==============================================================================

inline void rigid_body::SetElasticity(float Elasticity)
{
    m_Elasticity = Elasticity;
}

//==============================================================================

inline void rigid_body::SetStaticFriction(float StaticFriction)
{
    m_StaticFriction = StaticFriction;
}

//==============================================================================

inline void rigid_body::SetDynamicFriction(float DynamicFriction)
{
    m_DynamicFriction = DynamicFriction;
}

//==============================================================================
// Transform functions
//==============================================================================

inline const Matrix4& rigid_body::GetL2W() const
{
    return m_L2W;
}

//==============================================================================

inline const Matrix4& rigid_body::GetPrevL2W() const
{
    return m_BackupState.m_L2W;
}

//==============================================================================

inline Matrix4 rigid_body::GetW2L() const
{
    return m4_InvertRT(GetL2W());
}

//==============================================================================

inline void rigid_body::ComputeWorldBBox()
{
    // Grab from collision shape?
    if (m_pCollisionShape) {
        // Use world bbox of collision shape
        m_WorldBBox = m_pCollisionShape->ComputeWorldBBox();
    } else {
        // Setup default
        m_WorldBBox.Set(GetPosition(), 1.0f);
    }
}

//==============================================================================

inline void rigid_body::SetBodyInertia(const Vector3& I)
{
    // Set inverse body space inertia
    m_BodyInvInertia.set(1.0f / I.GetX(),
                         1.0f / I.GetY(),
                         1.0f / I.GetZ());
}

//==============================================================================

inline void rigid_body::SetPosition(const Vector3& Position)
{
    m_L2W.SetTranslation(Position);
}

//==============================================================================

inline Vector3 rigid_body::GetPosition() const
{
    return m_L2W.GetTranslation();
}

//==============================================================================

inline void rigid_body::ComputeWorldInvInertia()
{
    // Compute orientation and inverse orientation (no translation)
    Matrix4 Orient = m_L2W;
    Orient.ClearTranslation();
    Matrix4 InvOrient = m4_Transpose(Orient);

    // Compute world space inverse as follows (operations read from right->left):
    // m_WorldInvInertia = InvOrient * BodyInvInertia * Orient

    // Since the "BodyInvInertia" only has it's diagonal elements set,
    // the computation can be optimized to this:
    Orient.Scale(m_BodyInvInertia); // ie. BodyInvInertia * Orient
    m_WorldInvInertia = InvOrient * Orient;
}

//==============================================================================
// Bounding box functions
//==============================================================================

inline const BBox& rigid_body::GetWorldBBox() const
{
    return m_WorldBBox;
}

//==============================================================================

inline float rigid_body::GetDynamicFriction() const
{
    return m_DynamicFriction;
}

//==============================================================================
// Inertia tensor functions
//==============================================================================

inline const Matrix4& rigid_body::GetWorldInvInertia() const
{
    return m_WorldInvInertia;
}

//==============================================================================
// Velocity functions
//==============================================================================

inline void rigid_body::SetLinearVelocity(const Vector3& Velocity)
{
    m_LinearVelocity = Velocity;
}

//==============================================================================

inline const Vector3& rigid_body::GetLinearVelocity() const
{
    return m_LinearVelocity;
}

//==============================================================================

inline Vector3& rigid_body::GetLinearVelocity()
{
    return m_LinearVelocity;
}

//==============================================================================

inline void rigid_body::SetAngularVelocity(const Vector3& Velocity)
{
    m_AngularVelocity = Velocity;
}

//==============================================================================

inline const Vector3& rigid_body::GetAngularVelocity() const
{
    return m_AngularVelocity;
}

//==============================================================================

inline Vector3& rigid_body::GetAngularVelocity()
{
    return m_AngularVelocity;
}

//==============================================================================

inline void rigid_body::SetLinearDamping(float LinearDamping)
{
    m_LinearDamping = LinearDamping;
}

//==============================================================================

inline void rigid_body::SetAngularDamping(float AngularDamping)
{
    m_AngularDamping = AngularDamping;
}

//==============================================================================

inline void rigid_body::ZeroLinearVelocity()
{
    m_LinearVelocity.Zero();
}

//==============================================================================

inline void rigid_body::ZeroAngularVelocity()
{
    m_AngularVelocity.Zero();
}

//==============================================================================

inline void rigid_body::ZeroVelocity()
{
    m_LinearVelocity.Zero();
    m_AngularVelocity.Zero();
}

//==============================================================================
// State backup functions
//==============================================================================

inline void rigid_body::GetState(rigid_body::state& State) const
{
    // Store
    State.m_L2W = GetL2W();
    State.m_LinearVelocity = m_LinearVelocity;
    State.m_AngularVelocity = m_AngularVelocity;
}

//==============================================================================

inline void rigid_body::SetPosition(const rigid_body::state& State)
{
    // Restore position
    SetL2W(State.m_L2W);
}

//==============================================================================

inline void rigid_body::SetVelocity(const rigid_body::state& State)
{
    // Restore
    m_LinearVelocity = State.m_LinearVelocity;
    m_AngularVelocity = State.m_AngularVelocity;
}

//==============================================================================

inline void rigid_body::SetPrevL2W(const Matrix4& L2W)
{
    m_BackupState.m_L2W = L2W;
}

//==============================================================================
// Force/torque functions
//==============================================================================

inline void rigid_body::ClearForces()
{
    m_Force.Zero();
    m_Torque.Zero();
}

//==============================================================================

inline void rigid_body::AddWorldForce(const Vector3& Force)
{
    m_Force += Force;
}

//==============================================================================

inline void rigid_body::AddWorldForce(const Vector3& Force, const Vector3& Position)
{
    m_Force += Force;
    m_Torque += v3_Cross(Position - GetPosition(), Force);
}

//==============================================================================

inline void rigid_body::AddWorldTorque(const Vector3& Torque)
{
    m_Torque += Torque;
}

//==============================================================================

inline void rigid_body::AddWorldTorque(const Vector3& Torque, const Vector3& Position)
{
    m_Torque += Torque;
    m_Force += v3_Cross(Position - GetPosition(), Torque);
}

//==============================================================================

inline void rigid_body::ApplyWorldImpulse(const Vector3& Impulse)
{
    m_LinearVelocity += Impulse * m_InvMass;
}

//==============================================================================

inline void rigid_body::ApplyWorldImpulse(const Vector3& Impulse, const Vector3& Position)
{
    m_LinearVelocity += Impulse * m_InvMass;
    m_AngularVelocity += m_WorldInvInertia.RotateVector(v3_Cross(Position - GetPosition(), Impulse));
}

//==============================================================================

inline void rigid_body::ApplyLocalImpulse(const Vector3& Impulse, const Vector3& Position)
{
    m_LinearVelocity += Impulse * m_InvMass;
    m_AngularVelocity += m_WorldInvInertia.RotateVector(v3_Cross(Position, Impulse));
}

//==============================================================================
// Active functions
//==============================================================================

inline bool rigid_body::IsActive() const
{
    return (m_Flags & FLAG_IS_ACTIVE) != 0;
}

//==============================================================================
// Collision functions
//==============================================================================

inline collision_shape* rigid_body::GetCollisionShape() const
{
    return m_pCollisionShape;
}

//==============================================================================

inline void rigid_body::SetCollisionInfo(int Group, int ID, uint32_t Mask)
{
    m_CollisionGroup = Group;
    m_CollisionID = ID;
    m_CollisionBit = 1 << ID;
    m_CollisionMask = Mask;
}

//==============================================================================

inline int rigid_body::GetCollisionID() const
{
    return m_CollisionID;
}

//==============================================================================

inline bool rigid_body::IsCollisionEnabled(rigid_body* pBody)
{
    // Should only call for rigid bodies in the same group
    assert(this != pBody);
    assert(m_CollisionGroup == pBody->m_CollisionGroup);

    // Check masks
    if (m_CollisionMask & pBody->m_CollisionBit) {
        assert(pBody->m_CollisionMask & m_CollisionBit);
        return true;
    } else {
        assert(!(pBody->m_CollisionMask & m_CollisionBit));
        return false;
    }
}

//==============================================================================

inline void rigid_body::SetWorldCollision(bool bEnable)
{
    if (bEnable) {
        m_Flags |= FLAG_WORLD_COLLISION;
    } else {
        m_Flags &= ~FLAG_WORLD_COLLISION;
    }
}

//==============================================================================

inline void rigid_body::CollisionWakeup()
{
    // Sleeping?
    if ((m_Flags & rigid_body::FLAG_IS_ACTIVE) == 0) {
        // Grab state ready for physics solving
        GetState(m_BackupState);

        // Make as active (leave velocities so it can be put back to sleep ASAP)
        m_Flags |= rigid_body::FLAG_IS_ACTIVE;
    }
}

//==============================================================================

inline void rigid_body::ClearCollision()
{
    m_Flags &= ~FLAG_HAS_COLLIDED;
    m_CollisionSpeedSqr = 0.0f;
}

//==============================================================================

inline bool rigid_body::HasCollided() const
{
    return (m_Flags & (FLAG_IS_ACTIVE | FLAG_HAS_COLLIDED)) == (FLAG_IS_ACTIVE | FLAG_HAS_COLLIDED);
}

//==============================================================================

inline float rigid_body::GetCollisionSpeedSqr() const
{
    return m_CollisionSpeedSqr;
}

//==============================================================================
// Constraint functions
//==============================================================================

inline void rigid_body::SetPivotConstraint(constraint* pPivotConstraint)
{
    m_pPivotConstraint = pPivotConstraint;
}

//==============================================================================

inline constraint* rigid_body::GetPivotConstraint() const
{
    return m_pPivotConstraint;
}
