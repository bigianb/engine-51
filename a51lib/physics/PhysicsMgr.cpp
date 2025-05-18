
#include "PhysicsMgr.h"
#include "Collider.h"
#include "CollisionShape.h"
#include "PhysicsInst.h"
#include "../objects/Actor.h"
#include "../objectManager/ObjectManager.h"

#include <cassert>

//==============================================================================
// GLOBAL DATA
//==============================================================================
collision_shape g_WorldColl; // World collision shape
rigid_body      g_WorldBody; // World rigid body

collision_shape g_ActorColl;                 // Actor collision shape
rigid_body      g_ActorBody;                 // Actor rigid body
actor*          g_pCollisionActor = nullptr; // Current actor being tested against

physics_mgr g_PhysicsMgr; // Global physics manager

//==============================================================================
// SORT FUNCTIONS
//==============================================================================

// Sorts collisions from lowest to highest
int collision_sort_lo_to_hi(const void* pItem1, const void* pItem2)
{
    // Lookup collision struct pointers
    physics_mgr::collision* pColl0 = (physics_mgr::collision*)pItem1;
    physics_mgr::collision* pColl1 = (physics_mgr::collision*)pItem2;

    // Lookup world collision Y
    float Y0 = pColl0->m_Position.GetY();
    float Y1 = pColl1->m_Position.GetY();

    // Sort from lowest to highest
    if (Y0 > Y1) {
        return 1;
    } else if (Y0 < Y1) {
        return -1;
    } else {
        return 0;
    }
}

//==============================================================================

// Sorts collisions from highest to lowest
int collision_sort_hi_to_lo(const void* pItem1, const void* pItem2)
{
    // Lookup collision struct pointers
    physics_mgr::collision* pColl0 = (physics_mgr::collision*)pItem1;
    physics_mgr::collision* pColl1 = (physics_mgr::collision*)pItem2;

    // Lookup world collision Y
    float Y0 = pColl0->m_Position.GetY();
    float Y1 = pColl1->m_Position.GetY();

    // Sort from highest to lowest
    if (Y0 > Y1) {
        return -1;
    } else if (Y0 < Y1) {
        return 1;
    } else {
        return 0;
    }
}

//==============================================================================
// PHYSICS FUNCTIONS
//==============================================================================

physics_mgr::physics_mgr()
{
    // Settings
    m_Settings.m_MaxTimeStep = 1.0f / 29.0f;                // Maximum time step of simulation
    m_Settings.m_nMaxTimeSteps = 1;                         // Maximum # of steps to take
    m_Settings.m_Gravity.set(0, -9.81f * 100.0f * 2.0f, 0); // World gravity

    m_Settings.m_nMaxCollisions = 400;           // Maximum # of collisions
    m_Settings.m_nCollisionIterations = 4;       // # of iterations to solve collision
    m_Settings.m_CollisionHitBackoffDist = 1.0f; // Dist to backoff when a collision occurs
    m_Settings.m_bInstInstCollision = true;      // Collide physics instance with others?
    m_Settings.m_bSelfCollision = true;          // Collide bodies within physics instance?
    m_Settings.m_PenetrationFix = 1.0f;          // Penetration fix scalar
    m_Settings.m_MaxPenetrationFix = 2.0f;       // Max amount of penetration fix

    m_Settings.m_bSolveContacts = true;  // Solve contacts?
    m_Settings.m_nContactIterations = 5; // # of iterations to solve contact

    m_Settings.m_bShock = true; // Do shock propagation?

    m_Settings.m_nMaxActiveConstraints = 10 * 56; // Maximum # of active constraints (56 per ragdoll)
    m_Settings.m_ConstraintFix = 60.0f;           // Constraint fix scalar
    m_Settings.m_MaxConstraintFix = 6000.0f;      // Max amount of constraint

    m_Settings.m_ActiveLinearSpeed = 30.0f;  // Translation speed at which body is considered active
    m_Settings.m_ActiveAngularSpeed = 4.0f;  // Rotation speed at which body is considered active
    m_Settings.m_bDeactivate = true;         // Allow rigid bodies to deactivate
    m_Settings.m_DeactivateStartTime = 0.5f; // Time to start blending to deactivate
    m_Settings.m_DeactivateEndTime = 1.0f;   // Time before being fully deactivate

    m_Settings.m_MaxLinearVel = 1500.0f;     // Max linear velocity
    m_Settings.m_MaxAngularVel = 25.0f;      // Max angular velocity
    m_Settings.m_MaxForce = 18686.0f * 3.0f; // Max allowed force

    m_Settings.m_nRenderIterations = 10; // # of render jitter fix iterations

    m_Settings.m_bShowStats = false;   // Show stats info?
    m_Settings.m_bDebugRender = false; // Render debug info?
    m_Settings.m_bUsePolycache = true; // Use world collision or ground plane?

    // Collision
    m_NextCollisionGroup = -1;
    m_DeltaTime = 0.0f;
}

//==============================================================================

physics_mgr::~physics_mgr()
{
}

//==============================================================================

void physics_mgr::Init()
{
    // Allocate arrays
    m_Collisions.reserve(m_Settings.m_nMaxCollisions);
    m_ActiveConstraints.reserve(m_Settings.m_nMaxActiveConstraints);

    // Setup world collision and rigid body
    g_WorldColl.SetType(collision_shape::TYPE_WORLD);
    g_WorldBody.SetCollisionShape(&g_WorldColl, 0.0f, 100.0f);
    g_WorldBody.SetElasticity(1.0f);
    g_WorldBody.SetDynamicFriction(1.0f);
    g_WorldBody.SetStaticFriction(1.0f);

    // Setup actor collision and rigid body
    g_ActorColl.SetType(collision_shape::TYPE_CAPSULE);
    g_ActorColl.SetSphereCapacity(2);
    g_ActorColl.AddSphere(Vector3(0.0f, 0.0f, 0.0f));
    g_ActorColl.AddSphere(Vector3(0.0f, 0.0f, 0.0f));
    g_ActorBody.SetCollisionShape(&g_ActorColl, 0.0f, 100.0f);
    g_ActorBody.SetElasticity(1.0f);
    g_ActorBody.SetDynamicFriction(1.0f);
    g_ActorBody.SetStaticFriction(1.0f);
}

//==============================================================================

void physics_mgr::Kill()
{
    // Release collision memory
    m_Collisions.clear();
    m_ActiveConstraints.clear();

    // Make sure all lists are empty
    assert(m_AwakeInstances.size() == 0);
    assert(m_SleepingInstances.size() == 0);
    assert(m_ActiveBodies.size() == 0);
    assert(m_ActiveConstraints.size() == 0);
    assert(m_SolveConstraints.size() == 0);
}

//==============================================================================
// Physics instance functions
//==============================================================================

void physics_mgr::AddInstance(physics_inst* pInstance)
{
    // Instance should be initialized, but not in ANY lists
    assert(pInstance->m_bInitialized == false);
    assert(pInstance->m_bInAwakeList == false);
    assert(pInstance->m_bInSleepingList == false);
    assert(pInstance->m_bInCollisionWakeupList == false);

    // Add to sleeping list
    m_SleepingInstances.push_back(pInstance);
    pInstance->m_bInSleepingList = true;

    // Flag as ready
    pInstance->m_bInitialized = true;
}

//==============================================================================

void physics_mgr::RemoveInstance(physics_inst* pInstance)
{
    // Not in physics mgr?
    if (pInstance->m_bInitialized == false) {
        assert(pInstance->m_bInAwakeList == false);
        assert(pInstance->m_bInSleepingList == false);
        assert(pInstance->m_bInCollisionWakeupList == false);
        return;
    }

    // Instance should be initialized, but never in BOTH lists
    assert(pInstance->m_bInitialized == true);
    assert(!((pInstance->m_bInAwakeList == true) && (pInstance->m_bInSleepingList == true)));

    // Remove from awake list?
    if (pInstance->m_bInAwakeList) {
        m_AwakeInstances.remove(pInstance);
        pInstance->m_bInAwakeList = false;
    }

    // Remove from sleeping list?
    if (pInstance->m_bInSleepingList) {
        m_SleepingInstances.remove(pInstance);
        pInstance->m_bInSleepingList = false;
    }

    // Should have been removed from both lists!
    assert(!pInstance->m_bInAwakeList);
    assert(!pInstance->m_bInSleepingList);

    // Activate any overlapping sleeping instances so
    // that they aren't left floating in the air
    std::vector<physics_inst*> activateList;
    for (physics_inst* pSleepingInst : m_SleepingInstances) {
        // Validate list management
        assert(pSleepingInst->m_bInitialized);
        assert(!pSleepingInst->m_bInAwakeList);
        assert(pSleepingInst->m_bInSleepingList);
        assert(!pSleepingInst->m_bInCollisionWakeupList);

        // Quick zone check - in same zone?
        if (pInstance->GetZone() == pSleepingInst->GetZone()) {
            // Overlap?
            if (pInstance->GetWorldBBox().Intersect(pSleepingInst->GetWorldBBox())) {
                // Activate. This modifies sleepingInstances list to defer the activation
                activateList.push_back(pSleepingInst);
            }
        }
    }
    for (physics_inst* toActivate : activateList) {
        toActivate->Activate();
    }
    // Flag as not ready any more
    pInstance->m_bInitialized = false;
}

//==============================================================================

void physics_mgr::WakeupInstance(physics_inst* pInstance)
{
    // Should be initialized
    assert(pInstance->m_bInitialized);

    // Already awake?
    if (pInstance->m_bInAwakeList) {
        // Should not be in sleeping or collision wakeup list also!
        assert(pInstance->m_bInSleepingList == false);
        assert(pInstance->m_bInCollisionWakeupList == false);
        return;
    }

    // Remove from sleeping list?
    if (pInstance->m_bInSleepingList) {
        m_SleepingInstances.remove(pInstance);
        pInstance->m_bInSleepingList = false;
    }

    // Remove from collision wakeup list?
    if (pInstance->m_bInCollisionWakeupList) {
        m_CollisionWakeupInstances.remove(pInstance);
        pInstance->m_bInCollisionWakeupList = false;
    }

    // Add to front of awake list in case we came here from collision detection
    // so that this instances isn't double checked for collision
    assert(pInstance->m_bInAwakeList == false);
    m_AwakeInstances.push_front(pInstance);
    pInstance->m_bInAwakeList = true;
}

//==============================================================================

void physics_mgr::PutToSleepInstance(physics_inst* pInstance)
{
    // Should be initialized
    assert(pInstance->m_bInitialized);
    assert(pInstance->m_bInCollisionWakeupList == false);

    // Already asleep?
    if (pInstance->m_bInSleepingList) {
        // Should not be in awake or collision wakeup list also!
        assert(pInstance->m_bInAwakeList == false);
        assert(pInstance->m_bInCollisionWakeupList == false);
        return;
    }

    // Remove from awake list?
    if (pInstance->m_bInAwakeList) {
        m_AwakeInstances.remove(pInstance);
        pInstance->m_bInAwakeList = false;
    }

    // Update bbox of instance (and internal bodies) before it is made inactive
    // (since collision detection only updates bboxes of active instances)
    pInstance->ComputeWorldBBox();

    // Add to sleeping list
    assert(pInstance->m_bInSleepingList == false);
    m_SleepingInstances.push_front(pInstance);
    pInstance->m_bInSleepingList = true;
}

//==============================================================================

void physics_mgr::CollisionWakeupInstance(physics_inst* pInstance)
{
    // Should be initialized
    assert(pInstance->m_bInitialized);
    assert(!pInstance->m_bInAwakeList);

    // Already in collision wakeup list?
    if (pInstance->m_bInCollisionWakeupList) {
        return;
    }

    // Should be in sleeping list!
    assert(!pInstance->m_bInAwakeList);
    assert(pInstance->m_bInSleepingList);

    // Remove from sleeping list?
    if (pInstance->m_bInSleepingList) {
        m_SleepingInstances.remove(pInstance);
        pInstance->m_bInSleepingList = false;
    }

    // Add to collision wakeup list
    assert(!pInstance->m_bInCollisionWakeupList);
    m_CollisionWakeupInstances.push_back(pInstance);
    pInstance->m_bInCollisionWakeupList = true;
}

//==============================================================================
// Collision functions
//==============================================================================

int physics_mgr::AddCollision(rigid_body*    pBody0,
                              rigid_body*    pBody1,
                              const Vector3& Pos,
                              const Vector3& Normal,
                              const float&   Dist)
{
    assert(pBody0);
    assert(pBody1);

    // Reached max collision count?
    if (m_Collisions.size() == m_Settings.m_nMaxCollisions) {
        //LOG_WARNING("physics_mgr::AddCollision", "Ran out collisions! Need to increase m_Settings.m_nMaxCollisions\n");
        assert(false); //, "physics_mgr::AddCollision - ran out of collisions!");
        return -1;
    }

    // Create new collision
    int        Index = m_Collisions.size();
    collision& Collision = m_Collisions.emplace_back();

    // Setup collision
    Collision.m_pBody0 = pBody0;
    Collision.m_pBody1 = pBody1;
    Collision.m_pActor1 = g_pCollisionActor;

    Collision.m_Position = Pos;
    Collision.m_Normal = Normal;
    Collision.m_Depth = Dist;

    Collision.m_R0 = Pos - pBody0->GetPosition();
    Collision.m_R1 = Pos - pBody1->GetPosition();

    Collision.m_Elasticity = pBody0->GetElasticity() * pBody1->GetElasticity();
    Collision.m_StaticFriction = 0.5f * (pBody0->GetStaticFriction() + pBody1->GetStaticFriction());
    Collision.m_DynamicFriction = 0.5f * (pBody0->GetDynamicFriction() + pBody1->GetDynamicFriction());

    Collision.m_PenetrationExtra.Zero();
    Collision.m_Denominator = 0.0f;

    return Index;
}

//==============================================================================

void physics_mgr::DetectWorldCollisions(physics_inst* pInst)
{
    // Validate list management
    assert(pInst);
    assert(pInst->m_bInitialized);
    assert(pInst->m_bInAwakeList);
    assert(pInst->m_bInSleepingList);
    assert(pInst->m_bInCollisionWakeupList);

    // Collect world polys around instance
    g_Collider.CollectWorldPolys(pInst->GetWorldBBox());

    // Check all rigid bodies with world
    for (int i = 0; i < pInst->GetNRigidBodies(); i++) {
        // Lookup body info
        rigid_body* pBody = &pInst->GetRigidBody(i);
        assert(pBody);

        // Skip if body does not collide with world
        if ((pBody->m_Flags & rigid_body::FLAG_WORLD_COLLISION) == 0) {
            continue;
        }

        // Skip if not active
        if (!pBody->IsActive()) {
            continue;
        }

        // Lookup body collision shape
        collision_shape* pColl = pBody->GetCollisionShape();
        if (!pColl) {
            continue;
        }

        // Check collision with world
        g_Collider.Check(pColl, &g_WorldColl);
    }
}

//==============================================================================

void physics_mgr::DetectSelfCollisions(physics_inst* pInst)
{
    // Validate list management
    assert(pInst);
    assert(pInst->m_bInitialized);
    assert(pInst->m_bInAwakeList);
    assert(pInst->m_bInSleepingList == false);
    assert(pInst->m_bInCollisionWakeupList == false);

    // Check all rigid bodies with other bodies in this instance
    for (int i = 0; i < pInst->GetNRigidBodies(); i++) {
        // Lookup body0 info
        rigid_body* pBody0 = &pInst->GetRigidBody(i);
        assert(pBody0);

        // Clear collision info ready for tracking the biggest collision
        pBody0->ClearCollision();

        // Lookup body0 collision shape
        collision_shape* pColl0 = pBody0->GetCollisionShape();
        if (!pColl0) {
            continue;
        }

        // Lookup body0 bbox
        const BBox& BBox0 = pBody0->GetWorldBBox();

        // Check with all other bodies in instance
        for (int j = i + 1; j < pInst->GetNRigidBodies(); j++) {
            // Lookup body1
            rigid_body* pBody1 = &pInst->GetRigidBody(j);
            assert(pBody1);

            // Lookup body1 collision shape
            collision_shape* pColl1 = pBody1->GetCollisionShape();
            if (!pColl1) {
                continue;
            }

            // Collision enabled between bodies?
            if (pBody0->IsCollisionEnabled(pBody1)) {
                // At least one of the bodies is active?
                if (pBody0->IsActive() || pBody1->IsActive()) {
                    // Do world bounding boxes overlap?
                    const BBox& BBox1 = pBody1->GetWorldBBox();
                    if (BBox0.Intersect(BBox1)) {
                        // Detect collision between bodies

                        // Collision occurred?
                        if (g_Collider.Check(pColl0, pColl1)) {
                            // Wakeup bodies
                            pBody0->CollisionWakeup();
                            pBody1->CollisionWakeup();
                        }
                    }
                }
            }
        }
    }
}

//==============================================================================

// Yuck - this is Order(NxM) where N = # of active instances, M = total # of instances
// Because of the fast zone check and the fact that there usually aren't many
// instances in the same zone, this is fast enough for our needs.
void physics_mgr::DetectInstInstCollisions(physics_inst* pInst0)
{
    // Validate list management
    assert(pInst0);
    assert(pInst0->m_bInitialized == true);
    assert(pInst0->m_bInAwakeList == true);
    assert(pInst0->m_bInSleepingList == false);
    assert(pInst0->m_bInCollisionWakeupList == false);
    assert(pInst0->GetInstCollision());

    // Lookup bbox
    const BBox& BBox0 = pInst0->GetWorldBBox();

    // Check against the rest of the awake list
    for (physics_inst* pInst1 : m_AwakeInstances) {
        // Validate list management
        assert(pInst0 != pInst1);
        assert(pInst1->m_bInitialized);
        assert(pInst1->m_bInAwakeList);
        assert(!pInst1->m_bInSleepingList);
        assert(!pInst1->m_bInCollisionWakeupList);

        // Overlap?
        if ((pInst1->GetInstCollision())                  // Inst collision enabled?
            && (pInst0->GetZone() == pInst1->GetZone())   // Quick zone check
            && (BBox0.Intersect(pInst1->GetWorldBBox()))) // Slow bbox check
        {
            // Check all rigid bodies with other bodies in this instance
            for (int i = 0; i < pInst0->GetNRigidBodies(); i++) {
                // Lookup body0 and skip if inactive
                rigid_body* pBody0 = &pInst0->GetRigidBody(i);
                assert(pBody0);
                if (!pBody0->IsActive()) {
                    continue;
                }

                // Lookup collision info
                const BBox&      BBox0 = pBody0->GetWorldBBox();
                collision_shape* pColl0 = pBody0->GetCollisionShape();
                if (!pColl0) {
                    continue;
                }

                // Check with bodies in other instance
                for (int j = 0; j < pInst1->GetNRigidBodies(); j++) {
                    // Lookup body1
                    rigid_body* pBody1 = &pInst1->GetRigidBody(j);
                    assert(pBody1);

                    // Lookup body1 info
                    collision_shape* pColl1 = pBody1->GetCollisionShape();
                    if (!pColl1) {
                        continue;
                    }

                    // Do world bounding boxes overlap?
                    const BBox& BBox1 = pBody1->GetWorldBBox();
                    if (BBox0.Intersect(BBox1)) {
                        // Detect collision between bodies

                        // Collision occurred?
                        if (g_Collider.Check(pColl0, pColl1)) {
                            // Wakeup body1
                            pBody1->CollisionWakeup();
                        }
                    }
                }
            }
        }
    }

    std::vector<physics_inst*> instancesToCollisionWakeup;
    for (physics_inst* pInst1 : m_SleepingInstances) {
        // Validate list management
        assert(pInst0 != pInst1);
        assert(pInst1->m_bInitialized == true);
        assert(pInst1->m_bInAwakeList == false);
        assert(pInst1->m_bInSleepingList == true);
        assert(pInst1->m_bInCollisionWakeupList == false);

        // Overlap?
        if ((pInst1->GetInstCollision())                  // Inst collision enabled?
            && (pInst0->GetZone() == pInst1->GetZone())   // Quick zone check
            && (BBox0.Intersect(pInst1->GetWorldBBox()))) // Slow bbox check
        {
            // Check all rigid bodies with other bodies in this instance
            for (int i = 0; i < pInst0->GetNRigidBodies(); i++) {
                // Lookup body0 and skip if inactive
                rigid_body* pBody0 = &pInst0->GetRigidBody(i);
                assert(pBody0);
                if (!pBody0->IsActive()) {
                    continue;
                }

                // Lookup collision info
                const BBox&      BBox0 = pBody0->GetWorldBBox();
                collision_shape* pColl0 = pBody0->GetCollisionShape();
                if (!pColl0) {
                    continue;
                }

                // Check with bodies in other instance
                for (int j = 0; j < pInst1->GetNRigidBodies(); j++) {
                    // Lookup body1
                    rigid_body* pBody1 = &pInst1->GetRigidBody(j);
                    assert(pBody1);

                    // Lookup body1 info
                    collision_shape* pColl1 = pBody1->GetCollisionShape();
                    if (!pColl1) {
                        continue;
                    }

                    // Do world bounding boxes overlap?
                    const BBox& BBox1 = pBody1->GetWorldBBox();
                    if (BBox0.Intersect(BBox1)) {
                        // Detect collision between bodies

                        // Collision occurred?
                        if (g_Collider.Check(pColl0, pColl1)) {
                            // CollisionWakeupInstance changes the sleeping list, so defer.
                            instancesToCollisionWakeup.push_back(pInst1);
                            // Wakeup body1
                            pBody1->CollisionWakeup();
                        }
                    }
                }
            }
        }
    }
    for (physics_inst* p : instancesToCollisionWakeup) {
        CollisionWakeupInstance(p);
    }
}

//==============================================================================

void physics_mgr::DetectInstActorCollisions(physics_inst* pInst)
{
    // Validate list management
    assert(pInst->m_bInitialized == true);
    assert(pInst->m_bInAwakeList == true);
    assert(pInst->m_bInSleepingList == false);
    assert(pInst->m_bInCollisionWakeupList == false);

    // Lookup bbox of instance
    const BBox& bbox = pInst->GetWorldBBox();

    // Loop through all actors
    actor* pActor = actor::m_pFirstActive;
    while (pActor) {
        // Overlap?
        if ((pInst->GetZone() == pActor->GetZone1()) // Quick zone check
            && (bbox.Intersect(pActor->GetBBox()))) {
            // Setup global collision actor
            g_pCollisionActor = pActor;

            // Loop through all rigid bodies in this instance and check for colliding with actor
            for (int i = 0; i < pInst->GetNRigidBodies(); i++) {
                // Lookup body
                rigid_body* pBody = &pInst->GetRigidBody(i);
                assert(pBody);

                // Lookup collision shape and skip if none
                collision_shape* pColl = pBody->GetCollisionShape();
                if (!pColl) {
                    continue;
                }

                // Lookup actor collision radius (scale for 1st person player)
                float CapsuleRadius = pActor->GetCollisionRadius();
                if (pActor->GetType() == Object::TYPE_PLAYER) {
                    CapsuleRadius *= 2.0f;
                }

                // Compute capsule
                Vector3 CapsuleBase = pActor->GetPosition();
                Vector3 CapsuleStart = CapsuleBase;
                Vector3 CapsuleEnd = CapsuleBase + Vector3(0.0f, pActor->GetCollisionHeight(), 0.0f);

                // Setup capsule collision shape (just setup start and end spheres)
                collision_shape::sphere& Start = g_ActorColl.GetSphere(0);
                collision_shape::sphere& End = g_ActorColl.GetSphere(1);
                g_ActorColl.SetRadius(CapsuleRadius);
                Start.m_PrevPos = CapsuleStart;
                Start.m_CurrPos = CapsuleStart;
                Start.m_CollFreePos = CapsuleStart;
                End.m_PrevPos = CapsuleEnd;
                End.m_CurrPos = CapsuleEnd;
                End.m_CollFreePos = CapsuleEnd;

                // Check for collision
                if (g_Collider.Check(pColl, &g_ActorColl)) {
                    // Wakeup body
                    pBody->CollisionWakeup();
                }
            }
        }

        // Check against next actor
        pActor = pActor->m_pNextActive;
    }

    // Clear global collision actor
    g_pCollisionActor = NULL;
}

//==============================================================================

void physics_mgr::DetectCollisions(void)
{
    assert(m_CollisionWakeupInstances.size() == 0);

    // Clear collisions
    m_Collisions.clear();

    // Update bboxes of active instances (and internal bodies)
    for (physics_inst* pInst : m_AwakeInstances) {
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList);
        assert(!pInst->m_bInSleepingList);
        assert(!pInst->m_bInCollisionWakeupList);

        // Update bbox
        pInst->ComputeWorldBBox();
    }

    // Loop through all active instances
    for (physics_inst* pInst : m_AwakeInstances) {
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList);
        assert(pInst->m_bInSleepingList == false);
        assert(pInst->m_bInCollisionWakeupList == false);

        // Detect world and self collisions?
        if (pInst->GetWorldCollision()) {
            DetectWorldCollisions(pInst);
        }

        // Detect self collisions?
        if (m_Settings.m_bSelfCollision) {
            DetectSelfCollisions(pInst);
        }

        // Check collision with ALL other instances in simulation!
        if ((pInst->GetInstCollision()) && (m_Settings.m_bInstInstCollision)) {
            DetectInstInstCollisions(pInst);
        }

        // Collide with actors?
        // (Only active instances are tested for since the punch bag dummy is the only
        // instance that has actor collision and it's always active when visible)
        if (pInst->GetActorCollision()) {
            DetectInstActorCollisions(pInst);
        }
    }

    // Collision may wake up some sleeping instances
    std::vector<physics_inst*> wakeupList;
    for (physics_inst* pInst : m_CollisionWakeupInstances) {
        // needed because WakeupInstance modifies the list
        wakeupList.push_back(pInst);
    }

    for (physics_inst* pInst : wakeupList) {
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(!pInst->m_bInAwakeList);
        assert(!pInst->m_bInSleepingList);
        assert(pInst->m_bInCollisionWakeupList);
        WakeupInstance(pInst);

        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList);
        assert(!pInst->m_bInSleepingList);
        assert(!pInst->m_bInCollisionWakeupList);
    }

    assert(m_CollisionWakeupInstances.empty());
}

//==============================================================================

void physics_mgr::PreApplyCollisions(float DeltaTime)
{
    // Loop through all collisions
    for (int i = 0; i < m_Collisions.size(); i++) {
        // Lookup collision info
        physics_mgr::collision& Collision = m_Collisions[i];
        rigid_body*             pBody0 = Collision.m_pBody0;
        rigid_body*             pBody1 = Collision.m_pBody1;
        Vector3&                R0 = Collision.m_R0;
        Vector3&                R1 = Collision.m_R1;
        Vector3&                Normal = Collision.m_Normal;

        // Compute penetration penalty

        // Actually - this breaks the shock prop!!!!
        if (Collision.m_Depth > 0) {
            float Extra = Collision.m_Depth * m_Settings.m_PenetrationFix / (DeltaTime * 60.0f);
            if (Extra > m_Settings.m_MaxPenetrationFix) {
                Extra = m_Settings.m_MaxPenetrationFix;
            }

            Collision.m_PenetrationExtra = -Extra * Normal;
        } else {
            Collision.m_PenetrationExtra.Zero();
        }

        // Compute denominator
        float Denominator = 0.0f;

        if ((pBody0) && (pBody0->GetMass() != 0.0f)) {
            Denominator += pBody0->GetInvMass() +
                           v3_Dot(Normal, v3_Cross(pBody0->GetWorldInvInertia().RotateVector(v3_Cross(R0, Normal)), R0));
        }

        if ((pBody1) && (pBody1->GetMass() != 0.0f)) {
            Denominator += pBody1->GetInvMass() +
                           v3_Dot(Normal, v3_Cross(pBody1->GetWorldInvInertia().RotateVector(v3_Cross(R1, Normal)), R1));
        }

        Collision.m_Denominator = Denominator;
    }
}

//==============================================================================

int physics_mgr::SolveCollision(collision& Collision)
{
    // No penetration?
    if (Collision.m_Depth < 0) {
        return false;
    }

    // Too small denominator?
    if (Collision.m_Denominator < 0.0001f) {
        return false;
    }

    // Lookup rigid bodies and leave alone if infinite mass
    rigid_body* pBody0 = Collision.m_pBody0;
    rigid_body* pBody1 = Collision.m_pBody1;
    if ((pBody0) && (pBody0->GetMass() == 0.0f)) {
        pBody0 = NULL;
    }
    if ((pBody1) && (pBody1 == &g_WorldBody)) {
        pBody1 = NULL;
    }

    // Nothing to do?
    if ((pBody0 == NULL) && (pBody1 == NULL)) {
        return false;
    }

    // Setup body1 velocity from actor?
    if (pBody1 == &g_ActorBody) {
        // Grab movement velocity so correct impact happens when the player runs
        // into a rigid body (eg. for the punch bag dummy!)
        assert(Collision.m_pActor1);
        g_ActorBody.SetLinearVelocity(Collision.m_pActor1->GetVelocity());
    }

    // Compute relative velocity of bodies, taking penetration penatly into account
    Vector3 VRel = Collision.m_PenetrationExtra;
    if (pBody0) {
        VRel += pBody0->GetLinearVelocity() + v3_Cross(pBody0->GetAngularVelocity(), Collision.m_R0);
    }
    if (pBody1) {
        VRel -= pBody1->GetLinearVelocity() + v3_Cross(pBody1->GetAngularVelocity(), Collision.m_R1);
    }

    // Compute velocity along penetration normal
    float NormalVel = v3_Dot(VRel, Collision.m_Normal);

    // Moving away from each other?
    if (NormalVel >= 0.0f) {
        return false;
    }

    // Record collision on body0
    if (pBody0) {
        // Flag collision has happened
        pBody0->m_Flags |= rigid_body::FLAG_HAS_COLLIDED;

        // Biggest impact so far?
        float CollisionSpeedSqr = VRel.LengthSquared();
        if (CollisionSpeedSqr > pBody0->m_CollisionSpeedSqr) {
            pBody0->m_CollisionSpeedSqr = CollisionSpeedSqr;
        }
    }

    // Bodies are moving into each other, compute impulse
    float Numerator = -(1.0f + Collision.m_Elasticity) * NormalVel;
    float NormalImpulse = Numerator / Collision.m_Denominator;

    Vector3 CollisionImpulse = NormalImpulse * Collision.m_Normal;

    // Apply impulse to bodies
    if (pBody0) {
        pBody0->ApplyLocalImpulse(CollisionImpulse, Collision.m_R0);
    }

    if (pBody1) {
        pBody1->ApplyLocalImpulse(-CollisionImpulse, Collision.m_R1);
    }

    // Apply friction

    // Re-compute relative velocity of bodies ready for friction
    VRel.Zero();
    if (pBody0) {
        VRel += pBody0->GetLinearVelocity() + v3_Cross(pBody0->GetAngularVelocity(), Collision.m_R0);
    }
    if (pBody1) {
        VRel -= pBody1->GetLinearVelocity() + v3_Cross(pBody1->GetAngularVelocity(), Collision.m_R1);
    }

    // Compute tangent vel and speed
    Vector3 TangentVel = VRel - v3_Dot(VRel, Collision.m_Normal) * Collision.m_Normal;
    float   TangentSpeedSqr = TangentVel.LengthSquared();

    // Apply friction?
    if (TangentSpeedSqr > 0.001f) {
        // Compute tagent direction
        float   TangentSpeed = sqrt(TangentSpeedSqr);
        Vector3 T = -TangentVel / TangentSpeed;

        Numerator = TangentSpeed;
        float Denominator = 0;
        if (pBody0) {
            Denominator += pBody0->GetInvMass() +
                           v3_Dot(T, v3_Cross(pBody0->GetWorldInvInertia().RotateVector(v3_Cross(Collision.m_R0, T)), Collision.m_R0));
        }

        if (pBody1) {
            Denominator += pBody1->GetInvMass() +
                           v3_Dot(T, v3_Cross(pBody1->GetWorldInvInertia().RotateVector(v3_Cross(Collision.m_R1, T)), Collision.m_R1));
        }

        if (Denominator > 0.0001f) {
            float ImpulseToReverse = Numerator / Denominator;
            float ImpulseFromNormalImpulse = Collision.m_StaticFriction * NormalImpulse;
            float FrictionImpulse;

            if (ImpulseToReverse < ImpulseFromNormalImpulse) {
                FrictionImpulse = ImpulseToReverse;
            } else {
                FrictionImpulse = Collision.m_DynamicFriction * NormalImpulse;
            }

            T *= FrictionImpulse;

            if (pBody0) {
                pBody0->ApplyLocalImpulse(T, Collision.m_R0);
            }

            if (pBody1) {
                pBody1->ApplyLocalImpulse(-T, Collision.m_R1);
            }
        }
    }

    // Collision occurred!
    return true;
}

//==============================================================================

void physics_mgr::PreApplyConstraints(float DeltaTime)
{
    // Prepare constraints ready for solving collisions

    m_SolveConstraints.clear();
    int nActiveConstraints = m_ActiveConstraints.size();
    for (int i = 0; i < nActiveConstraints; i++) {
        // Lookup constraint
        active_constraint& Constraint = m_ActiveConstraints[i];

        // Pre apply and add to solve list if not satisfied

        assert(Constraint.m_pOwner);
        if (Constraint.m_pOwner->PreApply(DeltaTime, Constraint)) {
            m_SolveConstraints.push_back(&Constraint);
        }
    }
}

//==============================================================================

void physics_mgr::SolveCollisions(float DeltaTime, int nIterations, ObjectManager* om)
{
    int i;

    // Prepare constraints ready for solving
    PreApplyConstraints(DeltaTime);

    // Get collision and constraint info
    int nCollisions = m_Collisions.size();
    int nConstraints = m_SolveConstraints.size();
    if ((!nCollisions) && (!nConstraints)) {
        return;
    }

    // Prepare collisions
    PreApplyCollisions(DeltaTime);

    // Alternate solve direction to reduce jitter when resting
    int Direction = om->GetNLogicLoops() & 1;

    // Process as normal
    int bActive = true;
    int Loops = 0;
    while ((nIterations--) && (bActive)) {
        bActive = false;
        Loops++;

        // Solve in which direction?
        if (Direction) {
            // Solve all constraints
            for (active_constraint* pConstraint : m_SolveConstraints){
                assert(pConstraint->m_pOwner);
                bActive |= pConstraint->m_pOwner->Apply(*pConstraint);
            }

            for (i = 0; i < nCollisions; i++) {
                bActive |= SolveCollision(m_Collisions[i]);
            }
        } else {
            // Solve all constraints

            for (active_constraint* pConstraint : m_SolveConstraints){
                assert(pConstraint->m_pOwner);
                bActive |= pConstraint->m_pOwner->Apply(*pConstraint);
            }

            for (i = nCollisions - 1; i >= 0; i--) {
                bActive |= SolveCollision(m_Collisions[i]);
            }
        }

        // Toggle direction
        Direction ^= 1;
    }
}

//==============================================================================

void physics_mgr::SolveContacts(float DeltaTime, int nIterations, ObjectManager* om)
{
    int i;

    // Prepare constraints ready for solving
    PreApplyConstraints(DeltaTime);

    // Get collision and constraint info
    int nCollisions = m_Collisions.size();
    int nConstraints = m_SolveConstraints.size();
    if ((!nCollisions) && (!nConstraints)) {
        return;
    }

    // Set all collisions to inelastic
    for (i = 0; i < nCollisions; i++) {
        m_Collisions[i].m_Elasticity = 0.0f;
    }

    // Prepare collisions
    PreApplyCollisions(DeltaTime);

    // Alternate solve direction to reduce jitter when resting
    int Direction = om->GetNLogicLoops() & 1;

    // Process as inelastic collisions
    int bActive = true;
    int Loops = 0;
    while ((nIterations--) && (bActive)) {
        bActive = false;
        Loops++;

        // Solve in which direction?
        if (Direction) {
            // Solve all constraints

            for (active_constraint* pConstraint : m_SolveConstraints){
                assert(pConstraint->m_pOwner);
                bActive |= pConstraint->m_pOwner->Apply(*pConstraint);
            }

            for (i = 0; i < nCollisions; i++) {
                bActive |= SolveCollision(m_Collisions[i]);
            }

        } else {
            // Solve all constraints

            for (active_constraint* pConstraint : m_SolveConstraints){
                assert(pConstraint->m_pOwner);
                bActive |= pConstraint->m_pOwner->Apply(*pConstraint);
            }

            for (i = nCollisions - 1; i >= 0; i--) {
                bActive |= SolveCollision(m_Collisions[i]);
            }
        }

        // Toggle direction
        Direction ^= 1;
    }
}

//==============================================================================

void physics_mgr::ShockPropagation()
{

    // Sort collision bodies from lowest to highest
    if (m_Collisions.size()) {
        qsort(&m_Collisions[0],         // Address of first item in xarray.
              m_Collisions.size(),      // Number of items in xarray.
              sizeof(collision),        // Size of one item.
              collision_sort_lo_to_hi); // Compare function.
    }

    // Loop through collisions and make bottom body immovable, then solve
    for (int i = 0; i < m_Collisions.size(); i++) {
        // Get collision
        collision& Collision = m_Collisions[i];

        // Lookup rigid bodies
        rigid_body* pBody0 = Collision.m_pBody0;
        rigid_body* pBody1 = Collision.m_pBody1;
        assert(pBody0);
        assert(pBody1);

        // Set lowest body to be immovable
        if (pBody0->GetPosition().GetY() < pBody1->GetPosition().GetY()) {
            // Clear lower body
            Collision.m_pBody0 = NULL;

            // Re-compute denominator
            Collision.m_Denominator = pBody1->GetInvMass() +
                                      v3_Dot(Collision.m_Normal, v3_Cross(pBody1->GetWorldInvInertia().RotateVector(v3_Cross(Collision.m_R1, Collision.m_Normal)), Collision.m_R1));
        } else {
            // Clear lower body
            Collision.m_pBody1 = NULL;

            // Re-compute denominator
            Collision.m_Denominator = pBody0->GetInvMass() +
                                      v3_Dot(Collision.m_Normal, v3_Cross(pBody0->GetWorldInvInertia().RotateVector(v3_Cross(Collision.m_R0, Collision.m_Normal)), Collision.m_R0));
        }

        // Solve the collision
        SolveCollision(Collision);
    }
}

//==============================================================================

void physics_mgr::PutInstancesToSleep(float DeltaTime)
{

    // Loop through awake instances checking to see if we can put them to sleep
    physics_inst* pInst = m_AwakeInstances.GetHead();
    while (pInst) {
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList == true);
        assert(pInst->m_bInSleepingList == false);
        assert(pInst->m_bInCollisionWakeupList == false);

        // Get next instance in-case of deletion from this list
        physics_inst* pNextInst = m_AwakeInstances.GetNext(pInst);

        // Default to not active
        bool bInstActive = false;

        // Update body active state
        for (int i = 0; i < pInst->GetNRigidBodies(); i++) {
            // Lookup body
            rigid_body& Body = pInst->GetRigidBody(i);

            // Update active state
            Body.UpdateActiveState(DeltaTime);

            // If body is active, then instance is active
            if (Body.IsActive()) {
                // Keep instance in active list
                bInstActive = true;
            } else {
                // Make sure body vels/forces are cleared for constraints to work properly
                Body.ZeroVelocity();
                Body.ClearForces();
            }
        }

        // Put to sleep?
        if (bInstActive == false) {
            // Put to sleep
            PutToSleepInstance(pInst);
        }

        // Check next awake instance
        pInst = pNextInst;
    }
}

//==============================================================================

void physics_mgr::BuildActiveBodyList(void)
{

    // Clear the list
    m_ActiveBodies.clear();

    // Loop through all awake instances
    physics_inst* pInst = m_AwakeInstances.GetHead();
    while (pInst) {
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList == true);
        assert(pInst->m_bInSleepingList == false);
        assert(pInst->m_bInCollisionWakeupList == false);

        // Build list of active bodies
        for (int i = 0; i < pInst->GetNRigidBodies(); i++) {
            // Lookup body
            rigid_body& Body = pInst->GetRigidBody(i);

            // Body awake?
            if (Body.IsActive()) {
                // Add body to active list
                m_ActiveBodies.push_back(&Body);
            }
        }

        // Get next awake instance
        pInst = m_AwakeInstances.GetNext(pInst);
    }
}

//==============================================================================

void physics_mgr::BuildActiveBodyAndConstraintList()
{
    int i;

    // Clear all lists
    m_ActiveBodies.clear();
    m_ActiveConstraints.clear();
    m_SolveConstraints.clear();

    // Loop through all awake instances
    for (physics_inst* pInst : m_AwakeInstances){
        // Validate list management
        assert(pInst->m_bInitialized);
        assert(pInst->m_bInAwakeList == true);
        assert(pInst->m_bInSleepingList == false);
        assert(pInst->m_bInCollisionWakeupList == false);

        // Are there enough active constraints to make this instance active?
        int  NConstraintsNeeded = pInst->GetNBodyBodyConstraints() + pInst->GetNBodyWorldConstraints();
        bool bConstraintsAvailable = ((m_ActiveConstraints.size() + NConstraintsNeeded) <= m_Settings.m_nMaxActiveConstraints);
        assert(bConstraintsAvailable);
        if (bConstraintsAvailable) {
            // Force render matrices to be rebuilt
            pInst->DirtyMatrices();

            // Build list of active bodies
            for (i = 0; i < pInst->GetNRigidBodies(); i++) {
                // Lookup body
                rigid_body& Body = pInst->GetRigidBody(i);

                // Body awake?
                if (Body.IsActive()) {
                    // Add body to active list
                    m_ActiveBodies.push_back(&Body);
                }
            }

            // Lookup blend in constraint weight
            float ConstraintWeight = pInst->GetConstraintWeight();

            // Build list of active body -> body constraints
            for (i = 0; i < pInst->GetNBodyBodyConstraints(); i++) {
                // Add if active
                constraint& Constraint = pInst->GetBodyBodyConstraint(i);
                if (Constraint.IsActive()) {
                    // Allocate and add active constraint?
                    if (m_ActiveConstraints.size() < m_Settings.m_nMaxActiveConstraints) {
                        active_constraint& ActiveConstraint = m_ActiveConstraints.emplace_back();
                        ActiveConstraint.m_pOwner = &Constraint;

                        // Setup weight
                        if (Constraint.GetFlags() & constraint::FLAG_BLEND_IN) {
                            ActiveConstraint.m_Weight = ConstraintWeight;
                        } else {
                            ActiveConstraint.m_Weight = 1.0f;
                        }
                    } else {
                        //LOG_WARNING( "physics_mgr::Step", "Ran out active constraints! Need to increase m_Settings.m_nMaxActiveConstraints\n" );
                        assert(false); //0, "physics_mgr::Step - ran out of active constraints!");
                    }
                }
            }

            // Build list of active body -> world constraints
            for (i = 0; i < pInst->GetNBodyWorldConstraints(); i++) {
                // Add if active
                constraint& Constraint = pInst->GetBodyWorldConstraint(i);
                if (Constraint.IsActive()) {
                    // Allocate and add active constraint?
                    if (m_ActiveConstraints.size() < m_Settings.m_nMaxActiveConstraints) {
                        active_constraint& ActiveConstraint = m_ActiveConstraints.emplace_back();
                        ActiveConstraint.m_pOwner = &Constraint;
                        ActiveConstraint.m_Weight = 1.0f; // Body-World constraints always fully on
                    } else {
                        //LOG_WARNING( "physics_mgr::Step", "Ran out active constraints! Need to increase m_Settings.m_nMaxActiveConstraints\n" );
                        assert(false); //, "physics_mgr::Step - ran out of active constraints!");
                    }
                }
            }
        }
    }
}

//==============================================================================

void physics_mgr::Step(float DeltaTime, ObjectManager* om)
{
    /*
        // Physics simulation is as follows:
        0) Try put active instances to sleep
        1) Build active body list
        2) compute external forces
        3) store original position/vel/force
        4) predict new velocity
        5) predict new position
        6) detect collisions (may add more active bodies)
        7) Build new list of active bodies and active constraints
        8) restore velocity to original
        9) process collisions/constraints
        10) reset position to original
        11) integrate vels
        12) clear forces
        13) process contacts/constraints
        14) shock propagation
        15) integrate positions
        16) separate bodies
    */

    // Try put awake bodies to sleep
    PutInstancesToSleep(DeltaTime);

    // Build list of active rigid bodies
    BuildActiveBodyList();

    // Store body states and predict new velocities/positions using external forces
    for (rigid_body* pBody : m_ActiveBodies) {
        pBody->GetState(pBody->m_BackupState);
        pBody->ComputeForces(DeltaTime);
        pBody->IntegrateVelocity(DeltaTime);
        pBody->IntegratePosition(DeltaTime);
    }

    // Detect collisions with predicted velocity and position

    DetectCollisions();

    // Restore velocities to original zero forces

    for (rigid_body* pBody : m_ActiveBodies) {
        // Restore state
        pBody->SetVelocity(pBody->m_BackupState);
    }

    // Build list of active bodies/constraints ready for solving
    BuildActiveBodyAndConstraintList();

    // Solve collisions using old velocity

    SolveCollisions(DeltaTime, m_Settings.m_nCollisionIterations, om);

    // Restore original positions, integrate to new velocities, and clear forces

    for (rigid_body* pBody : m_ActiveBodies) {
        pBody->SetPosition(pBody->m_BackupState);
        pBody->IntegrateVelocity(DeltaTime);
        pBody->ClearForces();
    }

    // Solve contacts using new velocity
    if (m_Settings.m_bSolveContacts) {
        SolveContacts(DeltaTime, m_Settings.m_nContactIterations, om);
    }

    // Perform shock propagation for stacking etc.
    if (m_Settings.m_bShock) {
        ShockPropagation();
    }

    // Finally, integrate position

    for (rigid_body* pBody : m_ActiveBodies) {
        // Integrate
        pBody->IntegratePosition(DeltaTime);
    }

    // Clear lists in-case active instances are removed during game logic
    m_ActiveConstraints.clear();
    m_SolveConstraints.clear();
    m_ActiveBodies.clear();
}

//==============================================================================

void physics_mgr::Advance(float DeltaTime, ObjectManager* om)
{
    // Nothing to do?
    if (DeltaTime == 0.0f) {
        return;
    }

    // Loop and simulate

    int Iterations = m_Settings.m_nMaxTimeSteps;
    m_DeltaTime += DeltaTime;
    while ((Iterations--) && (m_DeltaTime > 0)) {
        // Compute time step
        float TimeStep = std::min(m_Settings.m_MaxTimeStep, m_DeltaTime);

        // Step physics simulation
        Step(TimeStep, om);

        // Update accumulated time
        m_DeltaTime -= TimeStep;
    }
}
