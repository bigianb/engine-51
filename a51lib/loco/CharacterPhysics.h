#pragma once

#include "../Property.h"
#include "../objects/Object.h"

class ObjectManager;
class collision_mgr;

#define NUM_STILL_FRAMES 15
class character_physics : public prop_interface
{
public:
    character_physics(ObjectManager* pObjectManager, collision_mgr* pCollisionMgr);
    ~character_physics() {}
    void Init(guid Guid);
    void ApplyCollision();

    void         SetPosition(const Vector3& Position) { m_Position = Position; }
    void         Advance(const Vector3& MoveTo, float DeltaTime, bool bIsDead = false);
    void         AdvanceWithoutCollision(const Vector3& MoveTo, float DeltaTime, bool bIsDead = false);
    Vector3      GetPosition() const;
    Vector3      GetVelocity() const;
    void         AddVelocity(const Vector3& Delta);
    void         SetVelocity(const Vector3& Velocity);
    void         ZeroVelocity();
    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);
    float        GetColHeight() const { return m_NavCollisionCurentHeight; }
    void         SetColHeight(float Height);
    float        GetColRadius() const { return m_NavCollisionRadius; }
    void         SetColRadius(float Radius);
    void         SetColCrouchOffset(float Offset) { m_NavCollisionCrouchOffset = Offset; }
    void         SetMaxCollsions(int Max) { m_MaxCollisions = Max; }
    BBox         GetBBox() const;
    bool         SetCrouchParametric(float NormalizePercent);
    void         Jump(float YVel);
    void         Fling(const Vector3& Velocity,
                       float          DeltaTime,
                       float          AirControl,
                       bool           FlingOnly,
                       bool           ReflingOnly,
                       bool           Instantaneous,
                       guid           LastFling);
    bool         Flung() { return m_bFlingMode; }
    void         SetUseGravity(bool bEnable) { m_bUseGravity = bEnable; }
    void         SetGravityAccel(const float& GravityAccel);
    float        GetGravityAccel();
    bool         GetFallMode();
    bool         GetJumpMode();
    void         SetAirControl(const float& AirControl) { m_AirControl = AirControl; }
    void         SetActorCollisionRadius(float newRadius) { m_ActorCollisionRadius = newRadius; }
    float        GetActorCollisionRadius() { return m_ActorCollisionRadius; }
    bool         GetNavCollided() const { return m_bNavCollided; }
    guid         GetGuid() { return m_Guid; }

    void    SetDeltaPos(Vector3& DeltaPos);
    Vector3 GetRecentDeltaPos();
    void    ResetDeltaPos();

    void SetLocoGravityOn(bool bValue) { m_bLocoGravityOn = bValue; }
    void SetLocoCollisionOn(bool bValue) { m_bLocoCollisionOn = bValue; }
    void SetHandlePermeable(bool bValue) { m_bHandlePermeable = bValue; }

    void SetSolveActorCollisions(bool doSolve) { m_SolveActorCollisions = doSolve; }

    void CopyValues(character_physics& rPhysics);

    void SolveActorAndPlatformCollisions();
    void CatchUpWithRidingPlatform(float DeltaTime);
    guid GetMovingPlatformGuid() const { return m_MovingPlatformGuid; }
    void WatchForRidingPlatform();
    void ResetRidingPlatforms();
    void Push(const Vector3& PushVector);
    void SetCollisionIgnoreGuid(guid ignoreGuid);

    bool IsAirborn() const;
    bool SetupPlayerCollisionCheck(const Vector3& Start, const Vector3& End);

    void SetGroundTracking(bool Track);
    void InitialGroundCheck(const Vector3& Position);

    //=========================================================================
protected:
    void HandleMove(Vector3& NewPos,
                    Vector3& OriginalVelocity,
                    float    DeltaTime,
                    float    SlideFriction,
                    float    SteepestSlide);

    bool UpdateGround(float DistBelow);
    void UpdatePhysics(float DeltaTime);
    void CollectPermeable(Object* pObject, Vector3& NewPos);
    void CollectPermeable(Object* pObject, const Vector3& StartPos, const Vector3& EndPos);
    void SolveActorCollisions();
    void SolveActorCollisions(const BBox& ActorBBox);
    void SolvePlatformCollisions();

    void ResolvePenetrations();

    //=========================================================================
protected:
    guid    m_Guid;
    float   m_NavCollisionHeight;
    float   m_NavCollisionRadius;
    float   m_NavCollisionCrouchOffset;
    float   m_NavCollisionCurentHeight;
    float   m_AirControl;
    float   m_FlingAC; // AirControl while in fling mode.
    int     m_MaxCollisions;
    bool    m_bHandlePermeable;
    bool    m_bNavCollided;
    bool    m_bFallMode;
    bool    m_bJumpMode;
    bool    m_bFlingMode;
    bool    m_bTrackingGround;
    bool    m_bUseGravity;
    bool    m_bLocoGravityOn;
    bool    m_bLocoCollisionOn;
    float   m_CollisionSnuggleDistance;
    Vector3 m_Velocity;
    Vector3 m_Position;
    float   m_GroundTolerance;
    float   m_FallTolerance;
    float   m_VelocityForFallMode;
    float   m_GravityAcceleration;
    float   m_MaxDistanceToGround;
    float   m_SteepestSlide;
    float   m_CrouchPercent;
    guid    m_IgnoreGuid;

    guid    m_MovingPlatformGuid;
    int     m_MovingPlatformBone;
    Matrix4 m_OldMovingPlatformL2W;
    Vector3 m_OldMovingPlatformVelocity;
    guid    m_PlatformCollisionIgnoreList[8];
    int     m_nPlatformsToIgnore;
    Vector3 m_LastMove;
    guid    m_LastFling;

    Vector3 m_LstDeltaPos[NUM_STILL_FRAMES];
    int     m_DeltaPosIndex;

    bool  m_SolveActorCollisions;
    float m_ActorCollisionRadius;

    guid    m_GroundGuid;
    plane   m_GroundPlane;
    Vector3 m_GroundPos;
    int     m_GroundBoneIndex;
    guid    m_LastSteepSurface;
    bool    m_bIsAirborn;

#define CHARACTER_PHYSICS_MAX_TRIGGERS 4
    guid m_TriggerGuid[CHARACTER_PHYSICS_MAX_TRIGGERS];
    int  m_nTriggerGuids;

    ObjectManager* m_pObjectManager;
    collision_mgr* collisionManager;
};

inline Vector3 character_physics::GetPosition() const
{
    return m_Position;
}

inline Vector3 character_physics::GetVelocity() const
{
    return m_Velocity;
}

inline void character_physics::AddVelocity(const Vector3& Delta)
{
    m_Velocity += Delta;
    if (m_Velocity.GetY() > 0) {
        m_bJumpMode = true;
    }
}

inline void character_physics::SetVelocity(const Vector3& Velocity)
{
    m_Velocity = Velocity;
}

inline void character_physics::ZeroVelocity()
{
    m_Velocity.Zero();
}

inline void character_physics::SetColHeight(float Height)
{
    m_NavCollisionHeight = Height;
    m_NavCollisionCurentHeight = Height;
}

inline void character_physics::SetColRadius(float Radius)
{
    m_NavCollisionRadius = Radius;
}

inline BBox character_physics::GetBBox() const
{
    BBox bb;
    bb.max.z = bb.max.x = GetColRadius();
    bb.min.z = bb.min.x = -bb.max.x;
    bb.max.y = GetColHeight();
    bb.min.y = 0;
    return bb;
}

//=========================================================================

inline void character_physics::SetGravityAccel(const float& fGravityAccel)
{
    m_GravityAcceleration = fGravityAccel;
}

//=========================================================================

inline float character_physics::GetGravityAccel()
{
    return m_GravityAcceleration;
}

//=========================================================================
inline bool character_physics::GetFallMode()
{
    return m_bFallMode;
}

//=========================================================================
inline bool character_physics::GetJumpMode()
{
    return m_bJumpMode;
}

//=========================================================================

inline bool character_physics::IsAirborn() const
{
    return (m_bFallMode || m_bJumpMode);
}

