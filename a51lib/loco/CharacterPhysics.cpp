
#include "CharacterPhysics.h"
#include "../objectManager/ObjectManager.h"
#include "../objects/Actor.h"
#include "../objects/AnimSurface.h"
#include "../objects/Player.h"
#include "../collisionMgr/CollisionMgr.h"
#include "../collisionMgr/PolyCache.h"
#include <cassert>

static const float REALLY_SMALL = 0.00001f;
static const float MIN_DELTA_TO_CLIMB_STEEPS = 10.0f;
static const float MIN_DELTA_TO_CLIMB_STEEPS_2 = x_sqr(MIN_DELTA_TO_CLIMB_STEEPS);
static const float SLIDE_PLANE_BACKOFF = 0.01f;

bool g_UpdateRidingPlatform = true;
bool g_ApplyPlatformCollision = true;

//=========================================================================
// FUNCTIONS
//=========================================================================

character_physics::character_physics(ObjectManager* pObjectManager, collision_mgr* pCollisionMgr)
 {
    m_pObjectManager = pObjectManager;
    collisionManager = pCollisionMgr;
    m_NavCollisionHeight = 180.0f;
    m_NavCollisionCurentHeight = m_NavCollisionHeight;
    m_NavCollisionRadius = 30.0f;
    m_NavCollisionCrouchOffset = 40.0f;
    m_AirControl = 0.04f;
    m_FlingAC = 0.0f;
    m_MaxCollisions = 8;
    m_bHandlePermeable = false;
    m_bNavCollided = false;
    m_bFallMode = false;
    m_bJumpMode = false;
    m_bFlingMode = false;
    m_bTrackingGround = true;
    m_bUseGravity = true;
    m_bLocoGravityOn = true;
    m_bLocoCollisionOn = true;
    m_CollisionSnuggleDistance = 1.0f;
    m_GroundTolerance = 5.0f;
    m_FallTolerance = 50.0f;
    m_VelocityForFallMode = 980.0f;
    m_GravityAcceleration = -1000.0f;
    m_MaxDistanceToGround = 20.0f;
    m_SteepestSlide = 0.5f;
    m_GroundPlane.Setup(1, 0, 0, 0);
    m_GroundGuid = 0;
    m_GroundPos.Zero();
    m_GroundBoneIndex = 0;
    m_MovingPlatformGuid = 0;
    m_MovingPlatformBone = -1;
    m_DeltaPosIndex = 0;
    m_ActorCollisionRadius = 50.0f;
    m_SolveActorCollisions = false;
    m_IgnoreGuid = 0;
    m_LastFling = 0;
    m_OldMovingPlatformVelocity.Zero();
    m_OldMovingPlatformL2W.Identity();

    m_nPlatformsToIgnore = 0;

    m_Velocity.set(0, 0, 0);
    m_Position.set(0, 0, 0);

    m_LastMove.set(0.0f, 0.0f, 0.0f);
    m_LastSteepSurface = 0;

    ResetDeltaPos();

    memset(m_TriggerGuid, 0, sizeof(m_TriggerGuid));
    m_nTriggerGuids = 0;
}

//=========================================================================

void character_physics::Init(guid Guid)
{
    m_Guid = Guid;
}

//=========================================================================
static const float k_KeepAwayDistance = 150.0f;

void character_physics::SolveActorAndPlatformCollisions()
{
    if (m_bLocoCollisionOn == false) {
        return;
    }

    //
    // Build bbox around character
    //
    BBox WorldBBox;
    WorldBBox.max.z = WorldBBox.max.x = m_ActorCollisionRadius;
    WorldBBox.min.z = WorldBBox.min.x = -WorldBBox.max.x;
    WorldBBox.max.y = GetColHeight();
    WorldBBox.min.y = 0;
    WorldBBox.Translate(m_Position);

    if (m_SolveActorCollisions) {
        //
        // Loop through list and process characters
        //
        SolveActorCollisions(WorldBBox);
    }

    //
    // Loop through list and process animated surfaces
    //
    SolvePlatformCollisions();
}

//=========================================================================

void character_physics::SolveActorCollisions(const BBox& ActorBBox)
{
    Object* pOurCharacter = m_pObjectManager->GetObjectByGuid(m_Guid);
    if (!pOurCharacter) {
        return;
    }
    assert(pOurCharacter->IsKindOf(actor::GetRTTI()));

    // Build actor's bbox
    int     nActorsChecked = 0;
    Vector3 FinalMoveDelta(0.0f, 0.0f, 0.0f);

    actor* pActor = actor::m_pFirstActive;
    while (pActor) {
        if ((pActor != pOurCharacter) &&
            (pActor->GetGuid() != m_IgnoreGuid) &&
            (!pActor->IsDead())) // We don't want to collide with dead players!
        {
            float ActorCollisonRadius = pActor->GetActorCollisionRadius();

            // calc cylindrical distance
            Vector3 CToUs = m_Position - pActor->GetPosition();
            CToUs.y = 0;
            float DistBetweenSq = CToUs.LengthSquared();
            float RadiusSum = m_ActorCollisionRadius + ActorCollisonRadius;

            if (DistBetweenSq < RadiusSum * RadiusSum) {
                // Do vertical overlap test
                BBox bb = pActor->GetBBox();
                if ((bb.max.GetY() >= ActorBBox.min.GetY()) && (ActorBBox.max.GetY() >= bb.min.GetY())) {
                    float PenetrationDist = RadiusSum - sqrt(DistBetweenSq);
                    assert(PenetrationDist >= 0);

                    Vector3 MoveDelta = CToUs;
                    MoveDelta.NormalizeAndScale(PenetrationDist);

                    // adding a player check so that NPCs can push the player but the player can't push NPCs.
                    // Otherwise the player can squeeze between things and push NPCs away from consoles and the like
                    // creating nasty bugs.
                    if (!pActor->IsPlayer() /*|| pOurCharacter->IsKindOf( player::GetRTTI() ) */) {
                        FinalMoveDelta += MoveDelta;
                    }

                    nActorsChecked++;

                    ((actor*)pOurCharacter)->SetCollidedActor(pActor->GetGuid());
                }
            }
        }

        // move to next actor
        pActor = pActor->m_pNextActive;
    }

    if (nActorsChecked) {
        Push(FinalMoveDelta);
    }
}

//=========================================================================

void character_physics::AdvanceWithoutCollision(const Vector3& MoveTo, float DeltaTime, bool bIsDead)
{
    Vector3 Delta = MoveTo - m_Position;

    m_Velocity = Delta / DeltaTime;
    m_LastMove = MoveTo - m_Position;
    m_Position = MoveTo;

    m_bFallMode = false;
    m_bFlingMode = false;

    m_GroundGuid = 0;
    m_GroundPlane.Setup(m_Position, Vector3(0, 1, 0));
    m_GroundPos = m_Position;
    m_GroundBoneIndex = 0;
}

//=========================================================================

void character_physics::Advance(const Vector3& MoveTo, float DeltaTime, bool bIsDead)
{
    static int FallCounter = 0;
    Vector3    PositionOnEntry = m_Position;
    Vector3    VelocityOnEntry = m_Velocity;

    Vector3 OldPos = m_Position;
    Vector3 NewPos = m_Position;
    Vector3 Delta = MoveTo - m_Position;
    Vector3 Momentum = m_Velocity;

    m_Velocity = Delta / DeltaTime;

    if (m_bLocoGravityOn == false) {
        m_bFallMode = false;
        m_bFlingMode = false;
        m_bTrackingGround = false;
    }

    //
    // Update information about ground!
    //
    float GroundCheckDist = 50.0f; //-fMin( DeltaTime * m_Velocity.Y - m_GroundTolerance*2.0f, -m_GroundTolerance*2.0f);
                                   //    UpdateGround( GroundCheckDist );

    //
    // Update physics and apply gravity etc.
    //
    UpdatePhysics(DeltaTime);

    if (m_bFallMode == true) {
        float AirControl = m_AirControl;

        m_Velocity.x = Momentum.GetX() + AirControl * (m_Velocity.GetX() - Momentum.GetX());
        m_Velocity.z = Momentum.GetZ() + AirControl * (m_Velocity.GetZ() - Momentum.GetZ());
        m_Velocity.y += Momentum.GetY();
    }

    HandleMove(NewPos,
               m_Velocity,
               DeltaTime,
               0.0f,
               m_SteepestSlide);

    // Resolve any penetrations
    m_Position = NewPos;
    ResolvePenetrations();
    NewPos = m_Position;

    // Track ground following the move
    if (UpdateGround(GroundCheckDist)) {
        if (m_bTrackingGround) {
            if (NewPos.GetY() > m_GroundPos.GetY()) {
                NewPos.y = m_GroundPos.GetY();
            }
        }
    } else {
        m_bTrackingGround = false;
    }

    m_LastMove = NewPos - OldPos;
    m_Position = NewPos;

        SolveActorAndPlatformCollisions();

    // TODO: E3: CJ: Hack to get the player out of stuck situations
    if (m_bFallMode && (VelocityOnEntry.GetY() < -50.0f)) {
        if (abs(PositionOnEntry.GetY() - m_Position.GetY()) < 0.1f) {
            FallCounter++;
            if (FallCounter > 2) {;
                m_Velocity.y = 0.0f;
                m_bFallMode = false;
                m_bFlingMode = false;
            }
        } else {
            FallCounter = 0;
        }
    } else {
        FallCounter = 0;
    }
}

//=========================================================================

#define HACK_MAX_PERMEABLE_CP (MAX_COLLISION_MGR_COLLISIONS)
void character_physics::CollectPermeable(Object* pActor, const Vector3& StartPos, const Vector3& EndPos)
{
    assert(pActor);

    if (strcmp(typeid(pActor).name(), "character*") == 0){
        // We are assuming this actor is a character!

        int i, j;

        guid NewGuid[CHARACTER_PHYSICS_MAX_TRIGGERS] = {0};
        int  nNewGuids = 0;

        // Do select bbox around actor using an inflated bbox (necessary for doors)
        BBox bb = GetBBox();
        bb.Translate(EndPos);

        BBox InflatedBBox = bb;
        InflatedBBox.Inflate(350.0f, 0.0f, 350.0f);

        // Handle special-case permeable object interaction
        m_pObjectManager->SelectBBox(Object::ATTR_COLLIDABLE | Object::ATTR_COLLISION_PERMEABLE, InflatedBBox, Object::TYPE_ALL_TYPES);
        int ObjectSlot;
        for (ObjectSlot = m_pObjectManager->StartLoop();
             ObjectSlot != SLOT_NULL;
             ObjectSlot = m_pObjectManager->GetNextResult(ObjectSlot)) {
            Object* pObject = m_pObjectManager->GetObjectBySlot(ObjectSlot);
            assert(pObject);

            switch (pObject->GetType()) {
                /*
            case Object::TYPE_TRIGGER_EX:
                // triggers just get added to a guid list if they have intersected with
                // our non-inflated bbox...we'll deal with the actual collision in a moment
                if (bb.Intersect(pObject->GetBBox())) {
                    if (nNewGuids < CHARACTER_PHYSICS_MAX_TRIGGERS) {
                        NewGuid[nNewGuids++] = pObject->GetGuid();
                    }
                }
                break;
            case Object::TYPE_DOOR:
            {
                // wake up the door so it knows to start looking for us
                door& DoorObj = door::GetSafeType(*pObject);
                DoorObj.WakeUp();
            } break;
            case Object::TYPE_DAMAGE_FIELD:
                // handle the damage field collision
                pObject->OnColNotify(*pActor);
                break;
            case Object::TYPE_COKE_CAN:
            {
                actor&    Actor = actor::GetSafeType(*pActor);
                coke_can& Can = coke_can::GetSafeType(*pObject);
                Can.ApplyActorConstraints(Actor);
            } break;
             */
            default:
                // nothing to do
                break;
            }
        }
        m_pObjectManager->EndLoop();

        // Look for any NEW triggers
        for (i = 0; i < nNewGuids; i++) {
            for (j = 0; j < m_nTriggerGuids; j++) {
                if (m_TriggerGuid[j] == NewGuid[i]) {
                    break;
                }
            }

            // if we did not find the new guid in the triggerguid list
            // then we have just entered it and should notify the object
            if (j == m_nTriggerGuids) {
                Object* pObject = m_pObjectManager->GetObjectByGuid(NewGuid[i]);
                if (pObject) {
                    pObject->OnColNotify(*pActor);
                }
            }
        }

        // Copy new triggers into old triggers
        for (i = 0; i < nNewGuids; i++) {
            m_TriggerGuid[i] = NewGuid[i];
        }
        m_nTriggerGuids = nNewGuids;
    } else {
        // We are assuming this actor is a player!

        //
        // Walk from where we were to where we are.
        //
        Vector3 Start = StartPos;
        Vector3 Stop = EndPos;

        SetupPlayerCollisionCheck(Start, Stop);
        collisionManager->SetMaxCollisions(HACK_MAX_PERMEABLE_CP);

        //
        // Check the pickups here.
        //
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_COLLISION_PERMEABLE);

        for (int i = 0; i < collisionManager->m_nCollisions; i++) {
            Object* pObj = m_pObjectManager->GetObjectByGuid(collisionManager->m_Collisions[i].ObjectHitGuid);
            pObj->OnColNotify(*pActor);
        }

        // We need the player to check for can objects.
        // To speed it up a minute amount, let's check to see if there even are
        // cans present before asking for a bbox check
        if (m_pObjectManager->GetFirst(Object::TYPE_COKE_CAN) != SLOT_NULL) {
            actor& Player = actor::GetSafeType(*pActor);
            BBox   bb = GetBBox();
            bb.Translate(EndPos);
            BBox InflatedBBox = bb;
            InflatedBBox.Inflate(100.0f, 0.0f, 100.0f);

            m_pObjectManager->SelectBBox(Object::ATTR_COLLIDABLE, InflatedBBox, Object::TYPE_COKE_CAN);
            int ObjectSlot;
            for (ObjectSlot = m_pObjectManager->StartLoop();
                 ObjectSlot != SLOT_NULL;
                 ObjectSlot = m_pObjectManager->GetNextResult(ObjectSlot)) {
                Object* pObject = m_pObjectManager->GetObjectBySlot(ObjectSlot);
                assert(pObject);
                //coke_can& Can = coke_can::GetSafeType(*pObject);
                //Can.ApplyActorConstraints(Player);
            }
            m_pObjectManager->EndLoop();
        }
    }
}


void character_physics::HandleMove(
    Vector3& NewPos,
    Vector3& OriginalVelocity,
    float    DeltaTime,
    float    SlideFriction,
    float    SteepestSlide)
{
    Vector3 OldPos = NewPos;
    Vector3 Velocity = OriginalVelocity * DeltaTime;


    (void)SteepestSlide;
    assert(SlideFriction >= 0.0f);
    assert(SlideFriction <= 1.0f);
    //  assert( Velocity.LengthSquared() < (300.0f * 300.0f) ); // We're trying to move too far in a single frame
    // See mreed...

    Vector3 StartPosForPermeables = NewPos;
    Vector3 Delta = Velocity;
    int     MaxLoops = 3;
    Object* pObject = NULL;
    plane   LastPlane;
    m_bNavCollided = false;

    // Skip if collision turned off
    if (m_bLocoCollisionOn == false) {
        NewPos += Velocity;
        return;
    }

    // Quick check to see if we should even bother
    if (Delta.LengthSquared() < REALLY_SMALL) {
        return;
    }

    pObject = m_pObjectManager->GetObjectByGuid(m_Guid);

    while (1) {

        //
        // Check if we have iterated too many times
        //
        if ((MaxLoops--) == 0) {
            Delta.Zero();
            break;
        }

        //
        // Check if the current delta is too small to bother with
        //
        if (Delta.LengthSquared() < REALLY_SMALL) {
            Delta.Zero();
            break;
        }

        //
        // Decide on Start and Stop positions
        //
        Vector3 Start = NewPos;
        Vector3 Stop = NewPos + Delta;
        float   MoveLen = Delta.Length();

        //
        // Fire up cylinder collision
        //
        SetupPlayerCollisionCheck(Start, Stop);

        collisionManager->SetMaxCollisions(m_MaxCollisions);
        collisionManager->AddToIgnoreList(m_PlatformCollisionIgnoreList, m_nPlatformsToIgnore);
        if (m_IgnoreGuid) {
            collisionManager->AddToIgnoreList(&m_IgnoreGuid, 1);
        }

        //
        // Collect all possible collisions
        //
        //collisionManager->CheckCollisions( Object::TYPE_ALL_TYPES, Object::ATTR_COLLIDABLE, Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING );
        Object* ourObject = m_pObjectManager->GetObjectByGuid(m_Guid);
        if (ourObject && ourObject->IsKindOf(player::GetRTTI())) {
            collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                           Object::ATTR_BLOCKS_PLAYER,
                                           Object::ATTR_COLLISION_PERMEABLE |
                                               Object::ATTR_LIVING);
        } else {
            collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                           Object::ATTR_BLOCKS_CHARACTER,
                                           Object::ATTR_COLLISION_PERMEABLE |
                                               Object::ATTR_LIVING);
        }
        //
        // Process any permeables we might have hit
        //
        //collisionManager->NotifyPermeables();

        //
        // If we didn't hit anything then move and return
        //
        if (collisionManager->m_nCollisions == 0) {
            // The move and return occurs following this loop
            break;
        }

        //
        // We have a collision, deal with it
        //
        m_bNavCollided = true;

        const collision_mgr::collision& Coll = collisionManager->m_Collisions[0];

        //
        // let's notify the object that we just hit
        //
        if (pObject) {
            Object* pObj = m_pObjectManager->GetObjectByGuid(Coll.ObjectHitGuid);
            if (pObj) {
                pObj->OnColNotify(*pObject);
            }
        }

        //
        // We must have hit something so compute the collision T
        // but pull back a snuggle distance for numerical safety.
        // Keep from backing up into something else
        //
        float SnuggleT = m_CollisionSnuggleDistance / MoveLen;
        float CollT = Coll.T - SnuggleT;
        assert((CollT <= 1.0f));
        if (CollT < 0) {
            CollT = 0;
        }
        if (CollT > 1) {
            CollT = 1;
        }

        //
        // Update NewPos with part of Delta
        //
        NewPos += Delta * CollT;

        //
        // Compute the slide delta and normal
        //
        float RemainingT = 1 - CollT;
        assert(RemainingT >= 0);
        assert(RemainingT <= 1);
        Delta *= RemainingT;
        Vector3 SlideDelta;
        Vector3 SlidePlaneNormal = Coll.Plane.Normal;
        Vector3 Perp;
        Vector3 OldDelta = Delta;

        //
        // Is this our 2nd collision on this move between acute planes?
        //
        if ((MaxLoops == 1) && (LastPlane.Normal.Dot(Coll.SlipPlane.Normal) <= 0)) {

            // we should move parallel to the crease between the planes
            SlideDelta = LastPlane.Normal.Cross(Coll.SlipPlane.Normal);
            SlideDelta.Normalize();
            const float Dist = Delta.Dot(SlideDelta);
            SlideDelta *= Dist;

            // Use average of normals for slide plane normal
            SlidePlaneNormal = LastPlane.Normal + Coll.SlipPlane.Normal;
            SlidePlaneNormal.Normalize();

            if (!(SlideDelta.LengthSquared() <= Delta.LengthSquared() + 0.01f)) {
                SlideDelta = Delta;
                SlidePlaneNormal = Coll.Plane.Normal;
            }

            if (!(SlideDelta.LengthSquared() <= OldDelta.LengthSquared() + 0.01f)) {
                SlideDelta = OldDelta;
                SlidePlaneNormal = Coll.Plane.Normal;
            }
        } else {
            LastPlane = Coll.SlipPlane;
            Coll.SlipPlane.GetComponents(Delta, SlideDelta, Perp);        }

        //
        // See if this is too steep for us.
        //
        if ((Coll.SlipPlane.Normal.GetY() >= 0.0f) &&
            (SlideDelta.GetY() > 0.0f) &&
            (Coll.SlipPlane.Normal.GetY() < SteepestSlide)) {

            // Vertical version of our plane
            plane Plane(Coll.SlipPlane);
            Plane.Normal.y = std::min(0.0f, Plane.Normal.GetY());
            Plane.Normal.Normalize();

            // Flattened Delta
            Vector3 Flat(SlideDelta);
            Flat.y = std::max(0.0f, SlideDelta.GetY());

            // If we're not moving significantly into the plane, then we
            // just slide horizontally along it.
            if (Plane.Normal.Dot(Delta) < 0.0f) {
                // We're moving into it, see if our last move was into it
                // slow enough to prevent coasting up.
                Vector3 Into(m_LastMove);
                Into.y = 0.0f;

                float Dot = Plane.Normal.Dot(Into);
                Into = Plane.Normal * Dot;

                if (((Dot < 0) && (Into.LengthSquared() < x_sqr(MIN_DELTA_TO_CLIMB_STEEPS)) || (Coll.ObjectHitGuid == m_LastSteepSurface))) {
                    // Ok, let's move along the plane
                    Vector3 Perp; // dummy
                    Plane.GetComponents(Flat, SlideDelta, Perp);
                }

            }
        }

        //
        // Use the slide delta
        //
        Delta = SlideDelta * (1 - SlideFriction);


        // SB: 3/5/05
        // Make next delta be slightly away from the collision plane to
        // avoid further collisions with the same plane (due to float precision)
        Delta += SlidePlaneNormal * SLIDE_PLANE_BACKOFF;

        //
        // Repeat process
        //
    }

    //
    // Move whatever delta is left after the collisions
    //
    if (Delta.LengthSquared() > REALLY_SMALL) {
        NewPos += Delta;
    }
    OriginalVelocity = (NewPos - OldPos) * (1.0f / DeltaTime);

    //
    // Collect all the pickups and such
    //
    if (m_bHandlePermeable && pObject) {
        CollectPermeable(pObject, StartPosForPermeables, NewPos);
    }
}

//=========================================================================

bool character_physics::UpdateGround(float BelowDist)
{

    bool    FoundGround = false;
    Vector3 FromPos = m_Position;
    Vector3 ToPos = m_Position - Vector3(0, BelowDist, 0);

    FromPos.y += 2.0f;

    SetupPlayerCollisionCheck(FromPos, ToPos);

    Object* ourObject = m_pObjectManager->GetObjectByGuid(m_Guid);
    if (ourObject && ourObject->IsKindOf(player::GetRTTI())) {
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                       Object::ATTR_BLOCKS_PLAYER,
                                       Object::ATTR_COLLISION_PERMEABLE |
                                           Object::ATTR_LIVING);
    } else {
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                       Object::ATTR_BLOCKS_CHARACTER,
                                       Object::ATTR_COLLISION_PERMEABLE |
                                           Object::ATTR_LIVING);
    }

    if (collisionManager->m_nCollisions > 0) {
        m_GroundGuid = collisionManager->m_Collisions[0].ObjectHitGuid;
        m_GroundPos = collisionManager->m_Collisions[0].Point;
        m_GroundPos.y += 1.0f;
        m_GroundPlane = collisionManager->m_Collisions[0].SlipPlane;
        m_GroundBoneIndex = GetBoneIndexFromPolyCachePrimKey(collisionManager->m_Collisions[0].PrimitiveKey);
        FoundGround = true;

    } else {

        m_GroundGuid = 0;
        m_GroundPlane.Setup(ToPos, Vector3(0, 1, 0));
        m_GroundPos = ToPos;
        // TODO: Maybe force ground position lower down here
        m_GroundBoneIndex = 0;
    }

    return FoundGround;
}

//=========================================================================

void character_physics::UpdatePhysics(float DeltaTime)
{
    if (m_bJumpMode) {
        m_bJumpMode = false;
        m_bFallMode = true;
        return;
    }

    //FindGround( GroundVector, m_Position, DeltaTime );
    //plane       GroundPlane( collisionManager->m_Collisions[0].Plane );

    if (m_bUseGravity && m_bLocoGravityOn) {
        //  if this object should use gravity and the ground is greater than
        //  ground tolerance away from us then we modify the downward velocity
        //  Or, if the ground is too steep...
        float Tolerance = m_GroundTolerance;
        if (m_bTrackingGround) {
            float dx = m_Velocity.GetX();
            float dz = m_Velocity.GetZ();
            float d2 = dx * dx + dz * dz;
            if (d2 > 0.01f) {
                Tolerance = sqrt(d2) * 1.5f * DeltaTime;
            }
            Tolerance = std::max(m_GroundTolerance, Tolerance);
        }

        if ((m_Position.GetY() - m_GroundPos.GetY()) > Tolerance) {
            m_bFallMode = true;
            m_bTrackingGround = false;

            //  Simple physics.
            m_Velocity.y += DeltaTime * m_GravityAcceleration;
        }
        // On a really steep surface?
        else if (m_GroundPlane.Normal.GetY() < m_SteepestSlide) {
            m_bFallMode = true;
            m_bTrackingGround = false;

            //  Simple physics,
            m_Velocity.y += DeltaTime * m_GravityAcceleration;

            // too steep, record the surface
            m_LastSteepSurface = m_GroundGuid;
        }
        // Ok, we're on the ground, make sure we don't think we're falling
        else {
            if (m_bFallMode) // mreed: this if () makes it possible to set a breakpoint on landing
            {
#ifdef LOG_CHARACTER_PHYSICS
                CLOG_MESSAGE(g_LogCharacterPhysics, "character_physics::UpdatePhysics", "Landed");
#endif
                m_bFallMode = false;
                m_bFlingMode = false;
                m_bTrackingGround = true;
                m_Position = m_GroundPos;
            }
        }
    } else {
        m_bFallMode = false;
        m_bFlingMode = false;
    }

    // Here we modify the velocity vector to make it follow downslopes <= 50 degrees
    if (!m_bFallMode && m_bUseGravity && m_bLocoGravityOn) {
        Vector3 Velocity = m_Velocity;
        Velocity.Normalize();
        float Dot = m_GroundPlane.Dot(Velocity);
        if ((Dot >= 0)        // Angle < 90 degrees
            && (Dot < 0.68f)) // Angle > 48 degrees
        {
            // stick to the ground
            float   Speed = m_Velocity.Length();
            Vector3 Perpendicular; // dummy placeholder
            m_GroundPlane.GetComponents(m_Velocity, m_Velocity, Perpendicular);
            m_Velocity.NormalizeAndScale(Speed);
        }
    }
}

//=========================================================================

bool character_physics::SetCrouchParametric(float NormalizePercent)
{
    //-----------------------------------------------------------------
    // Single player campaign crouch
    //-----------------------------------------------------------------

    assert(NormalizePercent >= 0);
    assert(NormalizePercent <= 1);

    Vector3 NewPosition;
    float   NewHeight;

    if (m_bFallMode == true) {
        float HeadPosition = m_Position.GetY() + m_NavCollisionCurentHeight;
        NewHeight = m_NavCollisionHeight - NormalizePercent * m_NavCollisionCrouchOffset;

        NewPosition = m_Position;
        NewPosition.y = HeadPosition - NewHeight;

        // The head doesn't move so check from the head to the floor
        collisionManager->SphereSetup(m_Guid, NewPosition + Vector3(0, (NewHeight - m_NavCollisionRadius), 0), NewPosition, m_NavCollisionRadius);
        collisionManager->UseLowPoly();
    } else {
        NewPosition = m_Position;
        NewHeight = m_NavCollisionHeight - NormalizePercent * m_NavCollisionCrouchOffset;

        // The head moves down so must check from the floor the the head
        collisionManager->SphereSetup(m_Guid, NewPosition, NewPosition + Vector3(0, (NewHeight - m_NavCollisionRadius), 0), m_NavCollisionRadius);
        collisionManager->UseLowPoly();
    }

    Object* ourObject = m_pObjectManager->GetObjectByGuid(m_Guid);
    if (ourObject && ourObject->IsKindOf(player::GetRTTI())) {
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                       Object::ATTR_BLOCKS_PLAYER,
                                       Object::ATTR_COLLISION_PERMEABLE |
                                           Object::ATTR_LIVING);
    } else {
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                       Object::ATTR_BLOCKS_CHARACTER,
                                       Object::ATTR_COLLISION_PERMEABLE |
                                           Object::ATTR_LIVING);
    }

    if (collisionManager->m_nCollisions != 0) {
        return false;
    }

    m_NavCollisionCurentHeight = NewHeight;
    m_Position = NewPosition;

    return true;
}

//=========================================================================
void character_physics::Jump(float Vel)
{
    if (m_bFallMode) {
        return;
    }

    m_Velocity += Vector3(0, Vel, 0);
    m_bJumpMode = true;
    m_bTrackingGround = false;
}

//=========================================================================

void character_physics::Fling(const Vector3& Velocity,
                              float          DeltaTime,
                              float          AirControl,
                              bool           FlingOnly,
                              bool           ReflingOnly,
                              bool           Instantaneous,
                              guid           FlingGuid)
{
    if (FlingOnly && m_bFlingMode) {
        return;
    }

    if (ReflingOnly && !m_bFlingMode) {
        return;
    }

    if (m_bFlingMode && (m_LastFling == FlingGuid)) {
        return;
    }

    if (Instantaneous) {
        m_Velocity = Velocity;
        m_LastFling = FlingGuid;
    } else {
        m_Velocity += Velocity * DeltaTime;
        m_LastFling = 0;
    }

    m_FlingAC = AirControl;
    m_bJumpMode = true;
    m_bFlingMode = true;
    m_bTrackingGround = false;
}

//=========================================================================

void character_physics::OnEnumProp(prop_enum& List)
{
    List.PropEnumHeader("CharacterPhysics", "This is were the physics of the character get descrive", 0);
    List.PropEnumBool("CharacterPhysics\\IsCollided", "Tells the system that the object encounter a collision", PROP_TYPE_READ_ONLY);
    List.PropEnumBool("CharacterPhysics\\IsFalling", "Tells the system that the object is in a falling mode", PROP_TYPE_READ_ONLY);
    List.PropEnumVector3("CharacterPhysics\\Position", "The position of the object", PROP_TYPE_READ_ONLY);
    List.PropEnumVector3("CharacterPhysics\\Velocity", "The Velocity of the object", PROP_TYPE_READ_ONLY);
    List.PropEnumFloat("CharacterPhysics\\ColHeight", "This is the physical height of the object in cm. It descrives a cylinder.", 0);
    List.PropEnumFloat("CharacterPhysics\\ColRadius", "This is the physical width of the object in cm. It descrives a cylinder.", 0);
    List.PropEnumFloat("CharacterPhysics\\ColCrouchOffset", "This is when in couch how much the Y off the bbox nneds to be subtracted", 0);
    List.PropEnumInt("CharacterPhysics\\MaxCollisions", "This descrives how many collisions can the object handle at ones", 0);
    List.PropEnumBool("CharacterPhysics\\DoPermeable", "When true objects that are Permeable will be notify of collisions", 0);
    List.PropEnumBool("CharacterPhysics\\UseGravity", "Tells the system whether it should use gravery or not", 0);
    List.PropEnumFloat("CharacterPhysics\\GravityAcc", "This is the gravety acceleration for the physics", 0);
    List.PropEnumFloat("CharacterPhysics\\ColSnuggleDis", "This is the minimun distance that the object maintains with its colliders", 0);
    List.PropEnumFloat("CharacterPhysics\\GroundTolerance", "This variable tells the system which height the system will start chking for the ground", 0);
    List.PropEnumFloat("CharacterPhysics\\FallTolerance", "Tells the system what height the player needs to be to the ground before he consider itself to be falling", 0);
    List.PropEnumFloat("CharacterPhysics\\AirControl", "While in the jump or fall how much control does the character has in the air. From 0 to 1.", 0);
    List.PropEnumFloat("CharacterPhysics\\VelForFall", "Tells the system at what Y velocity the object needs to be traveling before it consider itseld to be falling", 0);
    List.PropEnumFloat("CharacterPhysics\\MaxGroundDistance", "Tells the system what is the maximun distance that it will search for the ground", 0);
    List.PropEnumFloat("CharacterPhysics\\SlideFriction", "Indicates the slide friction of the object", 0);
    List.PropEnumFloat("CharacterPhysics\\ActorCollisionRadius", "Our collision radius vs other actors", 0);
}

//=========================================================================

bool character_physics::OnProperty(prop_query& I)
{
    if (I.VarBool("CharacterPhysics\\IsCollided", m_bNavCollided)) {
    } else if (I.VarBool("CharacterPhysics\\IsFalling", m_bFallMode)) {
    } else if (I.VarVector3("CharacterPhysics\\Position", m_Position)) {
    } else if (I.VarVector3("CharacterPhysics\\Velocity", m_Velocity)) {
    } else if (I.VarFloat("CharacterPhysics\\ColHeight", m_NavCollisionHeight)) {
        if (I.IsRead() == false) {
            m_NavCollisionCurentHeight = m_NavCollisionHeight;
        }
    } else if (I.VarFloat("CharacterPhysics\\ColRadius", m_NavCollisionRadius)) {
    } else if (I.VarFloat("CharacterPhysics\\ColCrouchOffset", m_NavCollisionCrouchOffset)) {
    } else if (I.VarInt("CharacterPhysics\\MaxCollisions", m_MaxCollisions)) {
    } else if (I.VarBool("CharacterPhysics\\DoPermeable", m_bHandlePermeable)) {
    } else if (I.VarBool("CharacterPhysics\\DoPermeable", m_bHandlePermeable)) {
    } else if (I.VarBool("CharacterPhysics\\UseGravity", m_bUseGravity)) {
    } else if (I.VarFloat("CharacterPhysics\\GravityAcc", m_GravityAcceleration)) {
    } else if (I.VarFloat("CharacterPhysics\\ColSnuggleDis", m_CollisionSnuggleDistance)) {
    } else if (I.VarFloat("CharacterPhysics\\GroundTolerance", m_GroundTolerance)) {
    } else if (I.VarFloat("CharacterPhysics\\FallTolerance", m_FallTolerance)) {
    } else if (I.VarFloat("CharacterPhysics\\AirControl", m_AirControl)) {
    } else if (I.VarFloat("CharacterPhysics\\VelForFall", m_VelocityForFallMode)) {
    } else if (I.VarFloat("CharacterPhysics\\MaxGroundDistance", m_MaxDistanceToGround)) {
    } else if (I.VarFloat("CharacterPhysics\\SlideFriction", m_SteepestSlide)) {
        I.VarFloat("CharacterPhysics\\SlideFriction", m_SteepestSlide);
    } else if (I.VarFloat("CharacterPhysics\\ActorCollisionRadius", m_ActorCollisionRadius)) {
    } else {
        return false;
    }

    return true;
}

//=========================================================================

void character_physics::ApplyCollision(void)
{
    Vector3 SpherePos[16];
    int     nSpheres;
    int     i;

    collisionManager->StartApply(m_Guid);

    nSpheres = collisionManager->GetCylinderSpherePositions(
        m_Position,
        m_Position + Vector3(0, m_NavCollisionCurentHeight, 0),
        m_NavCollisionRadius,
        SpherePos,
        Object::MAT_TYPE_FLESH);
    for (i = 0; i < nSpheres; i++) {
        collisionManager->ApplySphere(SpherePos[i], m_NavCollisionRadius, Object::MAT_TYPE_FLESH);
    }

    collisionManager->EndApply();
}

void character_physics::CopyValues(character_physics& rPhysics)
{
    m_bFallMode = rPhysics.GetFallMode();
    m_bJumpMode = rPhysics.GetJumpMode();
    m_Velocity = rPhysics.GetVelocity();
    m_Position = rPhysics.GetPosition();
}

//=========================================================================

void ComputePlatformMotion(const Matrix4& OldPlatformM,
                           const Matrix4& NewPlatformM,
                           const Vector3& CurrentPos,
                           Vector3&       DeltaPos,
                           Radian&        DeltaYaw)
{
    //
    // Compute motion matrix
    //
    Matrix4 DeltaM = OldPlatformM;
    DeltaM.InvertSRT();
    DeltaM = NewPlatformM * DeltaM;

    //
    // Compute DeltaPos
    //
    Vector3 NewPos = DeltaM * CurrentPos;
    DeltaPos = NewPos - CurrentPos;

    //
    // Compute DeltaYaw
    //
    Radian3 OldPlatformYaw = OldPlatformM.GetRotation();
    Radian3 NewPlatformYaw = NewPlatformM.GetRotation();
    DeltaYaw = NewPlatformYaw.yaw - OldPlatformYaw.yaw;
}

//===========================================================================

void character_physics::CatchUpWithRidingPlatform(float DeltaTime)
{
    if (!g_UpdateRidingPlatform) {
        return;
    }

    // Check that we are standing on a platform
    if (m_MovingPlatformGuid == 0) {
        return;
    }

    // Get access to the previous surface.  Does it still exist?
    anim_surface* pPlatform = (anim_surface*)m_pObjectManager->GetObjectByGuid(m_MovingPlatformGuid);
    if (!pPlatform) {
        return;
    }

    // Get access to the actor
    actor* pActor = (actor*)m_pObjectManager->GetObjectByGuid(m_Guid);
    if (!pActor) {
        return;
    }

    //
    // We need to move the amount that the current platform did.
    //
    {
        // Get new matrix
        Matrix4 NewMovingPlatformL2W;
        pPlatform->GetBoneL2W(m_MovingPlatformBone, NewMovingPlatformL2W);

        Vector3 DeltaPos;
        Radian  DeltaYaw;
        ComputePlatformMotion(m_OldMovingPlatformL2W,
                              NewMovingPlatformL2W,
                              pActor->GetPosition(),
                              DeltaPos,
                              DeltaYaw);

        //
        // Remember velocity
        //
        m_OldMovingPlatformVelocity = DeltaPos / DeltaTime;

        //
        // Remember new platform L2W
        //
        m_OldMovingPlatformL2W = NewMovingPlatformL2W;

        //
        // Get current position and yaw
        //
        const Matrix4& L2W = pActor->GetL2W();
        Vector3        CurrPos = L2W.GetTranslation();
        Radian         CurrYaw = L2W.GetRotation().yaw;

        //
        // Update yaw
        //
        CurrYaw += DeltaYaw;

        //
        // Update position
        //
        m_PlatformCollisionIgnoreList[0] = m_MovingPlatformGuid;
        m_nPlatformsToIgnore = 1;

        Vector3 NewVector = DeltaPos;
        HandleMove(CurrPos, NewVector, 1.0f, m_SteepestSlide, false);
        m_nPlatformsToIgnore = 0;

        //
        // Set new player information
        //
        Matrix4 NewL2W;
        NewL2W.Setup(Vector3(1, 1, 1), Radian3(0, CurrYaw, 0), CurrPos);
        pActor->OnTransform(NewL2W);
    }
}

//===========================================================================

void character_physics::ResetRidingPlatforms(void)
{
    m_MovingPlatformGuid = 0;
    m_MovingPlatformBone = -1;
    m_OldMovingPlatformVelocity.Zero();
    m_OldMovingPlatformL2W.Identity();
}

//===========================================================================

void character_physics::WatchForRidingPlatform(void)
{
    if (!g_UpdateRidingPlatform) {
        return;
    }

    anim_surface* pPlatform = nullptr;
    int           iPlatformBone = -1;
    guid          PlatformGuid = 0;

    //
    // Now check and see if we are still standing on any platform
    //
    if (m_GroundGuid) {
        Object* pObject = m_pObjectManager->GetObjectByGuid(m_GroundGuid);
        if (pObject && (pObject->GetAttrBits() & Object::ATTR_ACTOR_RIDEABLE)) {
            PlatformGuid = m_GroundGuid;
            pPlatform = (anim_surface*)pObject;
            iPlatformBone = m_GroundBoneIndex;
        }
    }

    //
    // Handle transition if there is one
    //
    if ((PlatformGuid == 0) ||
        (PlatformGuid != m_MovingPlatformGuid) ||
        (iPlatformBone != m_MovingPlatformBone)) {
        // Detach from old platform
        if ((PlatformGuid != m_MovingPlatformGuid) ||
            (iPlatformBone != m_MovingPlatformBone)) {
            // If previous platform is present inherit velocity
            if (m_MovingPlatformGuid != 0) {
                m_Velocity += m_OldMovingPlatformVelocity;
            }

            m_MovingPlatformGuid = 0;
            m_MovingPlatformBone = -1;
            m_OldMovingPlatformVelocity.Zero();
            m_OldMovingPlatformL2W.Identity();
            //x_DebugMsg("DETTACHED FROM MOVING PLATFORM\n");
        }

        // Is new ground a moving platform?
        if (PlatformGuid != 0) {
            // Attach to platform
            m_MovingPlatformGuid = PlatformGuid;
            m_MovingPlatformBone = iPlatformBone;
            pPlatform->GetBoneL2W(iPlatformBone, m_OldMovingPlatformL2W);
            //x_DebugMsg("ATTACHED TO MOVING PLATFORM\n");
        }
    }
}

//=========================================================================

bool ComputeTriSphereMovement(const Vector3& aP0,
                              const Vector3& aP1,
                              const Vector3& aP2,
                              const Vector3& SphereCenter,
                              float          SphereRadius,
                              float          SphereHalfHeight,
                              Vector3&       FinalMovement)
{
    float SphereRadiusSquared = SphereRadius * SphereRadius;
    float WorldToSphereScale = SphereRadius / SphereHalfHeight;

    // Compute points in sphere space
    Vector3 SphereSpacePt[4];
    SphereSpacePt[0] = aP0 - SphereCenter;
    SphereSpacePt[1] = aP1 - SphereCenter;
    SphereSpacePt[2] = aP2 - SphereCenter;
    SphereSpacePt[0].y *= WorldToSphereScale;
    SphereSpacePt[1].y *= WorldToSphereScale;
    SphereSpacePt[2].y *= WorldToSphereScale;
    SphereSpacePt[3] = SphereSpacePt[0];
    plane SphereSpacePlane(SphereSpacePt[0], SphereSpacePt[1], SphereSpacePt[2]);

    //
    // Sphere center is now at (0,0,0) and the sphere has radius SphereRadius
    //

    // Skip if completely in front of the plane
    float SphereCenterDistFromPlane = SphereSpacePlane.D;
    if (SphereCenterDistFromPlane >= SphereRadius) {
        return false;
    }

    // Skip if center is behind plane
    if (SphereCenterDistFromPlane < 0) {
        return false;
    }

    // Clear results
    Vector3 BestPushDir;
    float   BestPushDist = 0.0f;

    // Compute if sphere is inside edges
    bool    bInsideEdge[3];
    Vector3 ClosestPtOnPlaneToSphereCenter = -SphereSpacePlane.Normal * SphereCenterDistFromPlane;
    {
        for (int i = 0; i < 3; i++) {
            Vector3& PA = SphereSpacePt[i];
            Vector3& PB = SphereSpacePt[i + 1];
            Vector3  EdgeDir = PB - PA;
            Vector3  EdgeNormal = SphereSpacePlane.Normal.Cross(EdgeDir);
            Vector3  DeltaPoint = ClosestPtOnPlaneToSphereCenter - PA;
            float    Dot = EdgeNormal.Dot(DeltaPoint);
            bInsideEdge[i] = (Dot >= 0.0f);
        }
    }

    // Push out of plane?
    if (bInsideEdge[0] && bInsideEdge[1] && bInsideEdge[2]) {
        BestPushDist = SphereRadius - SphereCenterDistFromPlane;
        BestPushDir = -ClosestPtOnPlaneToSphereCenter;
    }

    // Push out of edges/vertices?
    if (BestPushDist == 0.0f) {
        Vector3 Zero(0, 0, 0);

        for (int i = 0; i < 3; i++) {
            // Skip if on inside of the edge (only allow pushing outwards)
            if (bInsideEdge[i]) {
                continue;
            }

            // Get edge points
            Vector3& PA = SphereSpacePt[i];
            Vector3& PB = SphereSpacePt[i + 1];

            // Get closest pt between edge and sphere center
            Vector3 CP = Zero.GetClosestVToLSeg(PA, PB);
            float   LenSquared = CP.LengthSquared();
            if (LenSquared < SphereRadiusSquared) {
                float PushDist = SphereRadius - sqrt(LenSquared);

                // Pt may intersect more than one valid edge - make sure to keep
                // the biggest push so that the sphere gets out of all the edges
                if (PushDist > BestPushDist) {
                    BestPushDist = PushDist;
                    BestPushDir = -CP;
                }
            }
        }
    }

    // If there were no close calls then bail
    if (BestPushDist == 0.0f) {
        return false;
    }

    // Normalize the push direction, move it into world space and extend it 1mm
    assert(BestPushDist > 0.0f);
    assert(BestPushDist <= SphereRadius);
    BestPushDir.Normalize();
    BestPushDir *= BestPushDist;
    BestPushDir.y /= WorldToSphereScale;
    float BPDLen = BestPushDir.Length();
    BestPushDir /= BPDLen;
    BestPushDir *= (BPDLen + 0.1f);

    FinalMovement = BestPushDir;

    return true;
}

//=========================================================================

void character_physics::SolvePlatformCollisions(void)
{
    if (!g_ApplyPlatformCollision) {
        return;
    }

    int  MaxLoops = 8;
    int  nLoops;
    BBox MoveBounds;
    MoveBounds.min.Zero();
    MoveBounds.max.Zero();

    for (nLoops = 0; nLoops < MaxLoops; nLoops++) {
        int nPlatformsHit = 0;

        m_nPlatformsToIgnore = 0;
        Vector3 AccumMoveDelta;
        AccumMoveDelta.Zero();

        //
        // Build sphere info
        //
        float   SphereRadius = m_NavCollisionRadius;
        float   SphereHalfHeight = m_NavCollisionCurentHeight * 0.5f; // CJ: CROUCH - m_NavCollisionHeight
        Vector3 SphereCenter = m_Position;
        SphereCenter.y += SphereHalfHeight;

        //
        // Build bbox around character
        //
        BBox ActorBBox;
        ActorBBox.max.z = ActorBBox.max.x = +SphereRadius;
        ActorBBox.min.z = ActorBBox.min.x = -SphereRadius;
        ActorBBox.max.y = m_NavCollisionCurentHeight; // CJ: CROUCH - m_NavCollisionHeight
        ActorBBox.min.y = 0;
        ActorBBox.min += m_Position;
        ActorBBox.max += m_Position;
        ActorBBox.Inflate(1, 1, 1);

        //
        // Gather factored out list of clusters in dynamic area
        //
        collisionManager->getPolyCache()->BuildClusterList(ActorBBox, Object::TYPE_ALL_TYPES, Object::ATTR_ACTOR_RIDEABLE, 0);

        //
        // Process clusters
        //
        if (collisionManager->getPolyCache()->m_nClusters) {
            //
            // Loop through the clusters and process the triangles
            //
            for (int iCL = 0; iCL < collisionManager->getPolyCache()->m_nClusters; iCL++) {
                bool                 bHitThisCluster = false;
                poly_cache::cluster& CL = *collisionManager->getPolyCache()->m_ClusterList[iCL];

                int iQ = -1;
                while (1) {
                    // Do tight loop on bbox checks
                    {
                        iQ++;
                        while (iQ < (int)CL.nQuads) {
                            //#### This could be moved outside the loop
                            // and do a pointer incremenet after we
                            // optimized the Vector3/bbox stuff
                            BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                            if (ActorBBox.Intersect(*pBBox)) {
                                break;
                            }
                            iQ++;
                        }
                        if (iQ == (int)CL.nQuads) {
                            break;
                        }
                    }

                    // Process this quad
                    poly_cache::cluster::quad& QD = CL.pQuad[iQ];

                    {
                        Vector3  MoveDelta;
                        Vector3* PT[4];
                        PT[0] = (Vector3*)(&CL.pPoint[QD.iP[0]]);
                        PT[1] = (Vector3*)(&CL.pPoint[QD.iP[1]]);
                        PT[2] = (Vector3*)(&CL.pPoint[QD.iP[2]]);
                        PT[3] = (Vector3*)(&CL.pPoint[QD.iP[3]]);

                        if (ComputeTriSphereMovement(*PT[0], *PT[1], *PT[2], SphereCenter, SphereRadius, SphereHalfHeight, MoveDelta)) {
                            nPlatformsHit++;
                            MoveBounds += MoveDelta;
                            AccumMoveDelta += MoveDelta;
                            bHitThisCluster = true;
                        }

                        if (CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD) {
                            if (ComputeTriSphereMovement(*PT[0], *PT[2], *PT[3], SphereCenter, SphereRadius, SphereHalfHeight, MoveDelta)) {
                                nPlatformsHit++;
                                MoveBounds += MoveDelta;
                                AccumMoveDelta += MoveDelta;
                                bHitThisCluster = true;
                            }
                        }
                    }
                }

                if (bHitThisCluster) {
                    int i;

                    for (i = 0; i < m_nPlatformsToIgnore; i++) {
                        if (m_PlatformCollisionIgnoreList[i] == CL.Guid) {
                            break;
                        }
                    }

                    if (i == m_nPlatformsToIgnore) {
                        if (m_nPlatformsToIgnore < 8) {
                            m_PlatformCollisionIgnoreList[m_nPlatformsToIgnore] = CL.Guid;
                            m_nPlatformsToIgnore++;
                        }
                    }
                }
            }
        }

        // No collisions found so bail
        if (nPlatformsHit == 0) {
            break;
        }

        Vector3 FinalMoveDelta(MoveBounds.min + MoveBounds.max);

        // Move by FinalMoveDelta
        Vector3 NewVector = FinalMoveDelta;
        HandleMove(m_Position, NewVector, 1.0f, 0, false);
        m_nPlatformsToIgnore = 0;
    }

    //
    // Check if actor is trapped and crushed!!
    //

    if (nLoops == MaxLoops) {
        Object* pObj = m_pObjectManager->GetObjectByGuid(m_Guid);
        if (pObj) {
            // Kill the character
            pain PainEvent(m_pObjectManager);
            PainEvent.Setup("GENERIC_LETHAL", 0, pObj->GetBBox().GetCenter());
            PainEvent.SetDirectHitGuid(pObj->GetGuid());
            PainEvent.ApplyToObject(pObj);
        }
    }
}

//==============================================================================

void character_physics::SetDeltaPos(Vector3& DeltaPos)
{
    m_LstDeltaPos[m_DeltaPosIndex] = DeltaPos;
    m_DeltaPosIndex += 1;

    if (m_DeltaPosIndex == NUM_STILL_FRAMES) {
        m_DeltaPosIndex = 0;
    }
}
//==============================================================================

Vector3 character_physics::GetRecentDeltaPos(void)
{
    Vector3 vSum(0.f, 0.f, 0.f);
    for (int i = 0; i < NUM_STILL_FRAMES; i++) {
        vSum += m_LstDeltaPos[i];
    }

    return vSum;
}

//===========================================================================

void character_physics::ResetDeltaPos(void)
{
    // Reset all of the positions in the delta pos storage.
    for (int i = 0; i < NUM_STILL_FRAMES; i++) {
        m_LstDeltaPos[i] = Vector3(100.f, 100.f, 100.f);
    }

    m_DeltaPosIndex = 0;
}

//=========================================================================

void character_physics::Push(const Vector3& PushVector)
{
    bool bPerm = m_bHandlePermeable;
    m_bHandlePermeable = false;
    Vector3 NewVector = PushVector;

    HandleMove(m_Position, NewVector, 1.0f, 0, false);
    ResolvePenetrations();
    m_bHandlePermeable = bPerm;
}

//=========================================================================

void character_physics::SetCollisionIgnoreGuid(guid ignoreGuid)
{
    m_IgnoreGuid = ignoreGuid;
}

//=========================================================================

#define PENETRATION_TOLERANCE2 (0.1f * 0.1f)

void character_physics::ResolvePenetrations(void)
{
    // Skip if collision is turned off
    if (m_bLocoCollisionOn == false) {
        return;
    }

    int     MaxLoops = 8;
    int     nLoops;
    Vector3 DeepestMove;

    BBox MoveBounds;
    MoveBounds.min.Zero();
    MoveBounds.max.Zero();

    for (nLoops = 0; nLoops < MaxLoops; nLoops++) {
        //assert(IsValidPosition(m_Position));

        Vector3 AccumMoveDelta;
        AccumMoveDelta.Zero();

        //
        // Build sphere info
        //
        float   SphereRadius = m_NavCollisionRadius;
        float   SphereHalfHeight = m_NavCollisionCurentHeight * 0.5f; 
        Vector3 SphereCenter = m_Position;
        SphereCenter.y += SphereHalfHeight;

        //
        // Build bbox around character
        //
        BBox ActorBBox;
        ActorBBox.max.z = ActorBBox.max.x = +SphereRadius;
        ActorBBox.min.z = ActorBBox.min.x = -SphereRadius;
        ActorBBox.max.y = m_NavCollisionCurentHeight; 
        ActorBBox.min.y = 0;
        ActorBBox.min += m_Position;
        ActorBBox.max += m_Position;
        ActorBBox.Inflate(1, 1, 1);

        //
        // Gather factored out list of clusters in dynamic area
        //
        uint32_t Attr = Object::ATTR_COLLIDABLE;
        Object*  pOurCharacter = m_pObjectManager->GetObjectByGuid(m_Guid);
        if (pOurCharacter && pOurCharacter->IsKindOf(player::GetRTTI())) {
            Attr = Object::ATTR_BLOCKS_PLAYER;
        } else {
            Attr = Object::ATTR_BLOCKS_CHARACTER;
        }

        g_PolyCache.BuildClusterList(ActorBBox, Object::TYPE_ALL_TYPES, Attr, Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING);

        //
        // Process clusters
        //
        if (g_PolyCache.m_nClusters) {
            // Clear deepest penetration found
            float DeepestPenetration2 = 0.0f;

            //
            // Loop through the clusters and process the triangles
            //
            for (int iCL = 0; iCL < g_PolyCache.m_nClusters; iCL++) {
                bool                 bHitThisCluster = false;
                poly_cache::cluster& CL = *g_PolyCache.m_ClusterList[iCL];

                int iQ = -1;
                while (1) {
                    // Do tight loop on bbox checks
                    {
                        iQ++;
                        while (iQ < (int)CL.nQuads) {
                            //#### This could be moved outside the loop
                            // and do a pointer incremenet after we
                            // optimized the Vector3/bbox stuff
                            BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                            if (ActorBBox.Intersect(*pBBox)) {
                                break;
                            }
                            iQ++;
                        }
                        if (iQ == (int)CL.nQuads) {
                            break;
                        }
                    }

                    // Process this quad
                    poly_cache::cluster::quad& QD = CL.pQuad[iQ];

                    {
                        Vector3  MoveDelta;
                        Vector3* PT[4];
                        PT[0] = (Vector3*)(&CL.pPoint[QD.iP[0]]);
                        PT[1] = (Vector3*)(&CL.pPoint[QD.iP[1]]);
                        PT[2] = (Vector3*)(&CL.pPoint[QD.iP[2]]);
                        PT[3] = (Vector3*)(&CL.pPoint[QD.iP[3]]);

                        if (ComputeTriSphereMovement(*PT[0], *PT[1], *PT[2], SphereCenter, SphereRadius, SphereHalfHeight, MoveDelta)) {
                            if (MoveDelta.LengthSquared() > DeepestPenetration2) {
                                DeepestPenetration2 = MoveDelta.LengthSquared();
                                DeepestMove = MoveDelta;
                                bHitThisCluster = true;
                            }
                        }

                        if (CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD) {
                            if (ComputeTriSphereMovement(*PT[0], *PT[2], *PT[3], SphereCenter, SphereRadius, SphereHalfHeight, MoveDelta)) {
                                if (MoveDelta.LengthSquared() > DeepestPenetration2) {
                                    DeepestPenetration2 = MoveDelta.LengthSquared();
                                    DeepestMove = MoveDelta;
                                    bHitThisCluster = true;
                                }
                            }
                        }
                    }
                }
            }

            // Apply the move if it's above our tolerance
            if (DeepestPenetration2 > PENETRATION_TOLERANCE2) {
                m_Position += DeepestMove;
                m_Velocity.Zero();
            } else {
                // Exit the iteration limiting for loop
                break;
            }
        }
    }
}

//=========================================================================
bool character_physics::SetupPlayerCollisionCheck(const Vector3& Start, const Vector3& End)
{
    collisionManager->CylinderSetup(m_Guid,
                                 Start,
                                 End,
                                 m_NavCollisionRadius,
                                 m_NavCollisionCurentHeight); // CJ: CROUCH - m_NavCollisionHeight

    return true; // placeholder return value in case we need to do
                 // some error checking on Start and End
}

//=========================================================================

void character_physics::SetGroundTracking(bool Track)
{
    m_bTrackingGround = Track;
}

//=========================================================================

void character_physics::InitialGroundCheck(const Vector3& Position)
{
    m_Position = Position;
    m_bTrackingGround = UpdateGround(50.0f);
}

//=========================================================================
