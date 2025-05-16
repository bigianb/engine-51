#pragma once

#include "../animation/animData.h"
#include "../objects/render/SkinInst.h"
#include "../animation/SMemMatrixCache.h"
#include "../collisionMgr/CollisionMgr.h"

#include "RigidBody.h"
#include "CollisionShape.h"
#include "Constraint.h"

class rigid_body;
class constraint;
class collision_shape;
class loco_char_anim_player;

class physics_inst
{
public:
    // Constructor/destructor
    physics_inst();
    ~physics_inst();

    // Initialization functions
private:
    bool Init(bool bPopFix, float ConstraintBlendTime);

public:
    bool Init(const char* pGeomName, bool bPopFix, float ConstraintBlendTime = 0.0f);
    bool Init(const skin_inst& SkinInst, bool bPopFix, float ConstraintBlendTime = 0.0f);
    void Kill();

    // Render functions
    void Render(uint32_t Flags, Colour Ambient);

    // Position functions
    Vector3 GetPosition() const;

    // Active functions
    bool  IsActive() const;
    float GetSpeedSqr() const;
    bool  HasActiveEnergy() const;
    void  Deactivate();
    void  Activate();
    void  SetActiveWhenVisible(bool bActive);
    void  SetKeepActiveTime(float Time);

    // Matrix functions
    void           DirtyMatrices();
    const Matrix4* GetBoneL2Ws(uint64_t& LODMask, int& nActiveBones);
    Vector3        GetBoneWorldPosition(int iBone);  // NOTE: Bind has been removed!
    Matrix4        GetBoneWorldTransform(int iBone); // NOTE: Bind has been removed!
    void           SetMatrices(loco_char_anim_player& AnimPlayer, const Vector3& Vel);
    void           SetMatrices(const Matrix4* pMatrices, int nBones, bool bInheritVel);

    // Blast/force functions
    void ApplyBlast(const Vector3& Pos, float Radius, float Amount);
    void ApplyBlast(const Vector3& Pos, const Vector3& Dir, float Radius, float Amount);
    void ApplyVectorForce(const Vector3& Dir, float Amount);

    // Collision functions
    void        OnColCheck(guid OwnerObject);
    void        ComputeWorldBBox();
    const BBox& GetWorldBBox() const;

    // Query functions
    AnimGroup::handle& GetAnimGroupHandle();
    skin_inst&         GetSkinInst();
    const skin_inst&   GetSkinInst() const;
    std::string        GetAnimName() const;
    std::string        GetSkinGeomName() const;

    // Rigid body functions
    int         GetNRigidBodies() const;
    rigid_body& GetRigidBody(int iRigidBody);
    const char* GetRigidBodyName(int iRigidBody) const;

    // Collision functions
    int              GetNCollisionShapes() const;
    collision_shape& GetCollisionShape(int Index);
    void             SetActorCollision(bool bEnable);
    bool             GetActorCollision() const;
    void             SetWorldCollision(bool bEnable);
    bool             GetWorldCollision() const;
    void             SetInstCollision(bool bEnable);
    bool             GetInstCollision() const;

    // Constraint functions
    int         GetNBodyBodyConstraints() const;
    constraint& GetBodyBodyConstraint(int Index);

    int         GetNBodyWorldConstraints() const;
    constraint& GetBodyWorldConstraint(int Index);

    int AddBodyWorldConstraint(int            iRigidBody,
                               const Vector3& WorldPos,
                               float          MaxDist);

    void SetBodyWorldConstraintWorldPos(int iConstraint, const Vector3& WorldPos);

    void DeleteAllBodyWorldConstraints();

    void  SetConstraintWeight(float Weight);
    float GetConstraintWeight() const;

    // Zone functions
    void    SetZone(uint8_t Zone);
    uint8_t GetZone() const;

    // Logic functions
    void Advance(float DeltaTime);

    //==============================================================================
    // Data
    //==============================================================================

private:
    // Flags
    int m_bInitialized : 1;           // Initialized
    int m_bInAwakeList : 1;           // In physics mgr awake list
    int m_bInSleepingList : 1;        // In physics mgr sleeping list
    int m_bInCollisionWakeupList : 1; // In physics mgr collision wake up list
    int m_bPopFix : 1;                // Use pop fix matrices?
    int m_bActorCollision : 1;        // Should collision with actors occur?
    int m_bWorldCollision : 1;        // Should instance collides with the world?
    int m_bInstCollision : 1;         // Should instance collide with other instances?
    int m_bActiveWhenVisible : 1;     // Bodies are always active when visible

protected:
    // Physics components
    std::vector<rigid_body>      m_RigidBodies;           // List of rigid bodies
    std::vector<collision_shape> m_CollisionShapes;       // List of collision shapes
    std::vector<constraint>      m_BodyBodyContraints;    // List of body -> body constraints
    std::vector<constraint>      m_BodyWorldContraints;   // List of body -> world constraints
    std::vector<Matrix4>         m_PopFixMatrices;        // Matrices to fix pop
    BBox                         m_WorldBBox;             // World bounding box
    float                        m_ConstraintWeight;      // Current constraint strength
    float                        m_ConstraintWeightDelta; // Use to blend in constraints
    float                        m_KeepActiveTime;        // Timer to force bodies to be active
    uint8_t                      m_Zone;                  // Game zone (used in col detection)

    // Rendering components
    AnimGroup::handle m_hAnimGroup;  // Animation group handle
    skin_inst         m_SkinInst;    // Skinned instance
    smem_matrix_cache m_MatrixCache; // Matrix allocation class

    //==============================================================================
    // Friends
    //==============================================================================
    friend class rigid_body;
    friend class physics_mgr;
    friend class collider;
};

//==============================================================================
// TYPES
//==============================================================================

typedef std::vector<physics_inst*> physics_inst_list;

//==============================================================================
// INLINE FUNCTIONS
//==============================================================================

inline const BBox& physics_inst::GetWorldBBox() const
{
    return m_WorldBBox;
}

//==============================================================================

inline AnimGroup::handle& physics_inst::GetAnimGroupHandle()
{
    return m_hAnimGroup;
}

//==============================================================================

inline skin_inst& physics_inst::GetSkinInst()
{
    return m_SkinInst;
}

//==============================================================================

inline const skin_inst& physics_inst::GetSkinInst() const
{
    return m_SkinInst;
}

//==============================================================================

inline std::string physics_inst::GetAnimName() const
{
    return m_hAnimGroup.getName();
}

//==============================================================================

inline std::string physics_inst::GetSkinGeomName() const
{
    return m_SkinInst.GetSkinGeomName();
}

//==============================================================================
// Rigid body functions
//==============================================================================

inline int physics_inst::GetNRigidBodies() const
{
    return m_RigidBodies.size();
}

//==============================================================================

inline rigid_body& physics_inst::GetRigidBody(int iRigidBody)
{
    return m_RigidBodies[iRigidBody];
}

//==============================================================================
// Collision functions
//==============================================================================

inline int physics_inst::GetNCollisionShapes() const
{
    return m_CollisionShapes.size();
}

//==============================================================================

inline collision_shape& physics_inst::GetCollisionShape(int Index)
{
    return m_CollisionShapes[Index];
}

//==============================================================================

inline void physics_inst::SetActorCollision(bool bEnable)
{
    m_bActorCollision = bEnable;
}

//==============================================================================

inline bool physics_inst::GetActorCollision() const
{
    return m_bActorCollision;
}

//==============================================================================

inline void physics_inst::SetWorldCollision(bool bEnable)
{
    m_bWorldCollision = bEnable;
}

//==============================================================================

inline bool physics_inst::GetWorldCollision() const
{
    return m_bWorldCollision;
}

//==============================================================================

inline void physics_inst::SetInstCollision(bool bEnable)
{
    m_bInstCollision = bEnable;
}

//==============================================================================

inline bool physics_inst::GetInstCollision() const
{
    return m_bInstCollision;
}

//==============================================================================
// Constraint functions
//==============================================================================

inline int physics_inst::GetNBodyBodyConstraints() const
{
    return m_BodyBodyContraints.size();
}

//==============================================================================

inline constraint& physics_inst::GetBodyBodyConstraint(int Index)
{
    return m_BodyBodyContraints[Index];
}

//==============================================================================

inline int physics_inst::GetNBodyWorldConstraints() const
{
    return m_BodyWorldContraints.size();
}

//==============================================================================

inline constraint& physics_inst::GetBodyWorldConstraint(int Index)
{
    return m_BodyWorldContraints[Index];
}

//==============================================================================

inline void physics_inst::SetBodyWorldConstraintWorldPos(int iConstraint, const Vector3& WorldPos)
{
    // Lookup constraint
    constraint& Constraint = GetBodyWorldConstraint(iConstraint);

    // Update world position
    // (since body1 is the world rigid body which is at the origin (0,0,0) and has zero rotation (0,0,0)
    //  - we can just set the body position directly)
    Constraint.SetBodyPos(1, WorldPos);
}

//==============================================================================

inline void physics_inst::DeleteAllBodyWorldConstraints()
{
    // Clear list
    m_BodyWorldContraints.clear();
}

//==============================================================================

inline void physics_inst::SetConstraintWeight(float Weight)
{
    m_ConstraintWeight = Weight;
}

//==============================================================================

inline float physics_inst::GetConstraintWeight() const
{
    return m_ConstraintWeight;
}

//==============================================================================
// Active functions
//==============================================================================

inline bool physics_inst::IsActive() const
{
    return m_bInAwakeList;
}

//==============================================================================

inline void physics_inst::SetActiveWhenVisible(bool bActive)
{
    m_bActiveWhenVisible = bActive;
}

//==============================================================================

inline void physics_inst::SetKeepActiveTime(float Time)
{
    m_KeepActiveTime = Time;
}

//==============================================================================
// Zone functions
//==============================================================================

inline void physics_inst::SetZone(uint8_t Zone)
{
    m_Zone = Zone;
}

//==============================================================================

inline uint8_t physics_inst::GetZone() const
{
    return m_Zone;
}
