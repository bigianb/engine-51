#pragma once

#include "Constraint.h"
#include <vector>
#include <list>
#include "../VectorMath.h"

class actor;
class rigid_body;
class physics_inst;
class ObjectManager;

class physics_mgr
{
public:
    struct collision
    {
        // Reported values
        Vector3 m_Position; // World position of collision
        Vector3 m_Normal;   // Normal of collision into shape0
        float   m_Depth;    // Penetration depth of collision

        rigid_body* m_pBody0;  // Rigid body0 or NULL
        rigid_body* m_pBody1;  // Rigid body1 or NULL
        actor*      m_pActor1; // Actor that body1 is attached to

        float m_Elasticity;      // Bouncy-ness of collision
        float m_StaticFriction;  // Friction when not moving
        float m_DynamicFriction; // Friction when moving

        // Computed values for each collision solve pass
        float   m_Denominator;      // Impulse denominator
        Vector3 m_R0;               // Position of collision relative to shape0
        Vector3 m_R1;               // Position of collision relative to shape1
        Vector3 m_PenetrationExtra; // Penetration penalty velocity
    };

    //==============================================================================
    // Functions
    //==============================================================================
public:
    physics_mgr();
    ~physics_mgr();

    // Initialization
    void Init();
    void ClearDeltaTime();
    void Kill();

    // Physics instance list management functions
    void AddInstance(physics_inst* pInstance);
    void RemoveInstance(physics_inst* pInstance);
    void WakeupInstance(physics_inst* pInstance);
    void PutToSleepInstance(physics_inst* pInstance);
    void CollisionWakeupInstance(physics_inst* pInstance);
    int  GetAwakeInstanceCount() const;

    // Collision functions
    int        GetNextCollisionGroup();
    collision& GetCollision(int Index);
    int        GetNCollisions() const;

    int AddCollision(rigid_body*    pBody0,
                     rigid_body*    pBody1,
                     const Vector3& Pos,
                     const Vector3& Dir,
                     const float&   Dist);

    void DetectWorldCollisions(physics_inst* pInst);
    void DetectSelfCollisions(physics_inst* pInst);
    void DetectInstInstCollisions(physics_inst* pInst);
    void DetectInstActorCollisions(physics_inst* pInst);
    void DetectCollisions(void);

    void PreApplyCollisions(float DeltaTime);
    bool  SolveCollision(collision& Collision);

    void PreApplyConstraints(float DeltaTime);
    void SolveCollisions(float DeltaTime, int nIterations, ObjectManager*);
    void SolveContacts(float DeltaTime, int nIterations, ObjectManager*);
    void ShockPropagation();

    // Logic functions
    void PutInstancesToSleep(float DeltaTime);
    void BuildActiveBodyList();
    void BuildActiveBodyAndConstraintList();
    void Step(float DeltaTime, ObjectManager*);
    void Advance(float DeltaTime, ObjectManager*);

public:
    // Settings
    struct settings
    {
        float   m_MaxTimeStep;   // Maximum time step of simulation
        int     m_nMaxTimeSteps; // Maximum # of steps to take
        Vector3 m_Gravity;       // World gravity

        int   m_nMaxCollisions;          // Maximum # of collisions
        int   m_nCollisionIterations;    // # of iterations to solve collision
        float m_CollisionHitBackoffDist; // Dist to backoff when a collision occurs
        bool  m_bInstInstCollision;      // Collide physics instance with others?
        bool  m_bSelfCollision;          // Collide bodies within physics instance?
        float m_PenetrationFix;          // Penetration fix scalar
        float m_MaxPenetrationFix;       // Max amount of penetration fix

        bool m_bSolveContacts;     // Solve contacts?
        int  m_nContactIterations; // # of iterations to solve contact

        bool m_bShock; // Do shock propagation?

        int   m_nMaxActiveConstraints; // Maximum # of active constraints
        float m_ConstraintFix;         // Constraint fix scalar
        float m_MaxConstraintFix;      // Max amount of constraint

        float m_ActiveLinearSpeed;   // Translation speed at which body is considered active
        float m_ActiveAngularSpeed;  // Rotation speed at which body is considered active
        bool  m_bDeactivate;         // Allow rigid bodies to deactivate
        float m_DeactivateStartTime; // Time to start blending to deactivate
        float m_DeactivateEndTime;   // Time before being fully deactivate

        float m_MaxLinearVel;  // Max linear velocity
        float m_MaxAngularVel; // Max angular velocity
        float m_MaxForce;      // Max allowed force

        int m_nRenderIterations; // # of render jitter fix iterations

        bool m_bShowStats;    // Show stats info?
        bool m_bDebugRender;  // Render debug info?
        bool m_bUsePolycache; // Use world collision or ground plane?
    };

public:
    // Settings
    settings m_Settings; // Settings

private:
    // Physics instance list
    std::list<physics_inst*> m_AwakeInstances;           // List of active instances
    std::list<physics_inst*> m_SleepingInstances;        // List of inactive instances
    std::list<physics_inst*> m_CollisionWakeupInstances; // List of instances woken up inside collision loop

    // Rigid body lists
    std::vector<rigid_body*> m_ActiveBodies; // List of active bodies

    // Constraint lists
    std::vector<active_constraint>  m_ActiveConstraints; // List of active constraints
    std::list<active_constraint*> m_SolveConstraints;  // List of constraints to solve

    // Collision
    int                    m_NextCollisionGroup; // Next collision group to allocate
    std::vector<collision> m_Collisions;         // List of collisions

    // Logic
    float m_DeltaTime; // Current delta time
};

//==============================================================================

inline void physics_mgr::ClearDeltaTime()
{
    m_DeltaTime = 0.0f;
}

//==============================================================================
// Physics instance list management functions
//==============================================================================

inline int physics_mgr::GetAwakeInstanceCount() const
{
    return m_AwakeInstances.size();
}

//==============================================================================
// Collision functions
//==============================================================================

inline int physics_mgr::GetNextCollisionGroup()
{
    return m_NextCollisionGroup++;
}

//==============================================================================

inline physics_mgr::collision& physics_mgr::GetCollision(int Index)
{
    return m_Collisions[Index];
}

//==============================================================================

inline int physics_mgr::GetNCollisions() const
{
    return m_Collisions.size();
}

extern collision_shape g_WorldColl; // World collision shape
extern rigid_body      g_WorldBody; // World rigid body

extern collision_shape g_ActorColl;       // Actor collision shape
extern rigid_body      g_ActorBody;       // Actor rigid body
extern actor*          g_pCollisionActor; // Current actor being tested against

extern physics_mgr g_PhysicsMgr; // Global physics mgr
