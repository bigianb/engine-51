
#include "ThirdPersonCamera.h"
#include "../objectManager/ObjectManager.h"
#include "../collisionMgr/CollisionMgr.h"
#include <cassert>

static float DISTANCE_MIN_ACCELERATION = 100.0f;
static float CAMERA_SPHERE_RADIUS = 20.0f;

static struct third_person_camera_desc : public object_desc
{
    third_person_camera_desc(void)
        : object_desc(
              Object::TYPE_THIRD_PERSON_CAMERA,
              "Third Person Camera",
              "",
              Object::ATTR_NEEDS_LOGIC_TIME,
              0)
    {
    }

    virtual Object* Create(ObjectManager* om, collision_mgr* cm) { return new third_person_camera(om, cm); }

} s_Third_Person_Camera;

third_person_camera::third_person_camera(ObjectManager* om, collision_mgr* cm) : Object(om)
    , collisionManager(cm)
    , m_OrbitPoint(0.0f, 0.0f, 0.0f)
    , m_DesiredOrbitPoint(0.0f, 0.0f, 0.0f)
    , m_OrbitPointVelocity(0.0f, 0.0f, 0.0f)
    , m_Distance(0.0f)
    , m_DesiredDistance(0.0f)
    , m_DistanceVelocity(0.0f)
    , m_DistanceAcceleration(DISTANCE_MIN_ACCELERATION)
    , m_Pitch(R_0)
    , m_DesiredPitch(R_0)
    , m_PitchVelocity(R_0)
    , m_Yaw(R_0)
    , m_DesiredYaw(R_0)
    , m_YawVelocity(R_0)
    , m_HostPlayerGuid(0)
{
}

const object_desc& third_person_camera::GetTypeDesc() const
{
    return s_Third_Person_Camera;
}

const object_desc& third_person_camera::GetObjectType()
{
    return s_Third_Person_Camera;
}

void third_person_camera::ComputeView(view& View) const
{
    View.LookAtPoint(m_CameraPos, m_DesiredOrbitPoint);
}

void third_person_camera::MoveTowards(const Vector3& DesiredPosition)
{
    view View;
    View.OrbitPoint(m_DesiredOrbitPoint, m_DesiredDistance, m_DesiredPitch, m_DesiredYaw);
    View.SetPosition(DesiredPosition);
    View.GetPitchYaw(m_DesiredPitch, m_DesiredYaw);
    m_DesiredDistance = (View.GetPosition() - m_DesiredOrbitPoint).Length();
}

void third_person_camera::MoveTowards(Radian Pitch, Radian Yaw, float Distance)
{
    m_DesiredPitch = Pitch;
    m_DesiredYaw = Yaw;
    m_DesiredDistance = Distance;
}

void third_person_camera::SetOrbitPoint(const Vector3& DesiredOrbitPoint)
{
    m_DesiredOrbitPoint = DesiredOrbitPoint;
}

void third_person_camera::OnAdvanceLogic(float DeltaTime)
{
    Object::OnAdvanceLogic(DeltaTime);

    // Now that everything is updated, handle the zone tracking
    view View;
    ComputeView(View);
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, View.GetPosition());
}

void third_person_camera::RotateYaw(Radian DeltaYaw)
{
    m_DesiredYaw += DeltaYaw;
}

void third_person_camera::MoveTowardsPitch(Radian NewPitch)
{
    m_DesiredPitch = NewPitch;
}

bool third_person_camera::CheckForObstructions(const Vector3& Dir, float DistToCheck, float& MaxDistFound)
{
    // Assume we have a clear line of sight.
    MaxDistFound = DistToCheck;

    // Figure out the end point for our ray test.
    Vector3 Delta = Dir;
    Delta.NormalizeAndScale(DistToCheck);
    float Length = Delta.Length();

    // Do a ray test first to see how far we can move the camera.
    collisionManager->RaySetup(GetGuid(), m_OrbitPoint, m_OrbitPoint + Delta);
    collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                   (Object::object_attr)(Object::ATTR_BLOCKS_CHARACTER | Object::ATTR_BLOCKS_CHARACTER_LOS),
                                   (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE));

    int nRayCollisions = collisionManager->m_nCollisions;
    if (nRayCollisions > 0) {
        // We hit something
        MaxDistFound = collisionManager->m_Collisions[0].T * Length;
        if (MaxDistFound < (CAMERA_SPHERE_RADIUS + 5)) {
            // The camera sphere would intersect geometry immediately
            // No point in doing the sphere test.  Just bail.
            MaxDistFound = 0;
            return true;
        }
    }

    // The above collision check was a good start, but there are too many cracks in the world,
    // and places where we can squeeze through parts we shouldn't. (One example would be the
    // camera going through a stairwell.)

    // Next, do a sphere test to see how far back the camera can get and
    // still have a good shot of the target
    collisionManager->SphereSetup(GetGuid(), m_OrbitPoint, m_OrbitPoint + Delta, CAMERA_SPHERE_RADIUS);
    collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES,
                                   (Object::object_attr)(Object::ATTR_BLOCKS_CHARACTER | Object::ATTR_BLOCKS_CHARACTER_LOS),
                                   (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE));

    // What was the distance from our sphere check?
    float SphereMaxDist;
    if (collisionManager->m_nCollisions) {
        SphereMaxDist = DistToCheck * collisionManager->m_Collisions[0].T;
    } else {
        SphereMaxDist = DistToCheck;
    }

    // Did we shoot out of the world?
    if (((collisionManager->m_nCollisions == 0) && nRayCollisions) ||
        (SphereMaxDist > MaxDistFound)) {
        // This means we probably shot out of the world, or at least outside of a wall
        // that we were pressed against. If the camera angle is oblique enough and the
        // sphere is already intersecting a wall, then the ray test might have a collision
        // where the sphere test would fail or the sphere would intersect a further object.
        // We'll assume we could've gone outside the world and mark this as an obstructed
        // view.
        MaxDistFound = 0;
        return true;
    }

    MaxDistFound = SphereMaxDist;

    // No collisions from the sphere test? Our camera is good to go...
    if (collisionManager->m_nCollisions == 0) {
        return false;
    }

    return true;
}

//==============================================================================

void third_person_camera::Setup(const Vector3& InitialOrbitPoint, const Vector3& IdealAimDirection, float StartDist, float EndDist, Object* pOrbitObject)
{
    Radian P, Y;
    IdealAimDirection.GetPitchYaw(P, Y);

    m_OrbitPoint = InitialOrbitPoint;

    Matrix4 L2W;
    L2W.Setup(Vector3(1, 1, 1), Radian3(0, 0, 0), m_OrbitPoint);
    OnTransform(L2W);

    int i;

    P = -R_20;

    float  BestDist = 0;
    Radian BestY = x_ModAngle(-Y);
    Radian StepY = R_360 / 16;

    // We'll try the prefered yaw, but then step around a circle looking for something better
    for (i = 0; i < 16; i++) {
        Vector3 Dir(0, 0, std::max(EndDist, StartDist));
        Dir.RotateX(P);
        Dir.RotateY(Y);
        Dir.Normalize();

        float DistFound;

        if (CheckForObstructions(Dir, StartDist, DistFound)) {
            if (DistFound > BestDist) {
                BestDist = DistFound;
                BestY = Y;
            }
        } else {
            // We found an unobstructed direction!
            BestY = Y;
            BestDist = StartDist;
            break;
        }

        Y += StepY;
        if (Y > R_360) {
            Y -= R_360;
        }
    }

    m_Distance = BestDist;
    m_Pitch = P;
    m_Yaw = BestY;
    m_DistanceAcceleration = DISTANCE_MIN_ACCELERATION;

    m_CameraPos.set(0, 0, m_Distance);
    m_CameraPos.RotateX(m_Pitch);
    m_CameraPos.RotateY(m_Yaw);
    m_CameraPos += InitialOrbitPoint;

    m_CameraRodEnds[0] = InitialOrbitPoint;
    m_CameraRodEnds[1] = m_CameraPos;
    m_CameraRodLength = (m_CameraRodEnds[0] - m_CameraRodEnds[1]).Length();
    m_iDesiredRodEnd = 1;

    assert(pOrbitObject);
    SetZones(pOrbitObject->GetZones());
    g_ZoneMgr.InitZoneTracking(*pOrbitObject, m_ZoneTracker);
    g_ZoneMgr.UpdateZoneTracking(*this, m_ZoneTracker, m_CameraPos);
}

//==============================================================================
bool third_person_camera::HaveClearView() const
{
    view View;
    ComputeView(View);

    Vector3 NewPos(View.GetPosition());
    g_CollisionMgr.SphereSetup(m_HostPlayerGuid, m_OrbitPoint, NewPos, 10.0f);
    g_CollisionMgr.UseLowPoly();
    g_CollisionMgr.SetMaxCollisions(MAX_COLLISION_MGR_COLLISIONS);
    g_CollisionMgr.CheckCollisions(
        Object::TYPE_ALL_TYPES,                               // these types
        Object::ATTR_COLLIDABLE,                              // these attributes
        Object::ATTR_PLAYER | Object::ATTR_CHARACTER_OBJECT); // not these attributes

    const bool Result = !(g_CollisionMgr.m_nCollisions > 0);
    return Result;
}
